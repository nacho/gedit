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

from SnippetPlaceholders import *
import os
from functions import *

class EvalUtilities:
	def __init__(self, view=None):
		self.view = view
		self.namespace = {}
		self.init_namespace()
	
	def init_namespace(self):
		self.namespace['__builtins__'] = __builtins__
		self.namespace['align'] = self.util_align

	def real_len(self, s, tablen = 0):
		if tablen == 0:
			tablen = self.view.get_tabs_width()
		
		return len(s.expandtabs(tablen))

	def util_align(self, items):
		maxlen = []
		tablen = self.view.get_tabs_width()
		
		for row in range(0, len(items)):
			for col in range(0, len(items[row]) - 1):
				if row == 0:
					maxlen.append(0)
				
				items[row][col] += "\t"
				rl = self.real_len(items[row][col], tablen)
				
				if (rl > maxlen[col]):
					maxlen[col] = rl

		result = ''
		
		for row in range(0, len(items)):
			for col in range(0, len(items[row]) - 1):
				item = items[row][col]
				
				result += item + ("\t" * ((maxlen[col] - \
						self.real_len(item, tablen)) / tablen))
			
			result += items[row][len(items[row]) - 1]
			
			if row != len(items) - 1:
				result += "\n"
		
		return result

class Snippet:
	def __init__(self, data):
		self.data = data
	
	def __getitem__(self, prop):
		return self.data[prop]
	
	def __setitem__(self, prop, value):
		self.data[prop] = value
	
	def accelerator_display(self):
		accel = self['accelerator']

		if accel:
			keyval, mod = gtk.accelerator_parse(accel)
			accel = gtk.accelerator_get_label(keyval, mod)
		
		return accel or ''

	def display(self):
		nm = markup_escape(self['description'])
		
		tag = self['tag']
		accel = self.accelerator_display()
		detail = []
		
		if tag and tag != '':
			detail.append(tag)
			
		if accel and accel != '':
			detail.append(accel)
		
		if not detail:
			return nm
		else:
			return nm + ' (<b>' + markup_escape(str.join(', ', detail)) + \
					'</b>)'

	def add_placeholder(self, placeholders, placeholder):
		if placeholders.has_key(placeholder.tabstop):
			if placeholder.tabstop == -1:
				placeholders[-1].append(placeholder)
			else:
				# This would be an error, really....
				None
		elif placeholder.tabstop == -1:
			placeholders[-1] = [placeholder]
		else:
			placeholders[placeholder.tabstop] = placeholder
	
	def updateLastInsert(self, view, index, lastInsert, text, mark, indent):
		if index - lastInsert > 0:
			buf = view.get_buffer()
			piter = buf.get_iter_at_mark(mark)
			
			lines = re.sub('\\\\(.)', '\\1', text[lastInsert:index]).split('\n')
			
			if len(lines) == 1:
				buf.insert(piter, spaces_instead_of_tabs(view, lines[0]))
			else:
				text = lines[0] + '\n'
			
				for i in range(1, len(lines)):
					text += indent + spaces_instead_of_tabs(view, lines[i]) + '\n'

				buf.insert(piter, text[:-1])

			lastInsert = index
			
		return lastInsert
	
	def substitute_environment(self, text, index, m):
		# make sure indentation is preserved
		
		if m.group(1) in os.environ:
			linestart = text.rfind('\n', 0, index)
			indentend = linestart + 1
			
			while text[indentend].isspace() and not text[indentend] == '\n' \
					and not text[indentend] == '\r':
				indentend += 1

			indent = text[linestart + 1:indentend]
			lines = os.environ[m.group(1)].split('\n')
			insert = unicode.join('\n' + unicode(indent), lines)
			text = text[:index] + insert + text[index + \
					len(m.group(0)):]
			
			return (text, index + len(insert))
		else:
			return (text, index + len(m.group(0)))
		
	def parse_variables(self, text, index):
		# Environmental variable substitution
		s = text[index:]
		match = re.match('\\$([A-Z_]+)', s) or re.match('\\${([A-Z_]+)}', s)
				
		if match:
			text, index = self.substitute_environment(text, index, match)
		
		return text, index, match

	def parse_placeholder(self, s):
		result = re.match('\\${([0-9]+)(:((\\\\:|\\\\}|[^:])+?))?}', s) or \
				re.match('\\$([0-9]+)', s)
		
		return result and (result, SnippetPlaceholder)
	
	def parse_shell(self, s):
		result = re.match('`(([0-9]+):)?((\\\\`|[^`])+?)`', s) or \
				re.match('\\$\\((([0-9]+):)?((\\\\\\)|[^`])+?)\\)', s)
		
		return result and (result, SnippetPlaceholderShell)
	
	def parse_eval(self, s):
		result = re.match('\\$<(([0-9, ]+):)?((\\\\>|[^>])+?)>', s)
		
		return result and (result, SnippetPlaceholderEval)

	def parse_text(self, view, marks):
		placeholders = {}
		lastInsert = 0
		index = 0
		text = self['text']
		buf = view.get_buffer()

		indent = compute_indentation(view, \
				view.get_buffer().get_iter_at_mark(marks[1]))

		utils = EvalUtilities(view)

		while index < len(text):
			c = text[index]
			match = None
			s = text[index:]	
					
			if c == '\\':
				# Skip escapements
				index += 1
			elif c == '`':
				match = self.parse_shell(s)
			elif c == '$':
				text, index, handled = self.parse_variables(text, index)
				
				if handled:
					continue

				match = self.parse_placeholder(s) or \
						self.parse_shell(s) or \
						self.parse_eval(s)

			if match:
				if index != lastInsert or index == 0:
					self.updateLastInsert(view, index, lastInsert, \
							text, marks[1], indent)
					begin = buf.get_iter_at_mark(marks[1])

					if match[1] == SnippetPlaceholderEval:
						refs = (match[0].group(2) and \
								match[0].group(2).split(','))
						placeholder = SnippetPlaceholderEval(view, \
								refs, begin, match[0].group(3), \
								utils.namespace)
					elif match[1] == SnippetPlaceholderShell:
						tabstop = (match[0].group(2) and \
								int(match[0].group(2))) or -1
						placeholder = SnippetPlaceholderShell(view, \
								tabstop, begin, match[0].group(3))						
					else:
						tabstop = int(match[0].group(1))
						default = match[0].lastindex >= 2 and \
								re.sub('\\\\(.)', '\\1', match[0].group(3))
						
						if tabstop == 0:
							# End placeholder
							placeholder = SnippetPlaceholderEnd(view, begin,
									default)
						elif placeholders.has_key(tabstop):
							# Mirror placeholder
							placeholder = SnippetPlaceholderMirror(view, \
									tabstop, begin)
						else:
							# Default placeholder
							placeholder = SnippetPlaceholder(view, tabstop, \
									default, begin)
						
					
					self.add_placeholder(placeholders, placeholder)
					index += len(match[0].group(0)) - 1
					lastInsert = index + 1
					
			index += 1
		
		lastInsert = self.updateLastInsert(view, index, lastInsert, text, \
				marks[1], indent)

		# Create end placeholder if there isn't one yet
		if not placeholders.has_key(0):
			placeholders[0] = SnippetPlaceholderEnd(view, \
					buf.get_iter_at_mark(marks[1]), None)

		# Make sure run_last is ran for all placeholders and remove any
		# non `ok` placeholders
		for tabstop in placeholders.copy():
			if tabstop == -1: # anonymous
				ph = list(placeholders[-1])
			else:
				ph = [placeholders[tabstop]]
			
			for placeholder in ph:
				placeholder.run_last(placeholders)
				
				if not placeholder.ok:
					if placeholder.default:
						buf.delete(placeholder.begin_iter(), \
								placeholder.end_iter())
					
					placeholder.remove()

					if placeholder.tabstop == -1:
						del placeholders[-1][placeholders[-1].index(\
								placeholder)]
					else:
						del placeholders[placeholder.tabstop]
		
		# Remove all the Expand placeholders which have a tabstop because
		# they can be used to mirror, but they shouldn't be real tabstops
		# This is problably a bit of a dirty hack :)
		if not placeholders.has_key(-1):
			placeholders[-1] = []

		for tabstop in placeholders.copy():
			if tabstop != -1:
				if isinstance(placeholders[tabstop], SnippetPlaceholderExpand):
					placeholders[-1].append(placeholders[tabstop])
					del placeholders[tabstop]
		
		return placeholders
	
	def insert_into(self, plugin_data):
		buf = plugin_data.view.get_buffer()
		insert = buf.get_iter_at_mark(buf.get_insert())
		lastIndex = 0
		
		# Find closest mark at current insertion, so that we may insert
		# our marks in the correct order
		(current, next) = plugin_data.next_placeholder()
		
		if current:
			# Insert AFTER current
			lastIndex = plugin_data.placeholders.index(current) + 1
		elif next:
			# Insert BEFORE next
			lastIndex = plugin_data.placeholders.index(next)
		else:
			# Insert at first position
			lastIndex = 0
		
		# lastIndex now contains the position of the last mark
		# Create snippet bounding marks
		marks = (buf.create_mark(None, insert, True), \
				buf.create_mark(None, insert, False), \
				buf.create_mark(None, insert, False))
		
		# Now parse the contents of this snippet, create SnippetPlaceholders
		# and insert the placholder marks in the marks array of plugin_data
		placeholders = self.parse_text(plugin_data.view, marks)
		
		# So now all of the snippet is in the buffer, we have all our 
		# placeholders right here, what's next, put all marks in the 
		# plugin_data.marks
		k = placeholders.keys()
		k.sort(reverse=True)
		
		plugin_data.placeholders.insert(lastIndex, placeholders[0])
		lastIter = placeholders[0].end_iter()
		
		for tabstop in k:
			if tabstop != -1 and tabstop != 0:
				placeholder = placeholders[tabstop]
				endIter = placeholder.end_iter()
				
				if lastIter.compare(endIter) < 0:
					lastIter = endIter
				
				# Inserting placeholder
				plugin_data.placeholders.insert(lastIndex, placeholder)
		
		# Move end mark to last placeholder
		buf.move_mark(marks[1], lastIter)
		
		return (marks[0], marks[1], marks[2], placeholders)
