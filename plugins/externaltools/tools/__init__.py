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

__all__ = ('ExternalToolsPlugin', 'ExternalToolsWindowHelper',
           'Manager', 'OutputPanel', 'Capture', 'UniqueById')

import gedit
import gtk
from manager import Manager
from outputpanel import OutputPanel
from capture import Capture
from functions import *

class ExternalToolsWindowHelper(object):
    def __init__(self, plugin, window):
        super(ExternalToolsWindowHelper, self).__init__()

        self._window = window
        self._plugin = plugin
    
        manager = window.get_ui_manager()

        self._action_group = gtk.ActionGroup('ExternalToolsPluginActions')
        self._action_group.set_translation_domain('gedit')
        self._action_group.add_actions([('ToolsManager',
                                         None,
                                         _('_External Tools...'),
                                         None,
                                         _("Opens the External Tools Manager"),
                                         lambda action: plugin.open_dialog())])
        manager.insert_action_group(self._action_group, -1)

        self._merge_id = manager.new_merge_id()
        manager.add_ui(self._merge_id,
                       '/MenuBar/ToolsMenu/ToolsOps_5',
                       'ToolsManager', 'ToolsManager',
                       gtk.UI_MANAGER_MENUITEM, False)
        insert_tools_menu(window)
        filter_tools_menu(window)
        manager.ensure_update()

        # Create output console
        self._output_buffer = OutputPanel(window)
        bottom = window.get_bottom_panel()
        bottom.add_item(self._output_buffer.panel,
                        _("Shell Output"),
                        gtk.STOCK_EXECUTE)

    def update_ui(self):
        filter_tools_menu(self._window)
        self._window.get_ui_manager().ensure_update()

    def deactivate(self):
        manager = self._window.get_ui_manager()
    
        manager.remove_ui(self._merge_id)
        manager.remove_action_group(self._action_group)

        remove_tools_menu(self._window)

        manager.ensure_update()
        bottom = self._window.get_bottom_panel()
        bottom.remove_item(self._output_buffer.panel)

class ExternalToolsPlugin(gedit.Plugin):
    WINDOW_DATA_KEY = "ExternalToolsPluginWindowData"

    def activate(self, window):
        super(ExternalToolsPlugin, self).__init__()
        
        helper = ExternalToolsWindowHelper(self, window)
        window.set_data(self.WINDOW_DATA_KEY, helper)

    def deactivate(self, window):
        window.get_data(self.WINDOW_DATA_KEY).deactivate()
        window.set_data(self.WINDOW_DATA_KEY, None)

    def update_ui(self, window):
        window.get_data(self.WINDOW_DATA_KEY).update_ui()

    def create_configure_dialog(self):
        return self.open_dialog()

    def open_dialog(self):
        tm = Manager().dialog
        window = gedit.app_get_default().get_active_window()
        if window:
            tm.set_transient_for(window)
        return tm

# ex:ts=4:et:
