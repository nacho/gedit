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

import os

import gobject
import gtk
from gtk import glade
from gtk import gdk
import gtksourceview2 as gsv
import pango
import gedit

from Snippet import Snippet
from Helper import *
from Library import *

class Manager:
        NAME_COLUMN = 0
        SORT_COLUMN = 1
        OBJ_COLUMN = 2

        model = None
        
        def __init__(self):
                self.snippet = None
                self.dlg = None
                self.key_press_id = 0
                self.tooltips = gtk.Tooltips()
                self.run()

        def get_language_snippets(self, path, name = None):
                library = Library()
                
                name = self.get_language(path)
                nodes = library.get_snippets(name)

                return nodes

        def add_new_snippet_node(self, parent):
                return self.model.append(parent, ('<i>' + _('Add a new snippet...') + \
                                '</i>', '', None))

        def fill_language(self, piter):
                # Remove all children
                child = self.model.iter_children(piter)
                
                while child and	self.model.remove(child):
                        True
                
                path = self.model.get_path(piter)
                nodes = self.get_language_snippets(path)
                language = self.get_language(path)
                
                Library().ref(language)
                
                if nodes:
                        for node in nodes:
                                self.add_snippet(piter, node)
                else:
                        # Add node that tells there are no snippets currently
                        self.add_new_snippet_node(piter)

                self.tree_view.expand_row(path, False)

        def build_model(self):
                window = gedit.app_get_default().get_active_window()
                
                if window:
                        view = window.get_active_view()

                        if not view:
                                current_lang = None
                        else:
                                current_lang = view.get_buffer().get_language()
                                source_view = self['source_view_snippet']
                                
                                source_view.set_auto_indent(view.get_auto_indent())
                                source_view.set_insert_spaces_instead_of_tabs( \
                                                view.get_insert_spaces_instead_of_tabs())
                                source_view.set_smart_home_end(view.get_smart_home_end())
                                source_view.set_tabs_width(view.get_tabs_width())

                else:
                        current_lang = None

                tree_view = self['tree_view_snippets']
                expand = None
                
                if not self.model:
                        self.model = gtk.TreeStore(str, str, object)
                        self.model.set_sort_column_id(self.SORT_COLUMN, gtk.SORT_ASCENDING)
                        manager = gedit.get_language_manager()
                        langs = manager.list_languages()
                        
                        piter = self.model.append(None, (_('Global'), '', None))
                        # Add dummy node
                        self.model.append(piter, ('', '', None))
                        
                        nm = None
                        
                        if current_lang:
                                nm = current_lang.get_name()
                
                        for lang in langs:
                                name = lang.get_name()
                                parent = self.model.append(None, (name, name, lang))

                                # Add dummy node
                                self.model.append(parent, ('', '', None))

                                if (nm == name):
                                        expand = parent
                else:
                        if current_lang:
                                piter = self.model.get_iter_first()
                                nm = current_lang.get_name()
                                
                                while piter:
                                        lang = self.model.get_value(piter, \
                                                        self.SORT_COLUMN)
                                        
                                        if lang	== nm:
                                                expand = piter
                                                break;
                                                
                                        piter = self.model.iter_next(piter)

                tree_view.set_model(self.model)
                
                if not expand:
                        expand = self.model.get_iter_root()
                        
                tree_view.expand_row(self.model.get_path(expand), False)
                self.select_iter(expand)

        def get_cell_data_cb(self, column, cell, model, iter):
                s = model.get_value(iter, self.OBJ_COLUMN)
                
                snippet = isinstance(s, SnippetData)
                
                cell.set_property('editable', snippet)
                
                if snippet and not s.valid:
                        cell.set_property('foreground-gdk', gdk.color_parse('red'))
                else:
                        cell.set_property('foreground-set', False)
                
                cell.set_property('markup', model.get_value(iter, self.NAME_COLUMN))
                

        def build_tree_view(self):		
                self.tree_view = self['tree_view_snippets']
                
                self.column = gtk.TreeViewColumn(None)
                self.renderer = gtk.CellRendererText()
                self.column.pack_start(self.renderer, False)
                self.column.set_cell_data_func(self.renderer, self.get_cell_data_cb)
                
                self.tree_view.append_column(self.column)
                
                self.renderer.connect('edited', self.on_cell_edited)
                self.renderer.connect('editing-started', self.on_cell_editing_started)

                self.tree_view.get_selection().connect('changed', \
                                self.on_tree_view_selection_changed)

        def custom_handler(self, xml, function_name, widget_name, str1, str2, \
                        int1 , int2):
                if function_name == 'create_source_view':
                        buf = gsv.Buffer()
                        buf.set_highlight(True)
                        source_view = gsv.View(buf)
                        source_view.set_auto_indent(True)
                        source_view.set_insert_spaces_instead_of_tabs(False)
                        source_view.set_smart_home_end(True)
                        source_view.set_tabs_width(4)
                        
                        return source_view
                else:
                        return None
                
        def build(self):
                glade.set_custom_handler(self.custom_handler)
                self.xml = glade.XML(os.path.dirname(__file__) + '/snippets.glade')
                
                handlers_dic = {
                                'on_dialog_snippets_response': \
                                        self.on_dialog_snippets_response, \
                                'on_button_new_snippet_clicked': \
                                        self.on_button_new_snippet_clicked, \
                                'on_button_remove_snippet_clicked': \
                                        self.on_button_remove_snippet_clicked, \
                                'on_entry_tab_trigger_focus_out': \
                                        self.on_entry_tab_trigger_focus_out, \
                                'on_entry_tab_trigger_changed': \
                                        self.on_entry_tab_trigger_changed, \
                                'on_entry_accelerator_focus_out': \
                                        self.on_entry_accelerator_focus_out, \
                                'on_entry_accelerator_focus_in': \
                                        self.on_entry_accelerator_focus_in, \
                                'on_entry_accelerator_key_press': \
                                        self.on_entry_accelerator_key_press, \
                                'on_source_view_snippet_focus_out': \
                                        self.on_source_view_snippet_focus_out, \
                                'on_tree_view_snippets_row_expanded': \
                                        self.on_tree_view_snippets_row_expanded, \
                                'on_tree_view_snippets_key_press': \
                                        self.on_tree_view_snippets_key_press \
                                }

                self.xml.signal_autoconnect(handlers_dic)
                
                self.build_tree_view()
                self.build_model()

                button = self['button_remove_snippet']
                button.set_use_stock(True)
                button.set_label(gtk.STOCK_REMOVE)

                source_view = self['source_view_snippet']
                source_view.modify_font(pango.FontDescription('Monospace 8'))

                self.dlg = self['dialog_snippets']
        
        def __getitem__(self, key):
                return self.xml.get_widget(key)

        def is_filled(self, piter):
                if not self.model.iter_has_child(piter):
                        return True
                
                child = self.model.iter_children(piter)
                nm = self.model.get_value(child, self.NAME_COLUMN)
                obj = self.model.get_value(child, self.OBJ_COLUMN)
                
                return (obj or nm)

        def fill_if_needed(self, piter):
                if not self.is_filled(piter):
                        self.fill_language(piter)

        def find_iter(self, parent, snippet):
                self.fill_if_needed(parent)
                piter = self.model.iter_children(parent)
                
                while (piter):
                        node = self.model.get_value(piter, self.OBJ_COLUMN)

                        if node == snippet.data:
                                return piter
                        
                        piter = self.model.iter_next(piter)
                
                return None

        def update_remove_button(self):
                button = self['button_remove_snippet']
                
                if not self.snippet:
                        button.set_sensitive(False)
                        button.set_label(gtk.STOCK_REMOVE)	
                else:
                        if self.snippet.data.can_modify():
                                button.set_sensitive(True)
                                
                         	if self.snippet.data.is_override():
                                        button.set_label(gtk.STOCK_REVERT_TO_SAVED)
                                else:
                                        button.set_label(gtk.STOCK_REMOVE)
                        else:
                                button.set_sensitive(False)
                                button.set_label(gtk.STOCK_REMOVE)

        def snippet_changed(self, piter = None):
                if piter:
                        node = self.model.get_value(piter, self.OBJ_COLUMN)
                        s = Snippet(node)
                else:
                        s = self.snippet
                        piter = self.find_iter(self.model.get_iter(self.language_path), s)

                if piter:
                        nm = s.display()
                        
                        self.model.set(piter, self.NAME_COLUMN, nm, self.SORT_COLUMN, nm)
                        self.update_remove_button()
                        self.entry_tab_trigger_update_valid()

                return piter

        def add_snippet(self, parent, snippet):
                piter = self.model.append(parent, ('', '', snippet))
                
                return self.snippet_changed(piter)

        def run(self):
                if not self.dlg:
                        self.build()
                        self.dlg.show_all()
                else:
                        self.build_model()
                        self.dlg.present()
        
        def selected_snippet(self):
                (model, piter) = self.tree_view.get_selection().get_selected()
                
                if piter:
                        parent = model.iter_parent(piter)
                        
                        if parent:
                                return parent, piter, \
                                                model.get_value(piter, self.OBJ_COLUMN)
                        else:
                                return parent, piter, None		
                else:
                        return None, None, None

        def selection_changed(self):
                if not self.snippet:
                        sens = False

                        self['entry_tab_trigger'].set_text('')
                        self['entry_accelerator'].set_text('')			
                        self['source_view_snippet'].get_buffer().set_text('')

                        self.tooltips.disable()
                else:
                        sens = True

                        self['entry_tab_trigger'].set_text(self.snippet['tag'])
                        self['entry_accelerator'].set_text( \
                                        self.snippet.accelerator_display())
                        
                        buf = self['source_view_snippet'].get_buffer()
                        lang = self.model.get_value(self.model.get_iter( \
                                        self.language_path), self.OBJ_COLUMN)
                        
                        buf.set_language(lang)
                        buf.set_text(self.snippet['text'])
                        
                        self.tooltips.enable()

                for name in ['source_view_snippet', 'label_tab_trigger', \
                                'entry_tab_trigger', 'label_accelerator', 'entry_accelerator']:
                        self[name].set_sensitive(sens)
                
                self.update_remove_button()
                        
        def select_iter(self, piter):
                self.tree_view.get_selection().select_iter(piter)
                self.tree_view.scroll_to_cell(self.model.get_path(piter), None, \
                        True, 0.5, 0.5)

        def get_language(self, path):
                if path[0] == 0:
                        return None
                else:
                        return self.model.get_value(self.model.get_iter( \
                                        (path[0],)), self.OBJ_COLUMN).get_id()

        def new_snippet(self, properties=None):
                if not self.language_path:
                        return None

                parent = self.model.get_iter(self.language_path)
                snippet = Library().new_snippet(self.get_language( \
                                self.language_path), properties)
                
                return Snippet(snippet)

        def get_dummy(self, parent):
                if not self.model.iter_n_children(parent) == 1:
                        return None
                
                dummy = self.model.iter_children(parent)
                
                if not self.model.get_value(dummy, self.OBJ_COLUMN):
                        return dummy
        
                return None
        
        def unref_languages(self):
                piter = self.model.get_iter_first()
                library = Library()
                
                while piter:
                        if self.is_filled(piter):
                                language = self.get_language(self.model.get_path(piter))
                                library.save(language)

                                library.unref(language)
                        
                        piter = self.model.iter_next(piter)

        # Callbacks
        def on_dialog_snippets_response(self, dlg, resp):
                if resp == gtk.RESPONSE_HELP:
                        gedit.help_display(self.dlg, 'gedit.xml', 'gedit-snippets-plugin')
                        return

                self.unref_languages()	
                self.snippet = None	
                self.model = None
                self.dlg.destroy()
                self.dlg = None
        
        def on_cell_editing_started(self, renderer, editable, path):
                piter = self.model.get_iter(path)
                
                if not self.model.iter_parent(piter):
                        renderer.stop_editing(True)
                        editable.remove_widget()
                elif isinstance(editable, gtk.Entry):
                        if self.snippet:
                                editable.set_text(self.snippet['description'])
                        else:
                                # This is the `Add a new snippet...` item
                                editable.set_text('')
                        
                        editable.grab_focus()
        
        def on_cell_edited(self, cell, path, new_text):		
                if new_text != '':
                        piter = self.model.get_iter(path)
                        node = self.model.get_value(piter, self.OBJ_COLUMN)
                        
                        if node:
                                if node == self.snippet.data:
                                        s = self.snippet
                                else:
                                        s = Snippet(node)
                        
                                s['description'] = new_text
                                self.snippet_changed(piter)
                                self.select_iter(piter)
                        else:
                                # This is the `Add a new snippet...` item
                                # We create a new snippet
                                snippet = self.new_snippet({'description': new_text})
                                
                                if snippet:
                                        self.model.set(piter, self.OBJ_COLUMN, snippet.data)
                                        self.snippet_changed(piter)
                                        self.snippet = snippet
                                        self.selection_changed()
        
        def on_entry_accelerator_focus_out(self, entry, event):
                if not self.snippet:
                        return

                entry.set_text(self.snippet.accelerator_display())

        def entry_tab_trigger_update_valid(self):
                entry = self['entry_tab_trigger']
                text = entry.get_text()
                
                if text and not Library().valid_tab_trigger(text):
                        entry.modify_base(gtk.STATE_NORMAL, gtk.gdk.Color(0xffff, 0x6666, \
                                        0x6666))
                        entry.modify_text(gtk.STATE_NORMAL, gtk.gdk.Color(0xffff, 0xffff, \
                                        0xffff))

                        self.tooltips.set_tip(entry, _('This is not a valid tab trigger. Triggers can either contain letters or a single, non alphanumeric, character like {, [, etcetera.'))
                else:
                        entry.modify_base(gtk.STATE_NORMAL, None)
                        entry.modify_text(gtk.STATE_NORMAL, None)

                        self.tooltips.set_tip(entry, None)
                
                return False

        def on_entry_tab_trigger_focus_out(self, entry, event):
                if not self.snippet:
                        return

                text = entry.get_text()

                # save tag
                self.snippet['tag'] = text
                self.snippet_changed()
        
        def on_entry_tab_trigger_changed(self, entry):
                self.entry_tab_trigger_update_valid()
        
        def on_source_view_snippet_focus_out(self, source_view, event):
                if not self.snippet:
                        return

                buf = source_view.get_buffer()
                text = buf.get_text(buf.get_start_iter(), \
                                buf.get_end_iter())

                self.snippet['text'] = text
                self.snippet_changed()
        
        def on_button_new_snippet_clicked(self, button):
                snippet = self.new_snippet()
                
                if not snippet:
                        return

                parent = self.model.get_iter(self.language_path)
                path = self.model.get_path(parent)
                
                dummy = self.get_dummy(parent)
                
                if dummy:
                        # Remove the dummy
                        self.model.remove(dummy)
                
                # Add the snippet
                piter = self.add_snippet(parent, snippet.data)
                self.select_iter(piter)

                if not self.tree_view.row_expanded(path):
                        self.tree_view.expand_row(path, False)
                        self.select_iter(piter)

                self.tree_view.grab_focus()

                path = self.model.get_path(piter)
                self.tree_view.set_cursor(path, self.column, True)
                
        def on_button_remove_snippet_clicked(self, button):
                parent, piter, node = self.selected_snippet()

                if not self.snippet or not node.can_modify():
                        return
                
                if node.is_override():
                        Library().revert_snippet(node)
                        self.selection_changed()
                else:
                        Library().remove_snippet(node)
                        self.snippet = None

                        path = self.model.get_path(piter)
                
                        if self.model.remove(piter):
                                self.select_iter(piter)
                        elif path[-1] != 0:
                                self.select_iter(self.model.get_iter((path[0], path[1] - 1)))
                        else:
                                dummy = self.add_new_snippet_node(parent)
                                self.tree_view.expand_row(self.model.get_path(parent), False)
                                self.select_iter(dummy)
                
                self.tree_view.grab_focus()
        
        def set_accelerator(self, keyval, mod):
                accelerator = gtk.accelerator_name(keyval, mod)
                self.snippet['accelerator'] = accelerator

                return True
        
        def on_entry_accelerator_key_press(self, entry, event):
                source_view = self['source_view_snippet']

                if event.keyval == gdk.keyval_from_name('Escape'):
                        # Reset
                        entry.set_text(self.snippet.accelerator_display())
                        self.tree_view.grab_focus()
                        
                        return True
                elif event.keyval == gdk.keyval_from_name('Delete') or \
                                event.keyval == gdk.keyval_from_name('BackSpace'):
                        # Remove the accelerator
                        entry.set_text('')
                        self.snippet['accelerator'] = ''
                        self.tree_view.grab_focus()
                        
                        self.snippet_changed()
                        return True
                elif Library().valid_accelerator(event.keyval, event.state):
                        # New accelerator
                        self.set_accelerator(event.keyval, \
                                        event.state & gtk.accelerator_get_default_mod_mask())
                        entry.set_text(self.snippet.accelerator_display())
                        self.snippet_changed()
                        self.tree_view.grab_focus()

                else:
                        return True
        
        def on_entry_accelerator_focus_in(self, entry, event):
                if self.snippet['accelerator']:
                        entry.set_text(_('Type a new shortcut, or press Backspace to clear'))
                else:
                        entry.set_text(_('Type a new shortcut'))
        
        def on_tree_view_selection_changed(self, selection):
                parent, piter, node = self.selected_snippet()
                
                if self.snippet:
                        self.on_entry_tab_trigger_focus_out(self['entry_tab_trigger'], \
                                        None)
                        self.on_source_view_snippet_focus_out( \
                                        self['source_view_snippet'], None)
                
                if parent:
                        self.language_path = self.model.get_path(parent)
                elif piter:
                        self.language_path = self.model.get_path(piter)
                else:
                        self.language_path = None

                if node:
                        self.snippet = Snippet(node)
                else:
                        self.snippet = None

                self.selection_changed()

        def iter_after(self, target, after):
                if not after:
                        return True

                tp = self.model.get_path(target)
                ap = self.model.get_path(after)
                
                if tp[0] > ap[0] or (tp[0] == ap[0] and (len(ap) == 1 or tp[1] > ap[1])):
                        return True
                
                return False
                
        def on_tree_view_snippets_key_press(self, treeview, event):
                if self.snippet and event.keyval == gdk.keyval_from_name('Delete') \
                                and	self.snippet:
                        self.on_button_remove_snippet_clicked(None)
                        return True

        def on_tree_view_snippets_row_expanded(self, treeview, piter, path):
                # Check if it is already filled
                self.fill_if_needed(piter)
                self.select_iter(piter)
