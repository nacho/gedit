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

__all__ = ('Manager', )

import gedit
import gtk
import gtksourceview2 as gsv
from gtk import glade
import os.path
from library import *
from functions import *
import md5

class Manager(object):
    GLADE_FILE = os.path.join(os.path.dirname(__file__), "tools.glade")

    LABEL_COLUMN = 0 # For Combo and Tree
    NODE_COLUMN  = 1 # For Tree only
    NAME_COLUMN = 1  # For Combo only

    __shared_state = None

    combobox_items = {
        'input': (
            ('nothing'  , _('Nothing')),
            ('document' , _('Current document')),
            ('selection', _('Current selection')),
            ('line'     , _('Current line')),
            ('word'     , _('Current word'))
        ),
        'output': (
            ('output-panel'     , _('Display in bottom pane')),
            ('new-document'     , _('Create new document')),
            ('append-document'  , _('Append to current document')),
            ('replace-document' , _('Replace current document')),
            ('replace-selection', _('Replace current selection')),
            ('insert'           , _('Insert at cursor position'))
        ),
        'applicability': (
            ('all'     , _('All documents')),
            ('titled'  , _('All documents except untitled ones')),
            ('local'   , _('Local files only')),
            ('remote'  , _('Remote files only')),
            ('untitled', _('Untitled documents only'))
        )
    }

    _instance = None

    def __new__(cls):
        if cls._instance is None:
            cls._instance = object.__new__(cls)
        return cls._instance

    def __init__(self):
        callbacks = {
            'on_new_tool_button_clicked'      : self.on_new_tool_button_clicked,
            'on_remove_tool_button_clicked'   : self.on_remove_tool_button_clicked,
            'on_tool_manager_dialog_response' : self.on_tool_manager_dialog_response,
            'on_tool_manager_dialog_focus_out': self.on_tool_manager_dialog_focus_out,
            'on_accelerator_key_press'        : self.on_accelerator_key_press,
            'on_accelerator_focus_in'         : self.on_accelerator_focus_in,
            'on_accelerator_focus_out'        : self.on_accelerator_focus_out
        }

        # Load the "main-window" widget from the glade file.
        glade.set_custom_handler(self.custom_handler)
        self.ui = glade.XML(self.GLADE_FILE, 'tool-manager-dialog')
        self.ui.signal_autoconnect(callbacks)
        self.dialog = self.ui.get_widget('tool-manager-dialog')
        self.view = self.ui.get_widget('view')
        for name in ['input', 'output', 'applicability']:
            self.__init_combobox(name)
        self.__init_tools_model()
        self.__init_tools_view()
        self.dialog.show()

    def __init_tools_model(self):
        self.tools = ToolLibrary()
        self.current_node = None
        self.script_hash = None
        self.accelerators = dict()

        self.model = gtk.ListStore(str, object)
        self.view.set_model(self.model)

        for item in self.tools.tree.tools:
            self.model.append([item.name, item])
            if item.shortcut:
                self.accelerators[item.shortcut] = item

    def __init_tools_view(self):
        # Tools column
        column = gtk.TreeViewColumn('Tools')
        renderer = gtk.CellRendererText()
        column.pack_start(renderer, False)
        column.set_attributes(renderer, text = self.LABEL_COLUMN)
        renderer.set_property('editable', True)
        self.view.append_column(column)

        renderer.connect('edited', self.on_view_label_cell_edited)
        self.selection_changed_id = self.view.get_selection().connect('changed', self.on_view_selection_changed, None)

    def __init_combobox(self, name):
        combo = self[name]
        model = gtk.ListStore(str, str)
        combo.set_model(model)

        for name, label in Manager.combobox_items[name]:
            model.append((label, name))
        combo.set_active(0)

    # Convenience function to get a widget from its name
    def __getitem__(self, key):
        return self.ui.get_widget(key)

    def custom_handler(self, xml, function_name, widget_name,
                       str1, str2, int1 , int2):
        if function_name == 'create_commands':
            buf = gsv.Buffer()
            view = gsv.View(buf)
            view.set_wrap_mode(gtk.WRAP_WORD)
            view.show()
            return view
        else:
            return None

    def set_active_by_name(self, combo_name, option_name):
        combo = self[combo_name]
        model = combo.get_model()
        piter = model.get_iter_first()
        while piter is not None:
            if model.get_value(piter, self.NAME_COLUMN) == option_name:
                combo.set_active_iter(piter)
                return True
            piter = model.iter_next(piter)
        return False

    def get_selected_tool(self):
        model, piter = self.view.get_selection().get_selected()

        if piter is not None:
            return piter, model.get_value(piter, self.NODE_COLUMN);
        else:
            return None, None

    def compute_hash(self, string):
        return md5.md5(string).hexdigest()

    def save_current_tool(self):
        if self.current_node is None:
             return

        if self.current_node.filename is None:
            self.current_node.autoset_filename()

        def combo_value(o, name):
            combo = o[name]
            return combo.get_model().get_value(combo.get_active_iter(), self.NAME_COLUMN)

        self.current_node.input = combo_value(self, 'input')
        self.current_node.output = combo_value(self, 'output')
        self.current_node.applicability = combo_value(self, 'applicability')
        self.current_node.comment = self['description'].get_text()

        buf = self['commands'].get_buffer()
        script  = buf.get_text(*buf.get_bounds())
        h = self.compute_hash(script)
        if h != self.script_hash:
            # script has changed -> save it
            self.current_node.save_with_script([line + "\n" for line in script.split("\n")])
            self.script_hash = h
        else:
            self.current_node.save()

    def clear_fields(self):
        self['description'].set_text('')
        self['accelerator'].set_text('')
        self['commands'].get_buffer().set_text('')

        self.set_active_by_name('input', Manager.combobox_items['input'][0][0])
        self.set_active_by_name('output', Manager.combobox_items['output'][0][0])
        self.set_active_by_name('applicability', Manager.combobox_items['applicability'][0][0])
        self['title'].set_label(_('Edit tool <i>%s</i>:') % '') # a bit ugly, but we're string frozen
    
    def fill_fields(self):
        node = self.current_node
        self['description'].set_text(default(node.comment, _('A Brand New Tool')))
        self['accelerator'].set_text(default(node.shortcut, ''))

        buf = self['commands'].get_buffer()
        script = default(''.join(node.get_script()), '')
        buf.set_text(script)
        self.script_hash = self.compute_hash(script)
        mimetype = gnomevfs.get_mime_type_for_data(script)
        lmanager = gedit.get_language_manager()
        language = gedit.language_manager_get_language_from_mime_type(lmanager, mimetype)

        if language is not None:
            buf.set_language(language)
            buf.set_highlight_syntax(True)
        else:
            buf.set_highlight_syntax(False)

        self.set_active_by_name('input',
                                default(node.input,
                                        Manager.combobox_items['input'][0][0]))
        self.set_active_by_name('output',
                                default(node.output,
                                        Manager.combobox_items['output'][0][0]))
        self.set_active_by_name('applicability',
                                default(node.applicability,
                                        Manager.combobox_items['applicability'][0][0]))
        self['title'].set_label(_('Edit tool <i>%s</i>:') % node.name)

    def on_new_tool_button_clicked(self, button):
        self.save_current_tool()
        
        # block handlers while inserting a new item
        self.view.get_selection().handler_block(self.selection_changed_id)

        self.current_node = Tool(self.tools.tree);
        self.current_node.name = _('New tool')
        self.tools.tree.tools.append(self.current_node)
        piter = self.model.append([self.current_node.name, self.current_node])
        self.view.set_cursor(self.model.get_path(piter),
                             self.view.get_column(self.LABEL_COLUMN),
                             True)
        self.fill_fields()
        self['tool-table'].set_sensitive(True)

        self.view.get_selection().handler_unblock(self.selection_changed_id)

    def on_remove_tool_button_clicked(self, button):
        piter, node = self.get_selected_tool()

        if node.is_global():
            if node.parent.revert_tool(node):
                if self.current_node.shortcut and \
                   self.accelerator.has_key(self.current_node.shortcut):
                    del self.accelerators[self.current_node.shortcut]                
                self['revert-tool-button'].set_sensitive(False)
                self.fill_fields()
        else:
            if node.parent.delete_tool(node):
                if self.current_node.shortcut:
                    del self.accelerators[self.current_node.shortcut]
                self.current_node = None
                self.script_hash = None
                if self.model.remove(piter):
                    self.view.set_cursor(self.model.get_path(piter),
                                         self.view.get_column(self.LABEL_COLUMN),
                                        False)
                    self.view.grab_focus()

    def on_view_label_cell_edited(self, cell, path, new_text):
        if new_text != '':
            piter = self.model.get_iter(path)
            node = self.model.get_value(piter, self.NODE_COLUMN)
            node.name = new_text
            self.model.set(piter, self.LABEL_COLUMN, new_text)
            self['title'].set_label(_('Edit tool <i>%s</i>:') % new_text)

    def on_view_selection_changed(self, selection, userdata):
        self.save_current_tool()
        piter, node = self.get_selected_tool()

        removable = piter is not None and node is not None and node.is_local()
        self['remove-tool-button'].set_sensitive(removable)
        self['revert-tool-button'].set_sensitive(removable)
        if node is not None and node.is_global():
            self['remove-tool-button'].hide()
            self['revert-tool-button'].show()
        else:
            self['remove-tool-button'].show()
            self['revert-tool-button'].hide()

        if node is not None:
            self.current_node = node
            self.fill_fields()
            self['tool-table'].set_sensitive(True)
        else:
            self.clear_fields()
            self['tool-table'].set_sensitive(False)

    def set_accelerator(self, keyval, mod):
        # Check whether accelerator already exists

        if self.current_node.shortcut:
            del self.accelerators[self.current_node.shortcut]
        name = gtk.accelerator_name(keyval, mod)
        if name != '':
            if self.accelerators.has_key(name):
                dialog = gtk.MessageDialog(self.dialog,
                                           gtk.DIALOG_MODAL,
                                           gtk.MESSAGE_ERROR,
                                           gtk.BUTTONS_OK,
                                           _('This accelerator is already bound to %s') % self.accelerators[name].name)
                dialog.run()
                dialog.destroy()
                return False
            self.current_node.shortcut = name
            self.accelerators[name] = self.current_node
        else:
            self.current_node.shortcut = None
        return True

    def on_accelerator_key_press(self, entry, event):
        mask = event.state & gtk.accelerator_get_default_mod_mask()

        if event.keyval == gtk.keysyms.Escape:
            entry.set_text(default(self.current_node.shortcut, ''))
            self['commands'].grab_focus()
            return True
        elif event.keyval == gtk.keysyms.Delete \
          or event.keyval == gtk.keysyms.BackSpace:
            entry.set_text('')
            self.current_node.shortcut = None
            self['commands'].grab_focus()
            return True
        elif event.keyval in range(gtk.keysyms.F1, gtk.keysyms.F12 + 1):
            # New accelerator
            self.set_accelerator(event.keyval, mask)
            entry.set_text(default(self.current_node.shortcut, ''))
            self['commands'].grab_focus()
            # Capture all `normal characters`
            return True
        elif gtk.gdk.keyval_to_unicode(event.keyval):
            if mask:
                # New accelerator
                self.set_accelerator(event.keyval, mask)
                entry.set_text(default(self.current_node.shortcut, ''))
                self['commands'].grab_focus()
            # Capture all `normal characters`
            return True
        else:
            return False

    def on_accelerator_focus_in(self, entry, event):
        pass
        if self.current_node is None:
            return
        if self.current_node.shortcut:
            entry.set_text(_('Type a new accelerator, or press Backspace to clear'))
        else:
            entry.set_text(_('Type a new accelerator'))

    def on_accelerator_focus_out(self, entry, event):
        if self.current_node is not None:
            entry.set_text(default(self.current_node.shortcut, ''))

    def on_tool_manager_dialog_response(self, dialog, response):
        if response == gtk.RESPONSE_HELP:
            gedit.help_display(self.dialog, 'gedit.xml', 'gedit-external-tools-plugin')
            return

        self.on_tool_manager_dialog_focus_out(dialog, None)

        self.dialog.destroy()
        self.dialog = None
        self.tools = None

    def on_tool_manager_dialog_focus_out(self, dialog, event):
        self.save_current_tool()
        for window in gedit.app_get_default().get_windows():
            helper = window.get_data("ExternalToolsPluginWindowData")
            helper.menu.update()

# ex:et:ts=4:
