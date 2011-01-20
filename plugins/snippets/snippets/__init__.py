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

import sys
import os
import shutil

import cairo
import glib

from gi.repository import Gtk, Gdk, Gedit, PeasGtk, GObject
import platform

from library import Library
from manager import Manager
from snippet import Snippet

from windowactivatable import WindowActivatable

class AppActivatable(GObject.Object, Gedit.AppActivatable, PeasGtk.Configurable):
        __gtype_name__ = "GeditSnippetsAppActivatable"

        app = GObject.property(type=Gedit.App)

        def __init__(self):
                GObject.Object.__init__(self)

                self.dlg = None

        def do_activate(self):
                # Initialize snippets library
                library = Library()
                library.set_accelerator_callback(self.accelerator_activated)

                if platform.platform() == 'Windows':
                        snippetsdir = os.path.expanduser('~/gedit/snippets')
                else:
                        userdir = os.getenv('GNOME22_USER_DIR')
                        if userdir:
                                snippetsdir = os.path.join(userdir, 'gedit/snippets')
                        else:
                                snippetsdir = os.path.join(glib.get_user_config_dir(), 'gedit/snippets')

                library.set_dirs(snippetsdir, self.system_dirs())

        def system_dirs(self):
                if platform.platform() != 'Windows':
                        if 'XDG_DATA_DIRS' in os.environ:
                                datadirs = os.environ['XDG_DATA_DIRS']
                        else:
                                datadirs = '/usr/local/share' + os.pathsep + '/usr/share'

                        dirs = []

                        for d in datadirs.split(os.pathsep):
                                d = os.path.join(d, 'gedit', 'plugins', 'snippets')

                                if os.path.isdir(d):
                                        dirs.append(d)

                dirs.append(self.plugin_info.get_data_dir())
                return dirs


        def do_create_configure_widget(self):
                builder = Gtk.Builder()
                builder.add_from_file(os.path.join(self.plugin_info.get_data_dir(), 'ui', 'snippets.ui'))

                return builder.get_object('snippets_manager')

        def accelerator_activated(self, group, obj, keyval, mod):
                ret = False

#                if hasattr(obj, '_snippets_plugin_data'):
#                        ret = obj._snippets_plugin_data.accelerator_activated(keyval, mod)

                return ret

# ex:ts=8:et:
