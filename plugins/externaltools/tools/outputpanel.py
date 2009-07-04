# -*- coding: utf-8 -*-
#    Gedit External Tools plugin
#    Copyright (C) 2005-2006  Steve Frécinaux <steve@istique.net>
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

__all__ = ('OutputPanel', 'UniqueById')

import gtk, gedit
import pango
import gobject
import os
from weakref import WeakKeyDictionary
from capture import *
from gtk import gdk
import re
import gio

class UniqueById:
    __shared_state = WeakKeyDictionary()

    def __init__(self, i):
        if i in self.__class__.__shared_state:
            self.__dict__ = self.__class__.__shared_state[i]
            return True
        else:
            self.__class__.__shared_state[i] = self.__dict__
            return False

    def states(self):
        return self.__class__.__shared_state

class OutputPanel(UniqueById):
    def __init__(self, datadir, window):
        if UniqueById.__init__(self, window):
            return

        callbacks = {
            'on_stop_clicked' : self.on_stop_clicked,
            'on_view_visibility_notify_event': self.on_view_visibility_notify_event,
            'on_view_motion_notify_event': self.on_view_motion_notify_event,
            'on_view_button_press_event': self.on_view_button_press_event
        }

        self.window = window
        self.ui = gtk.Builder()
        self.ui.add_from_file(os.path.join(datadir, 'ui', 'outputpanel.ui'))
        self.ui.connect_signals(callbacks)

        self.panel = self["output-panel"]
        self['view'].modify_font(pango.FontDescription('Monospace'))

        buffer = self['view'].get_buffer()
        
        self.normal_tag = buffer.create_tag("normal")
        
        self.error_tag = buffer.create_tag("error")
        self.error_tag.set_property("foreground", "red")
        
        self.italic_tag = buffer.create_tag('italic')
        self.italic_tag.set_property('style', pango.STYLE_OBLIQUE)
        
        self.bold_tag = buffer.create_tag('bold')
        self.bold_tag.set_property('weight', pango.WEIGHT_BOLD)
        
        self.link_tag = buffer.create_tag('link')
        self.link_tag.set_property('underline', pango.UNDERLINE_LOW)
        
        self.line_tag = buffer.create_tag('line')
        
        self.link_cursor = gdk.Cursor(gdk.HAND2)
        self.normal_cursor = gdk.Cursor(gdk.XTERM)
        
        self.link_regex = re.compile('((\\./|\\.\\./|/)[^\s:]+|[^\s:]+\\.[^\s:]+)(:([0-9]+))?')

        self.process = None

    def set_process(self, process):
        self.process = process
        self.cwd = process.cwd

    def __getitem__(self, key):
        # Convenience function to get an object from its name
        return self.ui.get_object(key)

    def on_stop_clicked(self, widget, *args):
        if self.process is not None:
            self.write("\n" + _('Stopped.') + "\n",
                       self.italic_tag)
            self.process.stop(-1)

    def scroll_to_end(self):
        iter = self['view'].get_buffer().get_end_iter()
        self['view'].scroll_to_iter(iter, 0.0)
        return False  # don't requeue this handler

    def clear(self):
        self['view'].get_buffer().set_text("")
    
    def visible(self):
        panel = self.window.get_bottom_panel()
        return panel.props.visible and panel.item_is_active(self.panel)

    def write(self, text, tag = None):
        buffer = self['view'].get_buffer()
        
        end_iter = buffer.get_end_iter()
        insert = buffer.create_mark(None, end_iter, True)

        if tag is None:
            buffer.insert(end_iter, text)
        else:
            buffer.insert_with_tags(end_iter, text, tag)
        
        for m in self.link_regex.finditer(text):
            start = buffer.get_iter_at_mark(insert)
            start.forward_chars(m.start(0))
            end = start.copy()
            end.forward_chars(m.end(0))
            
            filename = m.group(1)
            
            if (os.path.isabs(filename) and os.path.isfile(filename)) or \
               (os.path.isfile(os.path.join(self.cwd, filename))):
                buffer.apply_tag(self.link_tag, start, end)
                
                if m.group(4):
                    start = buffer.get_iter_at_mark(insert)
                    start.forward_chars(m.start(4))
                    end = start.copy()
                    end.forward_chars(m.end(4))
            
                    buffer.apply_tag(self.line_tag, start, end)
        
        buffer.delete_mark(insert)
        gobject.idle_add(self.scroll_to_end)

    def show(self):
        panel = self.window.get_bottom_panel()
        panel.show()
        panel.activate_item(self.panel)
    
    def update_cursor_style(self, view, x, y):
        x, y = view.window_to_buffer_coords(gtk.TEXT_WINDOW_TEXT, int(x), int(y))
        piter = view.get_iter_at_location(x, y)
        
        if piter.has_tag(self.link_tag):
            cursor = self.link_cursor
        else:
            cursor = self.normal_cursor

        view.get_window(gtk.TEXT_WINDOW_TEXT).set_cursor(cursor)
    
    def on_view_motion_notify_event(self, view, event):
        if event.window == view.get_window(gtk.TEXT_WINDOW_TEXT):
            self.update_cursor_style(view, event.x, event.y)

        return False

    def on_view_visibility_notify_event(self, view, event):
        if event.window == view.get_window(gtk.TEXT_WINDOW_TEXT):
            x, y, m = event.window.get_pointer()
            self.update_cursor_style(view, x, y)

        return False
    
    def idle_grab_focus(self):
        self.window.get_active_view().grab_focus()
        return False
    
    def on_view_button_press_event(self, view, event):
        if event.button != 1 or event.type != gdk.BUTTON_PRESS or \
           event.window != view.get_window(gtk.TEXT_WINDOW_TEXT):
            return False
        
        x, y = view.window_to_buffer_coords(gtk.TEXT_WINDOW_TEXT, int(event.x), int(event.y))
        
        start = view.get_iter_at_location(x, y)

        if not start.has_tag(self.link_tag):
            return False
        
        end = start.copy()
        start.backward_to_tag_toggle(self.link_tag)
        line = start.copy()
        
        end.forward_to_tag_toggle(self.link_tag)
        line.forward_to_tag_toggle(self.line_tag)

        if line.compare(end) < 0:
            tot = line.copy()
            tot.backward_char()
            
            text = start.get_text(tot)
            toline = int(line.get_text(end))
        else:
            text = start.get_text(end)
            toline = 0

        gfile = None
        
        if os.path.isabs(text) and os.path.isfile(text):
            gfile = gio.File(text)
        elif os.path.isfile(os.path.join(self.cwd, text)):
            gfile = gio.File(os.path.join(self.cwd, text))
            
        if gfile:
            gedit.commands.load_uri(self.window, gfile.get_uri(), None, toline)
            
            gobject.idle_add(self.idle_grab_focus)

# ex:ts=4:et:
