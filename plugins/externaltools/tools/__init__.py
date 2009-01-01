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
from library import ToolLibrary
from outputpanel import OutputPanel
from capture import Capture
from functions import *

class ToolMenu(object):
    ACTION_HANDLER_DATA_KEY = "ExternalToolActionHandlerData"
    ACTION_ITEM_DATA_KEY = "ExternalToolActionItemData"

    def __init__(self, library, window, menupath):
        super(ToolMenu, self).__init__()
        self._library = library
        self._window = window
        self._menupath = menupath

        self._merge_id = 0
        self._action_group = gtk.ActionGroup("ExternalToolsPluginToolActions")
        self._window.get_ui_manager().insert_action_group(self._action_group, -1)
        self.update()

    def deactivate(self):
        self.remove()
        self._window.get_ui_manager().remove_action_group(self._action_group)

    def remove(self):
        if self._merge_id != 0:
            self._window.get_ui_manager().remove_ui(self._merge_id)
            self._merge_id = 0

        for action in self._action_group.list_actions():
            handler = action.get_data(self.ACTION_HANDLER_DATA_KEY)
            if handler is not None:
                action.disconnect(handler)
            self._action_group.remove_action(action)

    def _insert_directory(self, directory, path):
        manager = self._window.get_ui_manager()

        for item in directory.subdirs:
            action_name = 'ExternalToolDirectory%X' % id(item)
            action = gtk.Action(action_name, item.name, None, None)
            self._action_group.add_action(action)

            manager.add_ui(self._merge_id, path,
                           action_name, action_name,
                           gtk.UI_MANAGER_MENU, False)
            self._insert_directory(item, path + '/' + action_name)

        for item in directory.tools:
            action_name = 'ExternalToolTool%X' % id(item)
            action = gtk.Action(action_name, item.name, item.comment, None)
            handler = action.connect("activate", capture_menu_action, self._window, item)

            action.set_data(self.ACTION_ITEM_DATA_KEY, item)
            action.set_data(self.ACTION_HANDLER_DATA_KEY, handler)
            self._action_group.add_action_with_accel(action, item.shortcut)

            manager.add_ui(self._merge_id, path,
                           action_name, action_name,
                           gtk.UI_MANAGER_MENUITEM, False)

    def update(self):
        self.remove()
        self._merge_id = self._window.get_ui_manager().new_merge_id()
        self._insert_directory(self._library.tree, self._menupath)
        self.filter(self._window.get_active_document())

    def filter(self, document):
        if document is None:
            return

        titled = document.get_uri() is not None
        remote = not document.is_local()

        states = {
            'all' : True,
            'local': titled and not remote,
            'remote': titled and remote,
            'titled': titled,
            'untitled': not titled,
        }

        for action in self._action_group.list_actions():
            item = action.get_data(self.ACTION_ITEM_DATA_KEY)
            if item is not None:
                action.set_sensitive(states[item.applicability])

class ExternalToolsWindowHelper(object):
    def __init__(self, plugin, window):
        super(ExternalToolsWindowHelper, self).__init__()

        self._window = window
        self._plugin = plugin
        self._library = ToolLibrary()

        manager = window.get_ui_manager()

        self._action_group = gtk.ActionGroup('ExternalToolsPluginActions')
        self._action_group.set_translation_domain('gedit')
        self._action_group.add_actions([('ExternalToolManager',
                                         None,
                                         _('_External Tools...'),
                                         None,
                                         _("Opens the External Tools Manager"),
                                         lambda action: plugin.open_dialog())])
        manager.insert_action_group(self._action_group, -1)

        ui_string = """
            <ui>
              <menubar name="MenuBar">
                <menu name="ToolsMenu" action="Tools">
                  <placeholder name="ToolsOps_4">
                    <separator/>
                    <placeholder name="ExternalToolPlaceholder"/>
                    <separator/>
                  </placeholder>
                  <placeholder name="ToolsOps_5">
                    <menuitem name="ExternalToolManager" action="ExternalToolManager"/>
                  </placeholder>
                </menu>
              </menubar>
            </ui>"""

        self._merge_id = manager.add_ui_from_string(ui_string)

        self.menu = ToolMenu(self._library, self._window,
                             "/MenuBar/ToolsMenu/ToolsOps_4/ExternalToolPlaceholder")
        manager.ensure_update()

        # Create output console
        self._output_buffer = OutputPanel(self._plugin.get_data_dir(), window)
        bottom = window.get_bottom_panel()
        bottom.add_item(self._output_buffer.panel,
                        _("Shell Output"),
                        gtk.STOCK_EXECUTE)

    def update_ui(self):
        self.menu.filter(self._window.get_active_document())
        self._window.get_ui_manager().ensure_update()

    def deactivate(self):
        manager = self._window.get_ui_manager()
        self.menu.deactivate()
        manager.remove_ui(self._merge_id)
        manager.remove_action_group(self._action_group)
        manager.ensure_update()

        bottom = self._window.get_bottom_panel()
        bottom.remove_item(self._output_buffer.panel)

class ExternalToolsPlugin(gedit.Plugin):
    WINDOW_DATA_KEY = "ExternalToolsPluginWindowData"

    def __init__(self):
        super(ExternalToolsPlugin, self).__init__()
        
        ToolLibrary().set_locations(self.get_data_dir())

    def activate(self, window):
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
        m = Manager(self.get_data_dir())

        m.run()
        window = gedit.app_get_default().get_active_window()
        if window:
            m.dialog.set_transient_for(window)

        return m.dialog

# ex:ts=4:et:
