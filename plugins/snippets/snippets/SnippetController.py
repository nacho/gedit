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

import gtk
from SnippetsLibrary import SnippetsLibrary
from gtk import gdk
from Snippet import Snippet
import os
from SnippetPlaceholders import *
from SnippetComplete import SnippetComplete

class SnippetController:
        TAB_KEY_VAL = (gtk.keysyms.Tab, \
                        gtk.keysyms.ISO_Left_Tab)
        SPACE_KEY_VAL = (gtk.keysyms.space,)
        
        def __init__(self, instance, view):
                self.view = None
                self.instance = instance
                
                self.placeholders = []
                self.active_snippets = []
                self.active_placeholder = None
                self.signal_ids = {}
                
                self.update_placeholders = []
                self.jump_placeholders = []
                self.language_id = 0
                self.timeout_update_id = 0
                
                self.set_view(view)
        
        # Stop controlling the view. Remove all active snippets, remove references
        # to the view and the plugin instance, disconnect all signal handlers
        def stop(self):
                if self.timeout_update_id != 0:
                        gobject.source_remove(self.timeout_update_id)
                        self.timeout_update_id = 0
                        del self.update_placeholders[:]
                        del self.jump_placeholders[:]

                SnippetsLibrary().unref(None)
                self.set_view(None)
                self.instance = None
                self.active_placeholder = None

        def disconnect_signal(self, obj, signal):
                if signal in self.signal_ids:
                        obj.disconnect(self.signal_ids[signal])
                        del self.signal_ids[signal]

        # Set the view to be controlled. Installs signal handlers and sets current
        # language. If there is already a view set this function will first remove
        # all currently active snippets and disconnect all current signals. So
        # self.set_view(None) will effectively remove all the control from the
        # current view
        def _set_view(self, view):
                if self.view:
                        buf = self.view.get_buffer()
                        
                        self.disconnect_signal(self.view, 'key-press-event')
                        self.disconnect_signal(self.view, 'destroy')
                        self.disconnect_signal(buf, 'notify::language')
                        self.disconnect_signal(self.view, 'notify::editable')
                        self.disconnect_signal(buf, 'changed')
                        self.disconnect_signal(buf, 'cursor-moved')
                        
                        # Remove all active snippets
                        for snippet in list(self.active_snippets):
                                self.deactivate_snippet(snippet, True)

                self.view = view
                
                if view != None:
                        buf = view.get_buffer()
                        
                        self.signal_ids['destroy'] = view.connect('destroy', \
                                        self.on_view_destroy)

                        if view.get_editable():
                                self.signal_ids['key-press-event'] = view.connect( \
                                                'key_press_event', self.on_view_key_press)

                        self.signal_ids['notify::language'] = buf.connect( \
                                        'notify::language', self.on_notify_language)
                        self.signal_ids['notify::editable'] = view.connect( \
                                        'notify::editable', self.on_notify_editable)
                        
                        self.update_language()
                elif self.language_id != 0:
                        SnippetsLibrary().unref(self.language_id)
        
        def set_view(self, view):
                if view == self.view:
                        return
                
                self._set_view(view)

        # Call this whenever the language in the view changes. This makes sure that
        # the correct language is used when finding snippets
        def update_language(self):
                lang = self.view.get_buffer().get_language()

                if lang == None and self.language_id == None:
                        return
                elif lang and lang.get_id() == self.language_id:
                        return

                if self.language_id != 0:
                        SnippetsLibrary().unref(self.language_id)

                if lang:
                        self.language_id = lang.get_id()
                else:
                        self.language_id = None

                self.instance.language_changed(self)
                SnippetsLibrary().ref(self.language_id)

        def accelerator_activate(self, keyval, mod):
                if not self.view or not self.view.get_editable():
                        return

                accelerator = gtk.accelerator_name(keyval, mod)
                snippets = SnippetsLibrary().from_accelerator(accelerator, \
                                self.language_id)

                snippets_debug('Accel!')

                if len(snippets) == 0:
                        return False
                elif len(snippets) == 1:
                        self.apply_snippet(snippets[0])
                else:
                        # Do the fancy completion dialog
                        return self.show_completion(snippets)

                return True

        def first_snippet_inserted(self):
                buf = self.view.get_buffer()
                
                self.signal_ids['changed'] = buf.connect('changed', \
                                self.on_buffer_changed)
                self.signal_ids['cursor-moved'] = buf.connect('cursor_moved', \
                                        self.on_buffer_cursor_moved)
        
        def last_snippet_removed(self):
                buf = self.view.get_buffer()
                self.disconnect_signal(buf, 'changed')
                self.disconnect_signal(buf, 'cursor-moved')

        def current_placeholder(self):
                buf = self.view.get_buffer()
                
                piter = buf.get_iter_at_mark(buf.get_insert())	
                current = None

                for placeholder in self.placeholders:
                        begin = placeholder.begin_iter()
                        end = placeholder.end_iter()

                        if piter.compare(begin) >= 0 and \
                                        piter.compare(end) <= 0:
                                current = placeholder

                return current

        def advance_placeholder(self, direction):
                # Returns (CurrentPlaceholder, NextPlaceholder), depending on direction
                buf = self.view.get_buffer()
                
                piter = buf.get_iter_at_mark(buf.get_insert())
                prev = current = next = None
                length = len(self.placeholders)		

                if direction == 1:
                        nearest = lambda w, x, y, z: (w.compare(y) >= 0 and (not z or \
                                        y.compare(z.end_iter()) >= 0))
                        indexer = lambda x: x < length - 1
                else:
                        nearest = lambda w, x, y, z: (w.compare(x) <= 0 and (not z or \
                                        x.compare(z.end_iter()) <= 0))
                        indexer = lambda x: x > 0

                for index in range(0, length):
                        placeholder = self.placeholders[index]
                        begin = placeholder.begin_iter()
                        end = placeholder.end_iter()
                        
                        # Find the nearest placeholder
                        if nearest(piter, begin, end, prev):
                                prevIndex = index
                                prev = placeholder
                        
                        # Find the current placeholder
                        if piter.compare(begin) >= 0 and \
                                        piter.compare(end) <= 0:
                                currentIndex = index
                                current = placeholder
                
                if current:
                        if indexer(currentIndex):
                                next = self.placeholders[currentIndex + direction]
                elif prev:
                        if indexer(prevIndex):
                                next = self.placeholders[prevIndex + direction]
                elif length > 0:
                        next = self.placeholders[0]
                
                return current, next
        
        def next_placeholder(self):
                return self.advance_placeholder(1)
        
        def previous_placeholder(self):
                return self.advance_placeholder(-1)

        def cursor_on_screen(self):
                buf = self.view.get_buffer()
                self.view.scroll_mark_onscreen(buf.get_insert())
        
        def goto_placeholder(self, current, next):
                last = None

                if current:
                        # Signal this placeholder to end action
                        current.leave()
                        
                        if current.__class__ == SnippetPlaceholderEnd:
                                last = current
                
                self.active_placeholder = next
                
                if next:
                        next.enter()
                        
                        if next.__class__ == SnippetPlaceholderEnd:
                                last = next

                if last:
                        # This is the end of the placeholder, remove the snippet etc
                        for snippet in list(self.active_snippets):
                                if snippet[3][0] == last:
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

        def env_get_selected_text(self, buf):
                bounds = buf.get_selection_bounds()

                if bounds:
                        return buf.get_text(bounds[0], bounds[1])
                else:
                        return ''

        def env_get_current_word(self, buf):
                start, end = buffer_word_boundary(buf)
                
                return buf.get_text(start, end)
                
        def env_get_filename(self, buf):
                uri = buf.get_uri()
                
                if uri:
                        return buf.get_uri_for_display()
                else:
                        return ''
        
        def env_get_basename(self, buf):
                uri = buf.get_uri()
                
                if uri:
                        return os.path.basename(buf.get_uri_for_display())
                else:
                        return ''

        def update_environment(self):
                buf = self.view.get_buffer()
                
                variables = {'GEDIT_SELECTED_TEXT': self.env_get_selected_text, \
                                'GEDIT_CURRENT_WORD': self.env_get_current_word, \
                                'GEDIT_FILENAME': self.env_get_filename, \
                                'GEDIT_BASENAME': self.env_get_basename}
                
                for var in variables:
                        os.environ[var] = variables[var](buf)
        
        def uses_current_word(self, snippet):
                matches = re.findall('(\\\\*)\\$GEDIT_CURRENT_WORD', snippet['text'])
                
                for match in matches:
                        if len(match) % 2 == 0:
                                return True
                
                return False

        def apply_snippet(self, snippet, start = None, end = None):
                if not snippet.valid:
                        return False

                buf = self.view.get_buffer()
                s = Snippet(snippet)
                
                if not start:
                        start = buf.get_iter_at_mark(buf.get_insert())
                
                if not end:
                        end = buf.get_iter_at_mark(buf.get_selection_bound())

                if start.equal(end) and self.uses_current_word(s):
                        # There is no tab trigger and no selection and the snippet uses
                        # the current word. Set start and end to the word boundary so that 
                        # it will be removed
                        start, end = buffer_word_boundary(buf)

                # Set environmental variables
                self.update_environment()
                
                # You know, we could be in an end placeholder
                (current, next) = self.next_placeholder()
                if current and current.__class__ == SnippetPlaceholderEnd:
                        self.goto_placeholder(current, None)
                
                buf.begin_user_action()

                # Remove the tag, selection or current word
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
                
                if current and current != self.active_placeholder:
                        self.goto_placeholder(None, current)
                elif next:
                        self.goto_placeholder(None, next)		
                        
                buf.end_user_action()
                
                if len(self.active_snippets) == 1:
                        self.first_snippet_inserted()

                return True

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

                                if word.isalnum() or word.isspace():
                                        return (None, None, None)
                        else:
                                return (None, None, None)
                
                return (word, start, end)

        def run_snippet(self):	
                if not self.view:
                        return False
                
                buf = self.view.get_buffer()
                
                # get the word preceding the current insertion position
                (word, start, end) = self.get_tab_tag(buf)
                
                if not word:
                        return self.skip_to_next_placeholder()
                
                snippets = SnippetsLibrary().from_tag(word, self.language_id)
                
                if snippets:
                        if len(snippets) == 1:
                                return self.apply_snippet(snippets[0], start, end)
                        else:
                                # Do the fancy completion dialog
                                return self.show_completion(snippets)

                return self.skip_to_next_placeholder()
        
        def deactivate_snippet(self, snippet, force = False):
                buf = self.view.get_buffer()
                remove = []
                
                for tabstop in snippet[3]:
                        if tabstop == -1:
                                placeholders = snippet[3][-1]
                        else:
                                placeholders = [snippet[3][tabstop]]
                        
                        for placeholder in placeholders:
                                if placeholder in self.placeholders:
                                        if placeholder in self.update_placeholders:
                                                placeholder.update_contents()
                                                
                                                self.update_placeholders.remove(placeholder)
                                        elif placeholder in self.jump_placeholders:
                                                placeholder[0].leave()
                                                
                                        remove.append(placeholder)
                
                for placeholder in remove:
                        if placeholder == self.active_placeholder:
                                self.active_placeholder = None

                        self.placeholders.remove(placeholder)
                        placeholder.remove(force)

                buf.delete_mark(snippet[0])
                buf.delete_mark(snippet[1])
                buf.delete_mark(snippet[2])
                
                self.active_snippets.remove(snippet)
                
                if len(self.active_snippets) == 0:
                        self.last_snippet_removed()

        # Moves the completion window to a suitable place honoring the hint given
        # by x and y. It tries to position the window so it's always visible on the
        # screen.
        def move_completion_window(self, complete, x, y):
                MARGIN = 15
                screen = self.view.get_screen()
                
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

        # Show completion, shows a completion dialog in the view.
        # If preset is not None then a completion dialog is shown with the snippets
        # in the preset list. Otherwise it will try to find the word preceding the
        # current cursor position. If such a word is found, it is taken as a 
        # tab trigger prefix so that only snippets with a tab trigger prefixed with
        # the word are in the list. If no such word can be found than all snippets
        # are shown.
        def show_completion(self, preset = None):
                buf = self.view.get_buffer()
                bounds = buf.get_selection_bounds()
                prefix = None
                
                if not bounds and not preset:
                        # When there is no text selected and no preset present, find the
                        # prefix
                        (prefix, start, end) = self.get_tab_tag(buf)
                
                if not prefix:
                        # If there is no prefix, than take the insertion point as the end
                        end = buf.get_iter_at_mark(buf.get_insert())
                
                if not preset or len(preset) == 0:
                        # There is no preset, find all the global snippets and the language
                        # specific snippets
                        
                        nodes = SnippetsLibrary().get_snippets(None)
                        
                        if self.language_id:
                                nodes += SnippetsLibrary().get_snippets(self.language_id)
                        
                        if prefix and len(prefix) == 1 and not prefix.isalnum():
                                hasnodes = False
                                
                                for node in nodes:
                                        if node['tag'] and node['tag'].startswith(prefix):
                                                hasnodes = True
                                                break
                                
                                if not hasnodes:
                                        prefix = None
                        
                        complete = SnippetComplete(nodes, prefix, False)	
                else:
                        # There is a preset, so show that preset
                        complete = SnippetComplete(preset, None, True)
                
                complete.connect('snippet-activated', self.on_complete_row_activated)
                
                rect = self.view.get_iter_location(end)
                win = self.view.get_window(gtk.TEXT_WINDOW_TEXT)
                (x, y) = self.view.buffer_to_window_coords( \
                                gtk.TEXT_WINDOW_TEXT, rect.x + rect.width, rect.y)
                (xor, yor) = win.get_origin()
                
                self.move_completion_window(complete, x + xor, y + yor)		
                return complete.run()

        def update_snippet_contents(self):
                self.timeout_update_id = 0
                
                for placeholder in self.update_placeholders:
                        placeholder.update_contents()
                
                for placeholder in self.jump_placeholders:
                        self.goto_placeholder(placeholder[0], placeholder[1])
                
                del self.update_placeholders[:]
                del self.jump_placeholders[:]
                
                return False

        # Callbacks
        def on_view_destroy(self, view):
                self.stop()
                return

        def on_complete_row_activated(self, complete, snippet):
                buf = self.view.get_buffer()
                bounds = buf.get_selection_bounds()
                
                if bounds:
                        self.apply_snippet(snippet.data, None, None)
                else:
                        (word, start, end) = self.get_tab_tag(buf)
                        self.apply_snippet(snippet.data, start, end)

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
                
                current = self.current_placeholder()
                
                if current != self.active_placeholder:
                        if self.active_placeholder:
                                self.jump_placeholders.append((self.active_placeholder, current))
                                
                                if self.timeout_update_id != 0:
                                        gobject.source_remove(self.timeout_update_id)
                                
                                self.timeout_update_id = gobject.timeout_add(0, 
                                                self.update_snippet_contents)

                        self.active_placeholder = current
        
        def on_buffer_changed(self, buf):
                current = self.current_placeholder()
                
                if current:
                        self.update_placeholders.append(current)
                
                        if self.timeout_update_id != 0:
                                gobject.source_remove(self.timeout_update_id)

                        self.timeout_update_id = gobject.timeout_add(0, \
                                        self.update_snippet_contents)
        
        def on_notify_language(self, buf, spec):
                self.update_language()

        def on_notify_editable(self, view, spec):
                self._set_view(view)
        
        def on_view_key_press(self, view, event):
                library = SnippetsLibrary()

                if not (event.state & gdk.CONTROL_MASK) and \
                                not (event.state & gdk.MOD1_MASK) and \
                                event.keyval in self.TAB_KEY_VAL:
                        if not event.state & gdk.SHIFT_MASK:
                                return self.run_snippet()
                        else:
                                return self.skip_to_previous_placeholder()
                elif (event.state & gdk.CONTROL_MASK) and \
                                not (event.state & gdk.MOD1_MASK) and \
                                not (event.state & gdk.SHIFT_MASK) and \
                                event.keyval in self.SPACE_KEY_VAL:
                        return self.show_completion()
                elif not library.loaded and \
                                library.valid_accelerator(event.keyval, event.state):
                        library.ensure_files()
                        library.ensure(self.language_id)
                        self.accelerator_activate(event.keyval, \
                                        event.state & gtk.accelerator_get_default_mod_mask())

                return False
