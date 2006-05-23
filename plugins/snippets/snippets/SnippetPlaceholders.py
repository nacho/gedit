#    Gedit snippets plugin
#    Copyright (C) 2005-2006  Jesse van den Kieboom <jesse@icecrew.nl>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

import traceback
import re
from functions import *
import os
import sys
import signal
import gobject
import select
import locale
import subprocess

# These are places in a view where the cursor can go and do things
class SnippetPlaceholder:
	def __init__(self, view, tabstop, default, begin):
		self.ok = True
		self.buf = view.get_buffer()
		self.view = view
		self.mirrors = []
		self.leave_mirrors = []
		self.tabstop = tabstop
		self.default = self.expand_environment(default)
		self.prev_contents = self.default
		
		if begin:
			self.begin = self.buf.create_mark(None, begin, True)
		else:
			self.begin = None
		
		self.end = None
	
	def literal(self, s):
		return repr(s)

	def re_environment(self, m):
		if m.group(1) or not m.group(2) in os.environ:
			return '"$' + m.group(2) + '"'
		else:
			return os.environ[m.group(2)]

	def expand_environment(self, text):
		if not text:
			return text

		return re.sub('(\\\\)?\\$([A-Z_]+)', self.re_environment, text)
	
	def get_iter(self, mark):
		if mark and not mark.get_deleted():
			return self.buf.get_iter_at_mark(mark)
		else:
			return None

	def begin_iter(self):
		return self.get_iter(self.begin)
	
	def end_iter(self):
		return self.get_iter(self.end)
	
	def run_last(self, placeholders):
		begin = self.begin_iter()
		self.end = self.buf.create_mark(None, begin, False)
		
		if self.default:
			insert_with_indent(self.view, begin, self.default, False)
	
	def remove(self, force = False):
		if self.begin and not self.begin.get_deleted():
			self.buf.delete_mark(self.begin)
		
		if self.end and not self.end.get_deleted():
			self.buf.delete_mark(self.end)
		
	# Do something on beginning this placeholder
	def enter(self):
		self.buf.move_mark(self.buf.get_insert(), self.begin_iter())

		if self.end:
			self.buf.move_mark(self.buf.get_selection_bound(), self.end_iter())
		else:
			self.buf.move_mark(self.buf.get_selection_bound(), self.begin_iter())
	
	def get_text(self):
		if self.begin and self.end:
			return self.buf.get_text(self.begin_iter(), self.end_iter())
		else:
			return ''
	
	def add_mirror(self, mirror):
		if isinstance(mirror, SnippetPlaceholderExpand):
			self.leave_mirrors.append(mirror)
		else:
			self.mirrors.append(mirror)

	def set_text(self, text):
		if self.begin.get_deleted() or self.end.get_deleted():
			return

		# Set from self.begin to self.end to text!
		self.buf.begin_user_action()
		# Remove everything between self.begin and self.end
		begin = self.begin_iter()
		self.buf.delete(begin, self.end_iter())
		# Insert the text from the mirror

		insert_with_indent(self.view, begin, text, True)
		self.buf.end_user_action()
		
		self.update_contents()

	def update_contents(self):
		prev = self.prev_contents
		self.prev_contents = self.get_text()
		
		if prev != self.get_text():
			for mirror in self.mirrors:
				if not mirror.update(self):
					return
			
	# Do something on ending this placeholder
	def leave(self):
		# Notify mirrors
		for mirror in self.leave_mirrors:
			if not mirror.update(self):
				return

# This is an placeholder which inserts a mirror of another SnippetPlaceholder	
class SnippetPlaceholderMirror(SnippetPlaceholder):
	def __init__(self, view, tabstop, begin):
		SnippetPlaceholder.__init__(self, view, -1, None, begin)
		self.mirror_stop = tabstop

	def update(self, mirror):
		self.set_text(mirror.get_text())
		return True
	
	def run_last(self, placeholders):
		SnippetPlaceholder.run_last(self, placeholders)
		
		if placeholders.has_key(self.mirror_stop):
			mirror = placeholders[self.mirror_stop]
			
			mirror.add_mirror(self)
			
			if mirror.default:
				self.set_text(mirror.default)
		else:
			self.ok = False

# This placeholder indicates the end of a snippet
class SnippetPlaceholderEnd(SnippetPlaceholder):
	def __init__(self, view, begin, default):
		SnippetPlaceholder.__init__(self, view, 0, default, begin)
	
	def run_last(self, placeholders):
		SnippetPlaceholder.run_last(self, placeholders)
		
		# Remove the begin mark and set the begin mark
		# to the end mark, this is needed so the end placeholder won't contain
		# any text
		
		if not self.default:
			self.buf.delete_mark(self.begin)
			self.begin = self.buf.create_mark(None, self.end_iter(), False)

	def enter(self):
		self.buf.move_mark(self.buf.get_insert(), self.begin_iter())
		self.buf.move_mark(self.buf.get_selection_bound(), self.end_iter())
		
	def leave(self):
		self.enter()

# This placeholder is used to expand a command with embedded mirrors	
class SnippetPlaceholderExpand(SnippetPlaceholder):
	def __init__(self, view, tabstop, begin, s):
		SnippetPlaceholder.__init__(self, view, tabstop, None, begin)

		self.mirror_text = {0: ''}
		self.timeout_id = None
		self.cmd = s

	def get_mirrors(self, placeholders):
		s = self.cmd
		mirrors = []		

		while (True):
			m = re.search('\\${([0-9]+)}', s) or re.search('\\$([0-9]+)', s)
			
			if not m:
				break

			tabstop = int(m.group(1))
			
			if placeholders.has_key(tabstop):
				if not tabstop in mirrors:
					mirrors.append(tabstop)

				s = s[m.end():]
			else:
				self.ok = False
				return None
		
		return mirrors
		
	# Check if all substitution placeholders are accounted for
	def run_last(self, placeholders):
		SnippetPlaceholder.run_last(self, placeholders)

		self.ok = True
		mirrors = self.get_mirrors(placeholders)
	
		if mirrors:
			allDefault = True
				
			for mirror in mirrors:
				p = placeholders[mirror]
				p.add_mirror(self)
				self.mirror_text[p.tabstop] = p.default
				
				if not p.default:
					allDefault = False
			
			if allDefault:
				self.expand(self.substitute())
		else:
			self.update(None)
			self.ok = False
		
	def re_placeholder(self, m):
		if m.group(1):
			return '"$' + m.group(2) + '"'
		else:
			if m.group(3):
				index = int(m.group(3))
			else:
				index = int(m.group(4))
			
			return SnippetPlaceholder.literal(self, self.mirror_text[index])

	def remove_timeout(self):
		if self.timeout_id != None:
			gobject.source_remove(self.timeout_id)
			self.timeout_id = None
		
	def install_timeout(self):
		self.remove_timeout()
		self.timeout_id = gobject.timeout_add(1000, self.timeout_cb)

	def timeout_cb(self):
		self.timeout_id = None
		
		return False
		
	def substitute(self):
		# substitute all mirrors, but also environmental variables
		text = re.sub('(\\\\)?\\$({([0-9]+)}|([0-9]+))', self.re_placeholder, 
				self.cmd)
		
		return SnippetPlaceholder.expand_environment(self, text)
	
	def update(self, mirror):
		text = None
		
		if mirror:
			self.mirror_text[mirror.tabstop] = mirror.get_text()

			# Check if all substitutions have been made
			for tabstop in self.mirror_text:
				if tabstop == 0:
					continue

				if not self.mirror_text[tabstop]:
					return False
			
		else:
			self.ok = False

		text = self.substitute()
		
		if text:
			return self.expand(text)
		
		return True

	def expand(self, text):
		return True

# The shell placeholder executes commands in a subshell
class SnippetPlaceholderShell(SnippetPlaceholderExpand):
	def __init__(self, view, tabstop, begin, s):
		SnippetPlaceholderExpand.__init__(self, view, tabstop, begin, s)

		self.shell = None
		self.remove_me = False

	def close_shell(self):
		self.shell.stdout.close()
		self.shell = None	
	
	def timeout_cb(self):
		SnippetPlaceholderExpand.timeout_cb(self)
		self.remove_timeout()
		
		if not self.shell:
			return False

		gobject.source_remove(self.watch_id)
		self.close_shell()

		if self.remove_me:
			SnippetPlaceholderExpand.remove(self)

		message_dialog(None, gtk.MESSAGE_ERROR, 'Execution of the shell ' \
				'command (%s) exceeds the maximum time, ' \
				'execution aborted.' % self.command)
		
		return False
	
	def process_close(self):
			self.close_shell()
			self.remove_timeout()

			self.set_text(str.join('', self.shell_output))
			
			if self.remove_me:
				SnippetPlaceholderExpand.remove(self, True)
		
	def process_cb(self, source, condition):
		if condition & gobject.IO_IN:
			line = source.readline()

			if len(line) > 0:
				try:
					line = unicode(line, 'utf-8')
				except:
					line = unicode(line, locale.getdefaultlocale()[1], 
							'replace')

			self.shell_output += line
			self.install_timeout()

			return True

		self.process_close()
		return False
		
	def expand(self, text):
		self.remove_timeout()

		if self.shell:
			gobject.source_remove(self.watch_id)
			self.close_shell()

		popen_args = {
			'cwd'  : None,
			'shell': True,
			'env'  : os.environ,
			'stdout': subprocess.PIPE
		}

		self.command = text
		self.shell = subprocess.Popen(text, **popen_args)
		self.shell_output = ''
		self.watch_id = gobject.io_add_watch(self.shell.stdout, gobject.IO_IN | \
				gobject.IO_HUP, self.process_cb)
		self.install_timeout()
		
		return True
		
	def remove(self, force = False):
		if not force and self.shell:
			# Still executing shell command
			self.remove_me = True
		else:
			if force:
				self.remove_timeout()
				
				if self.shell:
					self.close_shell()

			SnippetPlaceholderExpand.remove(self, force)

class TimeoutError(Exception):
	def __init__(self, value):
		self.value = value
	
	def __str__(self):
		return repr(self.value)

# The python placeholder evaluates commands in python
class SnippetPlaceholderEval(SnippetPlaceholderExpand):
	def __init__(self, view, refs, begin, s, namespace):
		SnippetPlaceholderExpand.__init__(self, view, -1, begin, s)

		self.fdread = 0
		self.remove_me = False
		self.namespace = namespace
		
		self.refs = []
		
		if refs:
			for ref in refs:
				self.refs.append(int(ref.strip()))
	
	def get_mirrors(self, placeholders):
		mirrors = SnippetPlaceholderExpand.get_mirrors(self, placeholders)
		
		if not self.ok:
			return None
		
		for ref in self.refs:
			if placeholders.has_key(ref):
				if not ref in mirrors:
					mirrors.append(ref)
			else:
				self.ok = False
				return None
		
		return mirrors
	
	def timeout_cb(self, signum = 0, frame = 0):
		raise TimeoutError, "Operation timed out (>2 seconds)"
	
	def install_timeout(self):
		if self.timeout_id != None:
			self.remove_timeout()
		
		self.timeout_id = signal.signal(signal.SIGALRM, self.timeout_cb)
		signal.alarm(2)
		
	def remove_timeout(self):
		if self.timeout_id != None:
			signal.alarm(0)
			
			signal.signal(signal.SIGALRM, self.timeout_id)

			self.timeout_id = None
		
	def expand(self, text):
		self.remove_timeout()

		text = text.strip()
		self.command = text

		if not self.command or self.command == '':
			self.set_text('')
			return

		text = "def process_snippet():\n\t" + "\n\t".join(text.split("\n"))
		
		if 'process_snippet' in self.namespace:
			del self.namespace['process_snippet']

		try:
			exec text in self.namespace
		except:
			traceback.print_exc()

		if 'process_snippet' in self.namespace:
			try:
				# Install a sigalarm signal. This is a HACK to make sure 
				# gedit doesn't get freezed by someone creating a python
				# placeholder which for instance loops indefinately. Since
				# the code is executed synchronously it will hang gedit. With
				# the alarm signal we raise an exception and catch this
				# (see below). We show an error message and return False.
				# ___this is a HACK___ and should be fixed properly (I just 
				# don't know how)				
				self.install_timeout()
				result = self.namespace['process_snippet']()
				self.remove_timeout()
			except TimeoutError:
				self.remove_timeout()

				message_dialog(None, gtk.MESSAGE_ERROR, \
				_('Execution of the python command (%s) exceeds the maximum ' \
				'time, execution aborted.') % self.command)
				
				return False
			except Exception, detail:
				self.remove_timeout()
				
				message_dialog(None, gtk.MESSAGE_ERROR, 
				_('Execution of the python command (%s) failed: %s') % 
				(self.command, detail))

				return False

			self.set_text(result)
		
		return True
