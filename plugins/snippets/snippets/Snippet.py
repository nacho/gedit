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

from xml.dom import minidom
import xml
from SnippetPlaceholders import *
import os
from functions import *

class Snippet:
	def __init__(self, node, plugin = None):
		self.node = node
		self.properties = {}
		
		self.initSnippetData(plugin)
	
	def initSnippetData(self, p):
		props = {'tag': '', 'text': '', 'description': 'New snippet', 
				'accelerator': ''}
		has_props = {}
		
		for child in self.node.childNodes:
			for prop in props:
				if child.localName == prop:
					has_props[prop] = child
				
					if child.hasChildNodes():
						self.properties[prop] = child.firstChild

		if p:
			for prop in props:
				if not prop in has_props:
					prop_node = p.xmldoc.createElement(prop)
					self.node.appendChild(prop_node)
				else:
					prop_node = has_props[prop]

				if not prop in self.properties:
					text_node = p.xmldoc.createTextNode(props[prop])
					self.properties[prop] = text_node
					prop_node.appendChild(text_node)

				if self.properties[prop].data == '':
					self.properties[prop].data = props[prop]
	
	def __getitem__(self, prop):
		if prop in self.properties:
			return self.properties[prop].data
		
		return ''
	
	def __setitem__(self, prop, value):
		if prop in self.properties:
			self.properties[prop].data = value
	
	def display(self):
		nm = markup_escape(self['description'])
		
		tag = self['tag']
		accel = self['accelerator']
		detail = []
		
		if tag:
			detail.append(tag)
			
		if accel:
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
				buf.insert(piter, lines[0])
			else:
				text = lines[0] + '\n'
			
				for i in range(1, len(lines)):
					text += indent + lines[i] + '\n'
			
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
		
	def parse_text(self, view, marks):
		placeholders = {}
		text = self['text']
		lastInsert = 0
		index = 0
		buf = view.get_buffer()

		indent = compute_indentation(view, \
				view.get_buffer().get_iter_at_mark(marks[1]))

		while index < len(text):
			c = text[index]
			
			if c == '\\':
				# Skip escapements
				index += 1
			#elif c == '`':
			#	m = re.match('`((\\\\`|[^`])+?)`', text[index:])
			#	
			#	if (m):
			#		# We found a shell thing!
			#		self.updateLastInsert(view, index, lastInsert, \
			#				text, marks[1], indent)
			#	
			#		begin = buf.get_iter_at_mark(marks[1])
			#		placeholder = SnippetPlaceholderShell(view, begin, \
			#				m.group(1))
			#		self.add_placeholder(placeholders, placeholder)
			#		
			#		index += len(m.group(0))
			#		lastInsert = index
			elif c == '$':
				# Environmental variable substitution
				m = re.match('\\$([A-Z_]+)', text[index:])
				
				if m:
					text, index = self.substitute_environment(text, index, m)
					continue
				
				m = re.match('\\${([A-Z_]+)}', text[index:])
				
				if m:
					text, index = self.substitute_environment(text, index, m)
					continue
					
				ev = False
				m = re.match('\\${([0-9]+)(:((\\\\:|\\\\}|[^:])+?))?}', \
						text[index:])
				
				if not m:
					m = re.match('\\$([0-9]+)', text[index:])
				
				#if not m:
				#	ev = True
				#	m = re.match('\\$<((\\\\>|[^>])+?)>', text[index:])

				if (m):
					# Placeholders cannot appear directly after eachother, so
					# ignore it if index == lastInsert
					if index != lastInsert or index == 0:
						self.updateLastInsert(view, index, lastInsert, \
								text, marks[1], indent)
						begin = buf.get_iter_at_mark(marks[1])
					
						if ev:
							# Python eval
							placeholder = SnippetPlaceholderEval(view, begin, \
									m.group(1))
						else:
							tabstop = int(m.group(1))

							if m.lastindex >= 2:
								default = m.group(3)
							else:
								default = None
						
							if default:
								default = re.sub('\\\\(.)', '\\1', default)
	
							if tabstop == 0:
								# End placeholder
								placeholder = SnippetPlaceholderEnd(view, begin, default)
							elif placeholders.has_key(tabstop):
								# this is a mirror
								placeholder = SnippetPlaceholderMirror(view, \
										tabstop, begin)
							else:
								# this is a first time
								placeholder = SnippetPlaceholder(view, tabstop, \
										default, begin)
					
						self.add_placeholder(placeholders, placeholder)

					index += len(m.group(0)) - 1
					lastInsert = index + 1
					
			index += 1
		
		lastInsert = self.updateLastInsert(view, index, lastInsert, text, \
				marks[1], indent)

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
		
		return placeholders
	
	def insert_into(self, plugin_data):
		buf = plugin_data.current_view.get_buffer()
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
				buf.create_mark(None, insert, False))
		
		# Now parse the contents of this snippet, create SnippetPlaceholders
		# and insert the placholder marks in the marks array of plugin_data
		placeholders = self.parse_text(plugin_data.current_view, marks)
		
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
					print tabstop
					lastIter = endIter
				
				# Inserting placeholder
				plugin_data.placeholders.insert(lastIndex, placeholder)
		
		# Move end mark to last placeholder
		buf.move_mark(marks[1], lastIter)
		
		return (marks[0], marks[1], placeholders)
