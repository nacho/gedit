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
from gtk import glade
import pango
import gobject
import os
import gnomevfs
from weakref import WeakKeyDictionary
from capture import *

GLADE_FILE = os.path.join(os.path.dirname(__file__), "tools.glade")

class UniqueById:
	__shared_state = WeakKeyDictionary()
	
	def __init__(self, i):
		if self.__class__.__shared_state.has_key(i):
			self.__dict__ = self.__class__.__shared_state[i]
			return True
		else:
			self.__class__.__shared_state[i] = self.__dict__
			return False

	def states(self):
		return self.__class__.__shared_state

class OutputPanel(UniqueById):
	
	def __init__(self, window):
		if UniqueById.__init__(self, window):
			return
		
		callbacks = {
			'on_stop_clicked' : self.on_stop_clicked
		}

		self.window = window
		self.ui = glade.XML(GLADE_FILE, "output-panel")
		self.ui.signal_autoconnect(callbacks)
		self.panel = self["output-panel"]
		self['view'].modify_font(pango.FontDescription('Monospace'))
		
		buffer = self['view'].get_buffer()
		self.normal_tag = buffer.create_tag("normal")
		self.error_tag  = buffer.create_tag("error")
		self.error_tag.set_property("foreground", "red")
		self.command_tag = buffer.create_tag("command")
		self.command_tag.set_property("foreground", "blue")

		self.process = None

	def __getitem__(self, key):
		# Convenience function to get a widget from its name
		return self.ui.get_widget(key)
	
	def on_stop_clicked(self, widget, *args):
		if self.process is not None:
			self.write("Stopping...\n", self.command_tag)
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
		# :TODO: Show the good panel
		pass
