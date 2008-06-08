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

import gobject
import gtk
from gtk import gdk
import gedit

from Helper import *
from Snippet import Snippet

class CompleteModel(gtk.GenericTreeModel):
        column_types = (str, object)
        column_names = ['Description', 'Node']

        def __init__(self, nodes, prefix = None, description_only = False):
                gtk.GenericTreeModel.__init__(self)
                
                self.nodes = self.create_list(nodes, prefix)
                
                # Sort the nodes
                if not prefix:
                        if description_only:
                                self.display_snippet = self.display_snippet_description
                        else:
                                self.display_snippet = self.display_snippet_default

                        self.do_filter = self.filter_snippet_default
                        
                        self.nodes.sort(lambda a, b: cmp(a['description'].lower(), \
                                        b['description'].lower()))
                else:
                        self.display_snippet = self.display_snippet_prefix
                        self.do_filter = self.filter_snippet_prefix
                        self.nodes.sort(self.sort_prefix)
                        
                self.visible_nodes = list(self.nodes)		

        def display_snippet_description(self, snippet):
                return markup_escape(snippet['description'])
        
        def display_snippet_default(self, snippet):
                return snippet.display()
        
        def display_snippet_prefix(self, snippet):
                return '<b>' + markup_escape(snippet['tag']) + '</b>: ' + \
                                markup_escape(snippet['description'])

        def sort_prefix(self, a, b):
                return cmp(a['tag'] + ': ' + a['description'], b['tag'] + ': ' + \
                                b['description'])

        def create_list(self, nodes, prefix):
                if prefix:
                        prelen = len(prefix)
                        
                result = []

                for node in nodes:
                        s = Snippet(node)
                        
                        if not prefix or \
                                        prefix.lower() == s['tag'][0:prelen].lower():
                                result.append(s)
                                
                return result

        def filter_snippet_process(self, new):		
                # Update all nodes
                old = self.visible_nodes
                oldlen = len(old)
                
                self.visible_nodes = new
                newlen = len(new)

                for index in range(0, min(newlen, oldlen)):
                        path = (index,)
                        self.row_changed(path, self.get_iter(path))
                
                if oldlen > newlen:
                        for index in range(oldlen - 1, newlen - 1, -1):
                                self.row_deleted((index,))

                elif newlen > oldlen:
                        for index in range(oldlen, newlen):
                                path = (index,)
                                self.row_inserted(path, self.get_iter(path))

        def filter_snippet_prefix(self, s):
                new = []
                s = s.lower()
                
                for node in self.nodes:
                        if s in node['tag'].lower():
                                new.append(node)
                
                self.filter_snippet_process(new)
        
        def filter_snippet_default(self, s):
                new = []
                s = s.lower()
                
                for node in self.nodes:
                        if s in node['description'].lower():
                                new.append(node)
                
                self.filter_snippet_process(new)

        def get_snippet(self, path):
                try:
                        return self.visible_nodes[path[0]]
                except IndexError:
                        return None

        def on_get_flags(self):
                return gtk.TREE_MODEL_LIST_ONLY
        
        def on_get_n_columns(self):
                return len(self.column_types)

        def on_get_column_type(self, index):
                return self.column_types[index]

        def on_get_iter(self, path):
                try:
                        return self.visible_nodes[path[0]]
                except IndexError:
                        return None

        def on_get_path(self, rowref):
                return self.visible_nodes.index(rowref)
                
        def on_get_value(self, rowref, column):
                if column == 0:
                        return self.display_snippet(rowref)
                elif column == 1:
                        return rowref
                
        def on_iter_next(self, rowref):
                try:
                        next = self.visible_nodes.index(rowref) + 1
                except ValueError:
                        next = 0
                
                try:
                        return self.visible_nodes[next]
                except IndexError:
                        return None
                
        def on_iter_children(self, parent):
                if parent:
                        return None
                else:
                        return self.visible_nodes[0]

        def on_iter_has_child(self, rowref):
                return False

        def on_iter_n_children(self, rowref):
                if rowref:
                        return 0
                return len(self.visible_nodes)

        def on_iter_nth_child(self, parent, n):
                if parent:
                        return None

                try:
                        return self.visible_nodes[n]
                except IndexError:
                        return None

        def on_iter_parent(self, child):
                return None

class SnippetComplete(gtk.Window):
        def __init__(self, nodes, prefix = None, description_only = False):
                gtk.Window.__init__(self, gtk.WINDOW_TOPLEVEL)
                self.set_keep_above(True)
                self.set_decorated(False)
                self.set_skip_taskbar_hint(True)
                self.set_skip_pager_hint(True)
                
                window = gedit.app_get_default().get_active_window()
                window.get_group().add_window(self)
                
                self.set_transient_for(window)
                self.set_size_request(200, 300)
                self.entry_changed_id = 0
                self.model = CompleteModel(nodes, prefix, description_only)
                self.build()

                if prefix:
                        self.entry.set_text(prefix)

                self.connect('delete-event', lambda x, y: x.destroy())
                self.connect('focus-out-event', self.on_focus_out)

        def run(self):
                if not self.model.nodes:
                        self.destroy()
                        return False
                
                self.show_all()
                self.entry.grab_focus()
                self.entry.set_position(-1)
                return True

        def build_tree(self):
                self.tree_view = gtk.TreeView(self.model)
                self.tree_view.set_headers_visible(False)

                column = gtk.TreeViewColumn(None)
                renderer = gtk.CellRendererText()
                column.pack_start(renderer, False)
                column.set_attributes(renderer, markup=0)
                
                self.tree_view.append_column(column)

                self.tree_view.connect('row-activated', self.on_tree_view_row_activated)
                self.tree_view.connect('key-press-event', self.on_tree_view_key_press)

        def build(self):
                vbox = gtk.VBox(False, 3)
                frame = gtk.Frame()

                frame.set_shadow_type(gtk.SHADOW_ETCHED_IN)

                self.entry = gtk.Entry()
                vbox.pack_start(self.entry, False, False, 0)
                
                self.build_tree()
                sw = gtk.ScrolledWindow()
                sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
                sw.add(self.tree_view)
                sw.set_shadow_type(gtk.SHADOW_OUT)
                
                vbox.pack_start(sw, True, True, 0)
                
                frame.add(vbox)
                self.add(frame)
                
                self.entry.connect('changed', self.on_entry_changed)
                self.entry.connect('activate', self.on_entry_activate)
                self.entry.connect('key-press-event', self.on_entry_key_press)
        
        def snippet_activated(self, snippet):
                self.emit('snippet-activated', snippet)
                self.destroy()

        def idle_filter(self, text):
                self.entry_changed_id = 0
                self.model.do_filter(text)
                
                piter = self.model.get_iter_first()
                
                if piter:
                        self.tree_view.get_selection().select_iter(piter)

                return False
        
        def on_focus_out(self, wnd, event):
                self.destroy()
                return False
                
        def on_entry_changed(self, entry):
                if self.entry_changed_id:
                        gobject.source_remove(self.entry_changed_id)
                
                self.entry_changed_id = gobject.idle_add(self.idle_filter, 
                                entry.get_text())
        
        def on_entry_key_press(self, entry, event):
                if event.keyval == gdk.keyval_from_name('Escape'):
                        self.destroy()
                        return True

                return False

        def on_tree_view_key_press(self, entry, event):
                if event.keyval == gdk.keyval_from_name('Escape'):
                        self.destroy()
                        return True

                return False

        def on_tree_view_row_activated(self, view, path, column):
                snippet = self.model.get_snippet(path)

                self.snippet_activated(snippet)

        def on_entry_activate(self, entry):
                (model, piter) = self.tree_view.get_selection().get_selected()
                
                if piter:
                        snippet = self.model.get_snippet(self.model.get_path(piter))
                        self.snippet_activated(snippet)

gobject.signal_new('snippet-activated', SnippetComplete, \
                gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, \
                (gobject.TYPE_PYOBJECT,))
