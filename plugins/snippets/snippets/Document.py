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

import os
import re

import gtk
from gtk import gdk
import gio
import gedit

from Library import Library
from Snippet import Snippet
from Placeholder import *
from SnippetComplete import SnippetComplete

class Document:
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
                
                self.ordered_placeholders = []
                self.update_placeholders = []
                self.jump_placeholders = []
                self.language_id = 0
                self.timeout_update_id = 0
                
                # Always have a reference to the global snippets
                Library().ref(None)
                self.set_view(view)
        
        # Stop controlling the view. Remove all active snippets, remove references
        # to the view and the plugin instance, disconnect all signal handlers
        def stop(self):
                if self.timeout_update_id != 0:
                        gobject.source_remove(self.timeout_update_id)
                        self.timeout_update_id = 0
                        del self.update_placeholders[:]
                        del self.jump_placeholders[:]

                # Always release the reference to the global snippets
                Library().unref(None)
                self.set_view(None)
                self.instance = None
                self.active_placeholder = None

        def disconnect_signal(self, obj, signal):
                if (obj, signal) in self.signal_ids:
                        obj.disconnect(self.signal_ids[(obj, signal)])
                        del self.signal_ids[(obj, signal)]
                
        def connect_signal(self, obj, signal, cb):
                self.disconnect_signal(obj, signal)     
                self.signal_ids[(obj, signal)] = obj.connect(signal, cb)

        def connect_signal_after(self, obj, signal, cb):
                self.disconnect_signal(obj, signal)
                self.signal_ids[(obj, signal)] = obj.connect_after(signal, cb)
                
        # Set the view to be controlled. Installs signal handlers and sets current
        # language. If there is already a view set this function will first remove
        # all currently active snippets and disconnect all current signals. So
        # self.set_view(None) will effectively remove all the control from the
        # current view
        def _set_view(self, view):
                if self.view:
                        buf = self.view.get_buffer()
                        
                        # Remove signals
                        signals = {self.view: ('key-press-event', 'destroy', 
                                               'notify::editable', 'drag-data-received'),
                                   buf:       ('notify::language', 'changed', 'cursor-moved', 'insert-text')}
                        
                        for obj, sig in signals.items():
                                for s in sig:
                                        self.disconnect_signal(obj, s)
                        
                        # Remove all active snippets
                        for snippet in list(self.active_snippets):
                                self.deactivate_snippet(snippet, True)

                self.view = view
                
                if view != None:
                        buf = view.get_buffer()
                       
                        self.connect_signal(view, 'destroy', self.on_view_destroy)

                        if view.get_editable():
                                self.connect_signal(view, 'key-press-event', self.on_view_key_press)

                        self.connect_signal(buf, 'notify::language', self.on_notify_language)
                        self.connect_signal(view, 'notify::editable', self.on_notify_editable)
                        self.connect_signal(view, 'drag-data-received', self.on_drag_data_received)

                        self.update_language()
                elif self.language_id != 0:
                        langid = self.language_id
                        
                        self.language_id = None;
                        
                        if self.instance:
                                self.instance.language_changed(self)

                        Library().unref(langid)

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

                langid = self.language_id

                if lang:
                        self.language_id = lang.get_id()
                else:
                        self.language_id = None

                if self.instance:
                        self.instance.language_changed(self)

                if langid != 0:
                        Library().unref(langid)

                Library().ref(self.language_id)

        def accelerator_activate(self, keyval, mod):
                if not self.view or not self.view.get_editable():
                        return

                accelerator = gtk.accelerator_name(keyval, mod)
                snippets = Library().from_accelerator(accelerator, \
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
                
                self.connect_signal(buf, 'changed', self.on_buffer_changed)
                self.connect_signal(buf, 'cursor-moved', self.on_buffer_cursor_moved)
                self.connect_signal_after(buf, 'insert-text', self.on_buffer_insert_text)
        
        def last_snippet_removed(self):
                buf = self.view.get_buffer()
                self.disconnect_signal(buf, 'changed')
                self.disconnect_signal(buf, 'cursor-moved')
                self.disconnect_signal(buf, 'insert-text')

        def current_placeholder(self):
                buf = self.view.get_buffer()
                
                piter = buf.get_iter_at_mark(buf.get_insert())        
                found = []

                for placeholder in self.placeholders:
                        begin = placeholder.begin_iter()
                        end = placeholder.end_iter()

                        if piter.compare(begin) >= 0 and piter.compare(end) <= 0:
                                found.append(placeholder)

                if self.active_placeholder in found:
                        return self.active_placeholder
                elif len(found) > 0:
                        return found[0]
                else:
                        return None

        def advance_placeholder(self, direction):
                # Returns (CurrentPlaceholder, NextPlaceholder), depending on direction
                buf = self.view.get_buffer()
                
                piter = buf.get_iter_at_mark(buf.get_insert())
                found = current = next = None
                length = len(self.placeholders)
                
                placeholders = list(self.placeholders)
                
                if self.active_placeholder:
                        begin = self.active_placeholder.begin_iter()
                        end = self.active_placeholder.end_iter()
                        
                        if piter.compare(begin) >= 0 and piter.compare(end) <= 0:
                                current = self.active_placeholder
                                currentIndex = placeholders.index(self.active_placeholder)

                if direction == 1:
                        # w = piter, x = begin, y = end, z = found
                        nearest = lambda w, x, y, z: (w.compare(x) <= 0 and (not z or \
                                        x.compare(z.begin_iter()) < 0))
                        indexer = lambda x: x < length - 1
                else:
                        # w = piter, x = begin, y = end, z = prev
                        nearest = lambda w, x, y, z: (w.compare(x) >= 0 and (not z or \
                                        x.compare(z.begin_iter()) >= 0))
                        indexer = lambda x: x > 0

                for index in range(0, length):
                        placeholder = placeholders[index]
                        begin = placeholder.begin_iter()
                        end = placeholder.end_iter()
                        
                        # Find the nearest placeholder
                        if nearest(piter, begin, end, found):
                                foundIndex = index
                                found = placeholder
                        
                        # Find the current placeholder
                        if piter.compare(begin) >= 0 and \
                                        piter.compare(end) <= 0 and \
                                        current == None:
                                currentIndex = index
                                current = placeholder
                
                if current and current != found and \
                   (current.begin_iter().compare(found.begin_iter()) == 0 or \
                    current.end_iter().compare(found.begin_iter()) == 0) and \
                   self.active_placeholder and \
                   current.begin_iter().compare(self.active_placeholder.begin_iter()) == 0:
                        # if current and found are at the same place, then
                        # resolve the 'hugging' problem
                        current = self.active_placeholder
                        currentIndex = placeholders.index(current)

                        found = current
                
                if current and current == found:
                        if indexer(currentIndex):
                                next = placeholders[currentIndex + direction]
                elif found:
                        next = found
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
        
        def set_active_placeholder(self, placeholder):
                self.active_placeholder = placeholder

        def goto_placeholder(self, current, next):
                last = None

                if current:
                        # Signal this placeholder to end action
                        current.leave()
                        
                        if current.__class__ == PlaceholderEnd:
                                last = current
                
                self.set_active_placeholder(next)
                
                if next:
                        next.enter()
                        
                        if next.__class__ == PlaceholderEnd:
                                last = next

                if last:
                        # This is the end of the placeholder, remove the snippet etc
                        for snippet in list(self.active_snippets):
                                if snippet.placeholders[0] == last:
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
                
                variables = {'GEDIT_SELECTED_TEXT': self.env_get_selected_text, 
                             'GEDIT_CURRENT_WORD': self.env_get_current_word, 
                             'GEDIT_FILENAME': self.env_get_filename, 
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
                if current and current.__class__ == PlaceholderEnd:
                        self.goto_placeholder(current, None)
                
                buf.begin_user_action()

                # Remove the tag, selection or current word
                buf.delete(start, end)
                
                # Insert the snippet
                holders = len(self.placeholders)
                
                if len(self.active_snippets) == 0:
                        self.first_snippet_inserted()

                sn = s.insert_into(self, start)
                self.active_snippets.append(sn)

                # Put cursor at first tab placeholder
                keys = filter(lambda x: x > 0, sn.placeholders.keys())
                
                if len(keys) == 0:
                        buf.place_cursor(sn.begin_iter())
                else:
                        self.goto_placeholder(self.active_placeholder, sn.placeholders[keys[0]])
                
                buf.end_user_action()

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
                
                snippets = Library().from_tag(word, self.language_id)
                
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
                
                for tabstop in snippet.placeholders:
                        if tabstop == -1:
                                placeholders = snippet.placeholders[-1]
                        else:
                                placeholders = [snippet.placeholders[tabstop]]
                        
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
                        self.ordered_placeholders.remove(placeholder)

                        placeholder.remove(force)

                snippet.deactivate()
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
                        
                        nodes = Library().get_snippets(None)
                        
                        if self.language_id:
                                nodes += Library().get_snippets(self.language_id)
                        
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
                        if snippet.begin_mark.get_deleted() or snippet.end_mark.get_deleted():
                                self.deactivate(snippet)
                        else:
                                begin = snippet.begin_iter()
                                end = snippet.end_iter()
                        
                                if piter.compare(begin) < 0 or piter.compare(end) > 0:
                                        # Oh no! Remove the snippet this instant!!
                                        self.deactivate_snippet(snippet)
                
                current = self.current_placeholder()
                
                if current != self.active_placeholder:
                        if self.active_placeholder:
                                self.jump_placeholders.append((self.active_placeholder, current))
                                
                                if self.timeout_update_id == 0:
                                        self.timeout_update_id = gobject.timeout_add(0, 
                                                        self.update_snippet_contents)

                        self.set_active_placeholder(current)
        
        def on_buffer_changed(self, buf):
                current = self.current_placeholder()
                
                if current:
                        if not current in self.update_placeholders:
                                self.update_placeholders.append(current)
                
                        if self.timeout_update_id == 0:
                                self.timeout_update_id = gobject.timeout_add(0, \
                                                self.update_snippet_contents)
        
        def on_buffer_insert_text(self, buf, piter, text, length):
                ctx = get_buffer_context(buf)
                
                # do nothing special if there is no context and no active 
                # placeholder
                if (not ctx) and (not self.active_placeholder):
                        return
                
                if not ctx:
                        ctx = self.active_placeholder
                
                if not ctx in self.ordered_placeholders:
                        return
                        
                # move any marks that were incorrectly moved by this insertion
                # back to where they belong
                begin = ctx.begin_iter()
                end = ctx.end_iter()
                idx = self.ordered_placeholders.index(ctx)
                
                for placeholder in self.ordered_placeholders:
                        if placeholder == ctx:
                                continue
                        
                        ob = placeholder.begin_iter()
                        oe = placeholder.end_iter()
                        
                        if ob.compare(begin) == 0 and ((not oe) or oe.compare(end) == 0):
                                oidx = self.ordered_placeholders.index(placeholder)
                                
                                if oidx > idx and ob:
                                        buf.move_mark(placeholder.begin, end)
                                elif oidx < idx and oe:
                                        buf.move_mark(placeholder.end, begin)
                        elif ob.compare(begin) >= 0 and ob.compare(end) < 0 and (oe and oe.compare(end) >= 0):
                                buf.move_mark(placeholder.begin, end)
                        elif (oe and oe.compare(begin) > 0) and ob.compare(begin) <= 0:
                                buf.move_mark(placeholder.end, begin)
        
        def on_notify_language(self, buf, spec):
                self.update_language()

        def on_notify_editable(self, view, spec):
                self._set_view(view)
        
        def on_view_key_press(self, view, event):
                library = Library()

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
        
        def path_split(self, path, components=[]):
                head, tail = os.path.split(path)
                
                if not tail and head:
                        return [head] + components
                elif tail:
                        return self.path_split(head, [tail] + components)
                else:
                        return components
        
        def relative_filename(self, first, second, mime):
                prot1 = re.match('(^[a-z]+:\/\/|\/)(.*)', first)
                prot2 = re.match('(^[a-z]+:\/\/|\/)(.*)', second)
                
                if not prot1 or not prot2:
                        return second
                
                # Different protocols
                if prot1.group(1) != prot2.group(1):
                        return second
                
                # Split on backslash
                path1 = self.path_split(prot1.group(2))
                path2 = self.path_split(prot2.group(2))
                
                # Remove as long as common
                while path1 and path2 and path1[0] == path2[0]:
                        path1.pop(0)
                        path2.pop(0)
                
                # If we need to ../ more than 3 times, then just return
                # the absolute path
                if len(path1) - 1 > 3:
                        return second
                
                if mime.startswith('x-directory'):
                        # directory, special case
                        if not path2:
                                result = './'
                        else:
                                result = '../' * (len(path1) - 1)       
                else:   
                        # Insert ../
                        result = '../' * (len(path1) - 1)
                
                        if not path2:
                                result = os.path.basename(second)
                
                if path2:
                        result += os.path.join(*path2)
                        
                return result
        
        def apply_uri_snippet(self, snippet, mime, uri):
                # Remove file scheme
                if gedit.utils.uri_has_file_scheme(uri):
                        gfile = gio.File(uri)
                        uri = gfile.get_path()
                
                # Set environmental variables
                filename = self.env_get_filename(self.view.get_buffer())
                
                os.environ['GEDIT_DROP_FILENAME'] = uri
                os.environ['GEDIT_DROP_MIME_TYPE'] = mime
                os.environ['GEDIT_DROP_REL_FILENAME'] = self.relative_filename(filename, uri, mime)

                buf = self.view.get_buffer()
                mark = buf.get_mark('gtk_drag_target')
                
                if not mark:
                        mark = buf.get_insert()

                piter = buf.get_iter_at_mark(mark)
                self.apply_snippet(snippet, piter, piter)

        def in_bounds(self, x, y):
                rect = self.view.get_visible_rect()
                rect.x, rect.y = self.view.buffer_to_window_coords(gtk.TEXT_WINDOW_WIDGET, rect.x, rect.y)

                return not (x < rect.x or x > rect.x + rect.width or y < rect.y or y > rect.y + rect.height)
        
        def on_drag_data_received(self, view, context, x, y, data, info, timestamp):   
                if not (gtk.targets_include_uri(context.targets) and data.data and self.in_bounds(x, y)):
                        return

                uris = drop_get_uris(data)
                uris.reverse()
                stop = False
                
                for uri in uris:
                        try:
                                mime = gio.content_type_guess(uri)
                        except:
                                mime = None

                        if not mime:
                                continue
                        
                        snippets = Library().from_drop_target(mime, self.language_id)
                        
                        if snippets:
                                stop = True
                                self.apply_uri_snippet(snippets[0], mime, uri)

                if stop:
                        context.finish(True, False, timestamp)
                        view.stop_emission('drag-data-received')
                        view.get_toplevel().present()
                        view.grab_focus()
        
        def find_uri_target(self, context):
                lst = gtk.target_list_add_uri_targets((), 0)
                
                return self.view.drag_dest_find_target(context, lst)
# ex:ts=8:et:
