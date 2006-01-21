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

import gtksourceview
import gedit
import gtk
from gtk import gdk
import re
import os
import gettext
from gettext import gettext as _
from Snippet import Snippet
from SnippetsDialog import SnippetsDialog
from SnippetPlaceholders import *
from SnippetComplete import SnippetComplete

class SnippetsPluginInstance:
	def __init__(self, plugin):
		self.plugin = plugin
		self.current_view = None
		self.tab_key_val = (gtk.gdk.keyval_from_name('Tab'), \
				gtk.gdk.keyval_from_name('ISO_Left_Tab'))
		self.space_key_val = gtk.gdk.keyval_from_name('space')
		self.placeholders = []
		self.active_snippets = []
		self.cursor_moved_handler = 0
		self.language_accel_group = None
		self.language_accel_group_id = 0
		self.global_accel_group_id = 0
#		gettext.bindtextdomain('gedit', '/opt/gnomecvs/share/locale')
#		gettext.textdomain('gedit')

	def run(self, window):
		self.window = window
		self.insert_menu()
		
		accel = self.plugin.language_accel_group(None)
		accel.connect('accel-activate', self.on_accel_group_activate)
		window.add_accel_group(accel)
		
		self.set_view(window.get_active_view())
	
	def stop(self):
		accel = self.plugin.language_accel_group(None)
		self.window.remove_accel_group(accel)
		
		self.remove_menu()
		self.set_view(None)
		self.window = None
		self.plugin = None
	
	def insert_menu(self):
		manager = self.window.get_ui_manager()

		self.action_group = gtk.ActionGroup("GeditSnippetPluginActions")
		self.action_group.set_translation_domain('gedit')
		self.action_group.add_actions([('Snippets', None,
				_('Manage _Snippets...'), \
				None, _('Manage snippets'), \
				self.on_action_snippets_activate)])

		self.merge_id = manager.new_merge_id()
		manager.insert_action_group(self.action_group, -1)		
		manager.add_ui(self.merge_id, '/MenuBar/ToolsMenu/ToolsOps_5', \
				'Snippets', 'Snippets', gtk.UI_MANAGER_MENUITEM, False)

	def remove_menu(self):
		manager = self.window.get_ui_manager()
		manager.remove_ui(self.merge_id)
		manager.remove_action_group(self.action_group)
		self.action_group = None

	def on_action_snippets_activate(self, item):
		self.plugin.create_configure_dialog()

	def ui_changed(self):
		self.set_view(self.window.get_active_view())
		
	def set_view(self, view):
		if view == self.current_view:
			return

		if self.current_view:
			buf = self.current_view.get_buffer()
			self.current_view.disconnect(self.key_press_id)
			buf.disconnect(self.notify_language)
			
			if self.cursor_moved_handler:
				buf.disconnect(self.cursor_moved_handler)
				self.cursor_moved_handler = 0
			
			# Remove all active snippets
			for snippet in list(self.active_snippets):
				self.deactivate_snippet(snippet, True)
		
		if self.language_accel_group:
			self.window.remove_accel_group(self.language_accel_group)
			self.language_accel_group.disconnect(self.language_accel_group_id)
		
		self.language_accel_group = None
		self.current_view = view
		
		if view != None:
			self.key_press_id = view.connect('key_press_event', \
					self.on_view_key_press, None)
			self.notify_language = view.get_buffer().connect( \
					'notify::language', self.on_notify_language, None)
			self.update_language()
			
	def update_language(self):
		lang = self.current_view.get_buffer().get_language()

		if self.language_accel_group:
			self.window.remove_accel_group(self.language_accel_group)
			self.language_accel_group.disconnect(self.language_accel_group_id)
			
		if lang:
			self.language_name = lang.get_name()
			self.language_accel_group = self.plugin.language_accel_group( \
					self.language_name)
			self.window.add_accel_group(self.language_accel_group)
			self.language_accel_group_id = self.language_accel_group.connect( \
					'accel-activate', self.on_accel_group_activate)
		else:
			self.language_accel_group = None
			self.language_name = None
	
	def find_snippet(self, snippets, tag):
		result = []
		
		for snippet in snippets:
			if Snippet(snippet)['tag'] == tag:
				result.append(snippet)
		
		return result
	
	def next_placeholder(self):
		# Returns (current_placeholder, next_placeholder)
		buf = self.current_view.get_buffer()
		
		# Find out if we are in a placeholder now
		piter = buf.get_iter_at_mark(buf.get_insert())
		prev = current = next = None
		length = len(self.placeholders)

		for index in range(0, length):
			placeholder = self.placeholders[index]
			begin = placeholder.begin_iter()
			end = placeholder.end_iter()

			# Find the nearest previous placeholder
			if piter.compare(end) >= 0 and (not prev or end.compare( \
					prev.end_iter()) >= 0):
				prevIndex = index
				prev = placeholder

			# Find the placeholder where we are now
			if piter.compare(begin) >= 0 and \
					piter.compare(end) <= 0:
				currentIndex = index
				current = placeholder
		
		# Find next placeholder
		if current:
			if currentIndex < length - 1:
				next = self.placeholders[currentIndex + 1]
		elif prev:
			if prevIndex < length - 1:
				next = self.placeholders[prevIndex + 1]
		elif length > 0:
			next = self.placeholders[0]

		return (current, next)
	
	def previous_placeholder(self):
		# Returns (current_placeholder, previous_placeholder)
		buf = self.current_view.get_buffer()
		
		# Find out if we are in a placeholder now
		piter = buf.get_iter_at_mark(buf.get_insert())
		prev = current = next = None
		length = len(self.placeholders)

		for index in range(0, length):
			placeholder = self.placeholders[index]
			begin = placeholder.begin_iter()
			end = placeholder.end_iter()

			# Find the nearest next placeholder
			if piter.compare(begin) <= 0 and (not next or begin.compare( \
					next.begin_iter()) <= 0):
				nextIndex = index
				next = placeholder

			# Find the placeholder where we are now
			if piter.compare(begin) >= 0 and \
					piter.compare(end) <= 0:
				currentIndex = index
				current = placeholder
		
		# Find next placeholder
		if current:
			if currentIndex > 0:
				prev = self.placeholders[currentIndex - 1]
		elif next:
			if nextIndex > 0:
				prev = self.placeholders[nextIndex - 1]
		elif length > 0:
			prev = self.placeholders[0]

		return (current, prev)
	
	def cursor_on_screen(self):
		buf = self.current_view.get_buffer()
		self.current_view.scroll_mark_onscreen(buf.get_insert())
	
	def goto_placeholder(self, current, next):
		last = None

		if current:
			# Signal this placeholder to end action
			current.leave()
			
			if current.__class__ == SnippetPlaceholderEnd:
				last = current
		
		if next:
			next.enter()
			
			if next.__class__ == SnippetPlaceholderEnd:
				last = next

		if last:
			# This is the end of the placeholder, remove the snippet etc
			for snippet in list(self.active_snippets):
				if snippet[2][0] == last:
					self.deactivate_snippet(snippet)
					break
		
		self.cursor_on_screen()
		
		return next != None
	
	def skip_to_next_placeholder(self):
		(current, next) = self.next_placeholder()
		return self.goto_placeholder(current, next)
	
	def skip_to_previous_placeholder(self):
		(current, prev) = self.previous_placeholder()
		return self.goto_placeholder(current, prev)

	def update_environment(self):
		buf = self.current_view.get_buffer()
		bounds = buf.get_selection_bounds()
		
		if bounds:
			os.environ['GEDIT_SELECTED_TEXT'] = buf.get_text(bounds[0], \
					bounds[1])
		else:
			os.environ['GEDIT_SELECTED_TEXT'] = ''
		
		uri = self.current_view.get_buffer().get_uri()
		
		if uri:
			os.environ['GEDIT_FILENAME'] = \
					self.current_view.get_buffer().get_uri_for_display()
			os.environ['GEDIT_BASENAME'] = \
					os.path.basename(uri)
		else:
			os.environ['GEDIT_FILENAME'] = ''
			os.environ['GEDIT_BASENAME'] = ''
		
	def apply_snippet(self, snippet, start = None, end = None):
		buf = self.current_view.get_buffer()
		s = Snippet(snippet)
		
		if not start:
			start = buf.get_iter_at_mark(buf.get_insert())
		
		if not end:
			end = buf.get_iter_at_mark(buf.get_selection_bound())
		
		# Set environmental variables
		self.update_environment()
		
		# You know, we could be in an end placeholder
		(current, next) = self.next_placeholder()
		if current and current.__class__ == SnippetPlaceholderEnd:
			self.goto_placeholder(current, None)
		
		buf.begin_user_action()
		# Remove the tag
		buf.delete(start, end)
		
		# Insert the snippet
		holders = len(self.placeholders)
		active_info = s.insert_into(self)
		self.active_snippets.append(active_info)

		# Put cursor back to beginning of the snippet
		piter = buf.get_iter_at_mark(active_info[0])
		buf.place_cursor(piter)

		# Jump to first placeholder
		(current, next) = self.next_placeholder()
		
		if current:
			self.goto_placeholder(None, current)
		elif next:
			self.goto_placeholder(None, next)		
			
		if not self.cursor_moved_handler:
			self.cursor_moved_handler = buf.connect('cursor_moved', \
					self.on_buffer_cursor_moved)
					
		buf.end_user_action()

	def get_tab_tag(self, buf):
		end = buf.get_iter_at_mark(buf.get_insert())
		start = end.copy()
		
		word = None
		
		if start.backward_word_start():
			# Check if we were at a word start ourselves
			tmp = start.copy()
			tmp.forward_word_end()
			
			if tmp.equal(end):
				word = buf.get_text(start, end)
			else:
				start = end.copy()
		else:
			start = end.copy()
		
		if not word or word == '':
			if start.backward_char():
				word = start.get_char()
				
				if word.isspace():
					return (None, None, None)
			else:
				return (None, None, None)
		
		return (word, start, end)

	def run_snippet(self):	
		if not self.current_view:
			return False
		
		buf = self.current_view.get_buffer()
		
		# get the word preceding the current insertion position
		(word, start, end) = self.get_tab_tag(buf)
		
		if not word:
			return self.skip_to_next_placeholder()
		
		snippet = None
		
		if self.language_name:
			# look in language
			language = self.plugin.lookup_language(self.language_name)

			if language:	
				snippet = self.find_snippet(language.getElementsByTagName( \
						'snippet'), word)
		
		if not snippet:
			# look in all
			snippet = self.find_snippet(self.plugin.language_all, word)
		
		if snippet:
			if len(snippet) == 1:
				self.apply_snippet(snippet[0], start, end)
			else:
				# Do the fancy completion dialog
				self.show_completion(snippet)
				
			return True

		return self.skip_to_next_placeholder()
	
	def deactivate_snippet(self, snippet, force = False):
		buf = self.current_view.get_buffer()
		
		for tabstop in snippet[2]:
			if tabstop == -1:
				placeholders = snippet[2][-1]
			else:
				placeholders = [snippet[2][tabstop]]
			
			for placeholder in placeholders:
				if placeholder in self.placeholders:
					del self.placeholders[self.placeholders.index(placeholder)]

				placeholder.remove(force)
				
		buf.delete_mark(snippet[0])
		buf.delete_mark(snippet[1])
		
		del self.active_snippets[self.active_snippets.index(snippet)]

	def move_completion_window(self, complete, x, y):
		MARGIN = 15
		screen = self.current_view.get_screen()
		
		width = screen.get_width()
		height = screen.get_height()
		
		cw, ch = complete.get_size()
		
		if x + cw > width:
			x = width - cw - MARGIN
		elif x < MARGIN:
			x = MARGIN
		
		if y + ch > height:
			y = height - ch - MARGIN
		elif y < MARGIN:
			y = MARGIN

		complete.move(x, y)

	def show_completion(self, preset = None):
		buf = self.current_view.get_buffer()
		bounds = buf.get_selection_bounds()
		prefix = None
		
		if not bounds and not preset:
			# When there is no text selected and no preset present, find the
			# prefix
			(prefix, start, end) = self.get_tab_tag(buf)
		
		if not prefix:
			# If there is no prefix, than take the insertion point as the end
			end = buf.get_iter_at_mark(buf.get_insert())
		
		if not preset:
			# There is no preset, find all the global snippets and the language
			# specific snippets
			nodes = list(self.plugin.language_all)
			
			if self.language_name:
				lang = self.plugin.lookup_language(self.language_name)
			
				if lang:
					nodes += lang.getElementsByTagName('snippet')
					
			complete = SnippetComplete(nodes, prefix, True)	
		else:
			# There is a preset, so show that preset
			complete = SnippetComplete(preset, None, True)
		
		complete.connect('snippet-activated', self.on_complete_row_activated)
		
		rect = self.current_view.get_iter_location(end)
		win = self.current_view.get_window(gtk.TEXT_WINDOW_TEXT)
		(x, y) = self.current_view.buffer_to_window_coords( \
				gtk.TEXT_WINDOW_TEXT, rect.x + rect.width, rect.y)
		(xor, yor) = win.get_origin()
		
		self.move_completion_window(complete, x + xor, y + yor)		
		complete.run()
		
	# Callbacks
	
	def on_complete_row_activated(self, complete, snippet):
		buf = self.current_view.get_buffer()
		bounds = buf.get_selection_bounds()
		
		if bounds:
			self.apply_snippet(snippet.node, None, None)
		else:
			(word, start, end) = self.get_tab_tag(buf)
			self.apply_snippet(snippet.node, start, end)
	
	def on_buffer_cursor_moved(self, buf):
		piter = buf.get_iter_at_mark(buf.get_insert())

		# Check for all snippets if the cursor is outside its scope
		for snippet in list(self.active_snippets):
			if snippet[0].get_deleted() or snippet[1].get_deleted():
				self.deactivate(snippet)
			else:
				begin = buf.get_iter_at_mark(snippet[0])
				end = buf.get_iter_at_mark(snippet[1])
			
				if piter.compare(begin) < 0 or piter.compare(end) > 0:
					# Oh no! Remove the snippet this instant!!
					self.deactivate_snippet(snippet)
		
		if not self.active_snippets and self.cursor_moved_handler:
			# There are no snippets anymore, remove the cursor_moved handler
			buf.disconnect(self.cursor_moved_handler)
			self.cursor_moved_handler = 0
	
	def on_notify_language(self, buf, spec, userdata):
		self.update_language()
		
	def on_view_key_press(self, view, event, userdata):
		if not (event.state & gdk.CONTROL_MASK) and \
				not (event.state & gdk.MOD1_MASK) and \
				event.keyval in self.tab_key_val:
			if not event.state & gdk.SHIFT_MASK:
				return self.run_snippet()
			else:
				return self.skip_to_previous_placeholder()
		elif (event.state & gdk.CONTROL_MASK) and \
				not (event.state & gdk.MOD1_MASK) and \
				not (event.state & gdk.SHIFT_MASK) and \
				event.keyval == self.space_key_val:
			self.show_completion()
			return True

		return False
	
	def on_accel_group_activate(self, group, window, keyval, mod):
		snippets = self.plugin.language_all
		
		if self.language_name:
			lang = self.plugin.lookup_language(self.language_name)
			
			if lang:
				snippets += lang.getElementsByTagName('snippet')

		snippet = self.plugin.get_snippet_from_accelerator(snippets, keyval, \
				mod)
				
		if snippet:
			if len(snippet) == 1:
				self.apply_snippet(snippet[0])
			else:
				# Do the fancy completion dialog
				self.show_completion(snippet)			
			return True
		
		return False
