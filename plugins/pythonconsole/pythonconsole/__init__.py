# -*- coding: utf-8 -*-

# __init__.py -- plugin object
#
# Copyright (C) 2006 - Steve Frécinaux
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

# Parts from "Interactive Python-GTK Console" (stolen from epiphany's console.py)
#     Copyright (C), 1998 James Henstridge <james@daa.com.au>
#     Copyright (C), 2005 Adam Hooper <adamh@densi.com>
# Bits from gedit Python Console Plugin
#     Copyrignt (C), 2005 Raphaël Slinckx

import gtk
import gedit
from console import PythonConsole

class PythonConsolePlugin(gedit.Plugin):
	def __init__(self):
		gedit.Plugin.__init__(self)
			
	def activate(self, window):
		console = PythonConsole(namespace = {'__builtins__' : __builtins__,
		                                     'gedit' : gedit,
		                                     'window' : window})
		console.eval('print "You can access the main window through ' \
		             '\'window\' :\\n%s" % window', False)
		bottom = window.get_bottom_panel()
		image = gtk.Image()
		image.set_from_icon_name('gnome-mime-text-x-python',
		                         gtk.ICON_SIZE_MENU)
		bottom.add_item(console, 'Python Console', image)
		window.set_data('PythonConsolePluginInfo', console)
	
	def deactivate(self, window):
		console = window.get_data("PythonConsolePluginInfo")
		window.set_data("PythonConsolePluginInfo", None)
		bottom = window.get_bottom_panel()
		bottom.remove_item(console)