# -*- coding: UTF-8 -*-
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

__all__ = ('ToolsPlugin', 'Manager', 'OutputPanel', 'Capture', 'UniqueById')

import gedit
import gtk
from gettext import gettext as _
from manager import Manager
from outputpanel import OutputPanel
from capture import Capture
from functions import *

class ToolsPlugin(gedit.Plugin):
	def activate(self, window):
		manager = window.get_ui_manager()
		window_data = dict()	
		window.set_data("ToolsPluginWindowData", window_data)

		window_data['action_group'] = gtk.ActionGroup("GeditToolsPluginActions")
		window_data['action_group'].set_translation_domain('gedit')
		window_data['action_group'].add_actions([('ToolsManager',
		                                          None,
		                                          '_External Tools...',
		                                          None,
		                                          "Opens the External Tools Manager",
		                                          lambda action: self.open_dialog())])
		window_data['ui_id'] = manager.new_merge_id()
		manager.insert_action_group(window_data['action_group'], -1)
		manager.add_ui(window_data['ui_id'],
		               '/MenuBar/ToolsMenu/ToolsOps_5',
		               'ToolsManager', 'ToolsManager',
		               gtk.UI_MANAGER_MENUITEM, False)
		insert_tools_menu(window)
		filter_tools_menu(window)
		manager.ensure_update()

		# Create output console
		window_data["output_buffer"] = OutputPanel(window)
		bottom = window.get_bottom_panel()
		bottom.add_item(window_data["output_buffer"].panel, "Shell Output", gtk.STOCK_EXECUTE)

	def deactivate(self, window):
		window_data = window.get_data("ToolsPluginWindowData")
		manager = window.get_ui_manager()
	
		manager.remove_ui(window_data["ui_id"])
		manager.remove_action_group(window_data["action_group"])

		remove_tools_menu(window)

		manager.ensure_update()
		bottom = window.get_bottom_panel()
		bottom.remove_item(window_data["output_buffer"].panel)
		window.set_data("ToolsPluginWindowData", None)

	def update_ui(self, window):
		filter_tools_menu(window)
		window.get_ui_manager().ensure_update()

	def create_configure_dialog(self):
		return self.open_dialog()

	def open_dialog(self):
		tm = Manager().dialog
		window = gedit.gedit_app_get_default().get_active_window()
		if window:
			tm.set_transient_for(window)
		return tm
