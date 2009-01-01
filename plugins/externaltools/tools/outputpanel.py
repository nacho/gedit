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
            'on_stop_clicked' : self.on_stop_clicked
        }

        self.window = window
        self.ui = gtk.Builder()
        self.ui.add_from_file(os.path.join(datadir, 'ui', 'outputpanel.ui'))
        self.ui.connect_signals(callbacks)

        self.panel = self["output-panel"]
        self['view'].modify_font(pango.FontDescription('Monospace'))

        buffer = self['view'].get_buffer()
        self.normal_tag = buffer.create_tag("normal")
        self.error_tag  = buffer.create_tag("error")
        self.error_tag.set_property("foreground", "red")
        self.italic_tag = buffer.create_tag('italic')
        self.italic_tag.set_property('style', pango.STYLE_OBLIQUE)
        self.bold_tag = buffer.create_tag('bold')
        self.bold_tag.set_property('weight', pango.WEIGHT_BOLD)

        self.process = None

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

    def write(self, text, tag = None):
        buffer = self['view'].get_buffer()
        if tag is None:
            buffer.insert(buffer.get_end_iter(), text)
        else:
            buffer.insert_with_tags(buffer.get_end_iter(), text, tag)
        gobject.idle_add(self.scroll_to_end)

    def show(self):
        panel = self.window.get_bottom_panel()
        panel.show()
        panel.activate_item(self.panel)

# ex:ts=4:et:
