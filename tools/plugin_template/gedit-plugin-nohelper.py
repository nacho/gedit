# -*- coding: utf-8 -*-

#  ##(FILENAME) - ##(DESCRIPTION)
#  
#  Copyright (C) ##(DATE_YEAR) - ##(AUTHOR_FULLNAME)
#  
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#   
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#   
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330,
#  Boston, MA 02111-1307, USA.

import gtk
import gedit
from gettext import gettext as _

##ifdef WITH_MENU
ui_str = """
<ui>
  <menubar name="MenuBar">
    <!-- Put your menu entries here -->
  </menubar>
</ui>
"""
##endif

class ##(PLUGIN_ID.camel)Plugin(gedit.Plugin):
    WINDOW_DATA_KEY = "##(PLUGIN_ID.camel)PluginWindowData"

    def __init__(self):
        gedit.Plugin.__init__(self)
					
    def activate(self, window):
##ifdef WITH_MENU
        window.set_data(self.WINDOW_DATA_KEY, dict())
        self.insert_menu(window)
##else
        pass
##endif
	
    def deactivate(self, window):
##ifdef WITH_MENU
        self.remove_menu(window)
        window.set_data(self.WINDOW_DATA_KEY, None)
##else
        pass
##endif

    def update_ui(self, window):
        pass

##ifdef WITH_CONFIGURE_DIALOG    
    def create_configure_dialog(self):
        pass

##endif
##ifdef WITH_MENU
    def insert_menu(self, window):
        data = window.get_data(self.WINDOW_DATA_KEY)
        manager = window.get_ui_manager()
        data['action_group'] = gtk.ActionGroup("##(PLUGIN_ID.camel)PluginActions")
        data['action_group'].add_actions([ 
                    # Put your actions here
                ])

        manager.insert_action_group(data['action_group'], -1)
        data['ui_id'] = manager.add_ui_from_string(ui_str)

    def remove_menu(self, window):
        data = window.get_data(self.WINDOW_DATA_KEY)
        manager = window.get_ui_manager()
        manager.remove_ui(data['ui_id'])
        manager.remove_action_group(data['action_group'])
        manager.ensure_update()

##endif

# ex:ts=4:et:
