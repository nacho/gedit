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

# These are places in a view where the cursor can go and do things
class SnippetPlaceholder:
	def __init__(self, view, tabstop, default, begin):
		self.ok = True
		self.buf = view.get_buffer()
		self.view = view
		self.mirrors = []
		self.tabstop = tabstop
		self.default = self.expand_environment(default)
		
		if begin:
			self.begin = self.buf.create_mark(None, begin, True)
		else:
			self.begin = None
		
		self.end = None
	
	def literal(self, s):
		return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'

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
		if mark:
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
		insert_with_indent(self.view, begin, text)
		self.buf.end_user_action()

	# Do something on ending this placeholder
	def leave(self):
		# Notify mirrors
		for mirror in self.mirrors:
			mirror.update(self)
		

# This is an placeholder which inserts a mirror of another SnippetPlaceholder	
class SnippetPlaceholderMirror(SnippetPlaceholder):
	def __init__(self, view, tabstop, begin):
		SnippetPlaceholder.__init__(self, view, -1, None, begin)
		self.mirror_stop = tabstop
	
	def update(self, mirror):
		self.set_text(mirror.get_text())
	
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
	# s = [$n]: something shelly
	def __init__(self, view, begin, s):
		self.mirror_text = {0: ''}

		m = re.match('\\${([0-9]+)(:((\\\\:|[^:])+))?}:', s)
		
		if m:
			# This thing has a tabstop
			SnippetPlaceholder.__init__(self, view, int(m.group(1)), \
					m.group(3), begin)
			self.cmd = s[m.end():]
		else:
			SnippetPlaceholder.__init__(self, view, -1, None, begin)
			self.cmd = s

	# Check if all substitution placeholders are accounted for
	def run_last(self, placeholders):
		SnippetPlaceholder.run_last(self, placeholders)

		s = self.cmd
		mirrors = []
				
		while (True):
			m = re.search('\\${([0-9]+)}', s)
			
			if not m:
				break

			tabstop = int(m.group(1))
			
			if placeholders.has_key(tabstop):
				mirrors.append(tabstop)
				s = s[m.end():]
			else:
				self.ok = False
				break
	
		if self.ok:
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

	def substitute(self):
		# substitute all mirrors, but also environmental variables
		text = re.sub('(\\\\)?\\$({([0-9]+)}|([0-9]+))', self.re_placeholder, self.cmd)
		
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
			self.expand(text)

	def expand(self, text):
		None

# The shell placeholder executes commands in a subshell
class SnippetPlaceholderShell(SnippetPlaceholderExpand):
	def __init__(self, view, begin, s):
		SnippetPlaceholderExpand.__init__(self, view, begin, s)
		self.shell = None
		self.timeout_id = 0
		self.remove_me = False

	def remove_timeout(self):
		if self.timeout_id:
			gobject.source_remove(self.timeout_id)
			self.timeout_id = 0
		
	def install_timeout(self):
		self.remove_timeout()
		self.timeout_id = gobject.timeout_add(1000, self.timeout_cb)
	
	def close_shell(self):
		self.shell.close()
		self.shell = None	
	
	def timeout_cb(self):
		self.timeout_id = 0
		
		if not self.shell:
			return

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
		if condition & gobject.IO_HUP:
			self.process_close()
			return False
		
		if condition & gobject.IO_IN:
			inp = source.read(1)
			
			if not inp:
				self.process_close()
				return False
			else:
				self.shell_output += inp
				self.install_timeout()

			return True

	def expand(self, text):
		self.remove_timeout()

		if self.shell:
			gobject.source_remove(self.watch_id)
			self.close_shell()
		
		self.command = text
		self.shell = os.popen(text)
		self.shell_output = ''
		self.watch_id = gobject.io_add_watch(self.shell, gobject.IO_IN | \
				gobject.IO_HUP, self.process_cb)
		self.install_timeout()
		
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

# The python placeholder evaluates commands in python
class SnippetPlaceholderEval(SnippetPlaceholderExpand):
	def __init__(self, view, begin, s):
		SnippetPlaceholderExpand.__init__(self, view, begin, s)

		self.child = None
		self.fdread = 0
		self.timeout_id = 0
		self.remove_me = False

	def remove_timeout(self):
		if self.timeout_id:
			gobject.source_remove(self.timeout_id)
			self.timeout_id = 0
		
	def install_timeout(self):
		self.remove_timeout()
		self.timeout_id = gobject.timeout_add(1000, self.timeout_cb)
	
	def close_child(self):
		os.close(self.fdread)
		os.kill(self.child, signal.SIGTERM)
		self.child = None
	
	def timeout_cb(self):
		self.timeout_id = 0
		
		if not self.child:
			return

		gobject.source_remove(self.watch_id)
		self.close_child()

		if self.remove_me:
			SnippetPlaceholderExpand.remove(self)

		message_dialog(None, gtk.MESSAGE_ERROR, 'Execution of the python ' \
				'command (%s) exceeds the maximum time,' \
				'execution aborted.' % self.command)
		
		return False
	
	def process_close(self):
			self.close_child()
			self.remove_timeout()

			self.set_text(str.join('', self.child_output))
			
			if self.remove_me:
				SnippetPlaceholderExpand.remove(self, True)
		
	def process_cb(self, source, condition):
		if condition & gobject.IO_HUP:
			self.process_close()
			return False
		
		if condition & gobject.IO_IN:
			i = select.select([source], [], [], 0.0)
			inp = None
			
			if i and i[0]:
				inp = os.read(source, i[0][0])
			
			if not inp:
				self.process_close()
				return False
			else:
				self.child_output += inp
				self.install_timeout()

			return True
	
	def expand(self, text):
		self.remove_timeout()

		if self.child:
			gobject.source_remove(self.watch_id)
			self.close_child()
		
		fdread, fdwrite = os.pipe()
		self.fdread = fdread
		
		text = text.strip()
		self.command = text

		f = os.fork()
		
		if f == 0:
			# child process, close the read pipe thing
			os.close(fdread)
			# make everything on stdout go into the write pipe
			os.dup2(fdwrite, sys.stdout.fileno())

			namespace = {'__builtins__': __builtins__}

			try:
				try:
					r = eval(text, namespace)
				except SyntaxError:
					exec text in namespace
			except:
				traceback.print_exc()

			sys.exit(0)
		else:
			# parent process, close the write pipe thing
			os.close(fdwrite)
			self.child = f
			self.child_output = ''
			self.watch_id = gobject.io_add_watch(fdread, gobject.IO_IN | \
				gobject.IO_HUP, self.process_cb)
			self.install_timeout()
		
	def remove(self, force = False):
		if not force and self.child:
			# Still executing shell command
			self.remove_me = True
		else:
			if force:
				self.remove_timeout()
				
				if self.child:
					self.close_child()

			SnippetPlaceholderExpand.remove(self, force)
