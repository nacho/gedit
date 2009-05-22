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
import os.path
from library import *
from functions import *
import hashlib
from xml.sax import saxutils
import gobject

class LanguagesPopup(gtk.Window):
    COLUMN_NAME = 0
    COLUMN_ID = 1
    COLUMN_ENABLED = 2

    def __init__(self, languages):
        gtk.Window.__init__(self, gtk.WINDOW_POPUP)
        
        self.set_default_size(200, 200)
        self.props.can_focus = True

        self.build()
        self.init_languages(languages)

        self.show()
        self.map()
        
        self.grab_add()
        
        gtk.gdk.keyboard_grab(self.window, False, 0L)
        gtk.gdk.pointer_grab(self.window, False, gtk.gdk.BUTTON_PRESS_MASK |
                                                 gtk.gdk.BUTTON_RELEASE_MASK |
                                                 gtk.gdk.POINTER_MOTION_MASK |
                                                 gtk.gdk.ENTER_NOTIFY_MASK |
                                                 gtk.gdk.LEAVE_NOTIFY_MASK |
                                                 gtk.gdk.PROXIMITY_IN_MASK |
                                                 gtk.gdk.PROXIMITY_OUT_MASK, None, None, 0L)

        self.view.get_selection().select_path((0,))

    def build(self):
        self.model = gtk.ListStore(str, str, bool)
        
        self.sw = gtk.ScrolledWindow()
        self.sw.show()
        
        self.sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        self.sw.set_shadow_type(gtk.SHADOW_ETCHED_IN)
        
        self.view = gtk.TreeView(self.model)
        self.view.show()
        
        self.view.set_headers_visible(False)
        
        column = gtk.TreeViewColumn()
        
        renderer = gtk.CellRendererToggle()
        column.pack_start(renderer, False)
        column.set_attributes(renderer, active=self.COLUMN_ENABLED)
        
        renderer.connect('toggled', self.on_language_toggled)
        
        renderer = gtk.CellRendererText()
        column.pack_start(renderer, True)
        column.set_attributes(renderer, text=self.COLUMN_NAME)
        
        self.view.append_column(column)
        self.view.set_row_separator_func(self.on_separator)
        
        self.sw.add(self.view)
        
        self.add(self.sw)
    
    def enabled_languages(self, model, path, piter, ret):
        enabled = model.get_value(piter, self.COLUMN_ENABLED)
        
        if path == (0,) and enabled:
            return True

        if enabled:
            ret.append(model.get_value(piter, self.COLUMN_ID))

        return False
    
    def languages(self):
        ret = []
        
        self.model.foreach(self.enabled_languages, ret)
        return ret
    
    def on_separator(self, model, piter):
        val = model.get_value(piter, self.COLUMN_NAME)
        return val == '-'
    
    def init_languages(self, languages):
        manager = gsv.LanguageManager()
        langs = gedit.language_manager_list_languages_sorted(manager, True)
        
        self.model.append([_('All languages'), None, not languages])
        self.model.append(['-', None, False])
        self.model.append([_('Plain Text'), 'plain', 'plain' in languages])
        self.model.append(['-', None, False])
        
        for lang in langs:
            self.model.append([lang.get_name(), lang.get_id(), lang.get_id() in languages])

    def correct_all(self, model, path, piter, enabled):
        if path == (0,):
            return False
        
        model.set_value(piter, self.COLUMN_ENABLED, enabled)

    def on_language_toggled(self, renderer, path):
        piter = self.model.get_iter(path)
        
        enabled = self.model.get_value(piter, self.COLUMN_ENABLED)
        self.model.set_value(piter, self.COLUMN_ENABLED, not enabled)
        
        if path == '0':
            self.model.foreach(self.correct_all, False)
        else:
            self.model.set_value(self.model.get_iter_first(), self.COLUMN_ENABLED, False)

    def do_key_press_event(self, event):
        if event.keyval == gtk.keysyms.Escape:
            self.destroy()
            return True
        else:
            event.window = self.view.get_bin_window()
            return self.view.event(event)
    
    def do_key_release_event(self, event):
        event.window = self.view.get_bin_window()
        return self.view.event(event)
    
    def in_window(self, event, window=None):
        if not window:
            window = self.window

        geometry = window.get_geometry()
        origin = window.get_origin()
        
        return event.x_root >= origin[0] and \
               event.x_root <= origin[0] + geometry[2] and \
               event.y_root >= origin[1] and \
               event.y_root <= origin[1] + geometry[3]
    
    def do_destroy(self):
        gtk.gdk.keyboard_ungrab(0L)
        gtk.gdk.pointer_ungrab(0L)
        
        return gtk.Window.do_destroy(self)
    
    def setup_event(self, event, window):
        fr = event.window.get_origin()
        to = window.get_origin()
        
        event.window = window
        event.x += fr[0] - to[0]
        event.y += fr[1] - to[1]
    
    def resolve_widgets(self, root):
        res = [root]
        
        if isinstance(root, gtk.Container):
            root.forall(lambda x, y: res.extend(self.resolve_widgets(x)), None)
        
        return res
    
    def resolve_windows(self, window):
        if not window:
            return []

        res = [window]
        res.extend(window.get_children())
        
        return res
    
    def propagate_mouse_event(self, event):
        allwidgets = self.resolve_widgets(self.get_child())
        allwidgets.reverse()
        
        orig = [event.x, event.y]

        for widget in allwidgets:
            windows = self.resolve_windows(widget.window)
            windows.reverse()
            
            for window in windows:
                if not (window.get_events() & event.type):
                    continue

                if self.in_window(event, window):                    
                    self.setup_event(event, window)

                    if widget.event(event):
                        return True
        
        return False
    
    def do_button_press_event(self, event):
        if not self.in_window(event):
            self.destroy()
        else:
            return self.propagate_mouse_event(event)

    def do_button_release_event(self, event):
        if not self.in_window(event):
            self.destroy()
        else:
            return self.propagate_mouse_event(event)

    def do_scroll_event(self, event):
        return self.propagate_mouse_event(event)
    
    def do_motion_notify_event(self, event):
        return self.propagate_mouse_event(event)
    
    def do_enter_notify_event(self, event):
        return self.propagate_mouse_event(event)

    def do_leave_notify_event(self, event):
        return self.propagate_mouse_event(event)
    
    def do_proximity_in_event(self, event):
        return self.propagate_mouse_event(event)
    
    def do_proximity_out_event(self, event):
        return self.propagate_mouse_event(event)

gobject.type_register(LanguagesPopup)

class Manager:
    LABEL_COLUMN = 0 # For Combo and Tree
    NODE_COLUMN = 1 # For Tree only
    NAME_COLUMN = 1  # For Combo only

    def __init__(self, datadir):
        self.datadir = datadir
        self.default_size = None
        self.dialog = None
        
        self.build()
    
    def build(self):
        callbacks = {
            'on_new_tool_button_clicked'      : self.on_new_tool_button_clicked,
            'on_remove_tool_button_clicked'   : self.on_remove_tool_button_clicked,
            'on_tool_manager_dialog_response' : self.on_tool_manager_dialog_response,
            'on_tool_manager_dialog_focus_out': self.on_tool_manager_dialog_focus_out,
            'on_accelerator_key_press'        : self.on_accelerator_key_press,
            'on_accelerator_focus_in'         : self.on_accelerator_focus_in,
            'on_accelerator_focus_out'        : self.on_accelerator_focus_out,
            'on_languages_button_clicked'     : self.on_languages_button_clicked
        }

        # Load the "main-window" widget from the ui file.
        self.ui = gtk.Builder()
        self.ui.add_from_file(os.path.join(self.datadir, 'ui', 'tools.ui'))
        self.ui.connect_signals(callbacks)
        self.dialog = self.ui.get_object('tool-manager-dialog')
        
        if self.default_size != None:
            self.dialog.set_default_size(*self.default_size)
        
        self.view = self.ui.get_object('view')
        
        self.__init_tools_model()
        self.__init_tools_view()

        for name in ['input', 'output', 'applicability', 'save-files']:
            self.__init_combobox(name)
        
        self.do_update()
    
    def run(self):
        if self.dialog == None:
            self.build()
            self.dialog.show()
        else:
            self.dialog.present()

    def add_accelerator(self, item):
        if not item.shortcut:
            return
        
        if item.shortcut in self.accelerators:
            self.accelerators[item.shortcut].append(item)
        else:
            self.accelerators[item.shortcut] = [item]

    def remove_accelerator(self, item, shortcut=None):
        if not shortcut:
            shortcut = item.shortcut
            
        if not item.shortcut in self.accelerators:
            return
        
        self.accelerators[item.shortcut].remove(item)
        
        if not self.accelerators[item.shortcut]:
            del self.accelerators[item.shortcut]
        
    def __init_tools_model(self):
        self.tools = ToolLibrary()
        self.current_node = None
        self.script_hash = None
        self.accelerators = dict()

        self.model = gtk.ListStore(str, object)
        self.view.set_model(self.model)

        for item in self.tools.tree.tools:
            self.model.append([item.name, item])
            self.add_accelerator(item)

    def __init_tools_view(self):
        # Tools column
        column = gtk.TreeViewColumn('Tools')
        renderer = gtk.CellRendererText()
        column.pack_start(renderer, False)
        renderer.set_property('editable', True)
        self.view.append_column(column)
        
        column.set_cell_data_func(renderer, self.get_cell_data_cb)

        renderer.connect('edited', self.on_view_label_cell_edited)
        renderer.connect('editing-started', self.on_view_label_cell_editing_started)
        
        self.selection_changed_id = self.view.get_selection().connect('changed', self.on_view_selection_changed, None)

    def __init_combobox(self, name):
        combo = self[name]
        combo.set_active(0)

    # Convenience function to get an object from its name
    def __getitem__(self, key):
        return self.ui.get_object(key)

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
        return hashlib.md5(string).hexdigest()

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
        self.current_node.save_files = combo_value(self, 'save-files')

        buf = self['commands'].get_buffer()
        script  = buf.get_text(*buf.get_bounds())
        h = self.compute_hash(script)
        if h != self.script_hash:
            # script has changed -> save it
            self.current_node.save_with_script([line + "\n" for line in script.splitlines()])
            self.script_hash = h
        else:
            self.current_node.save()

        self.update_remove_revert()

    def clear_fields(self):
        self['accelerator'].set_text('')
        self['commands'].get_buffer().set_text('')

        for nm in ('input', 'output', 'applicability', 'save-files'):
            self[nm].set_active(0)
        
        self['languages_label'].set_text(_('All Languages'))
    
    def fill_languages_button(self):
        if not self.current_node or not self.current_node.languages:
            self['languages_label'].set_text(_('All Languages'))
        else:
            manager = gsv.LanguageManager()
            langs = []
            
            for lang in self.current_node.languages:
                if lang == 'plain':
                    langs.append(_('Plain Text'))
                else:
                    l = manager.get_language(lang)
                    
                    if l:
                        langs.append(l.get_name())
            
            self['languages_label'].set_text(', '.join(langs))
    
    def fill_fields(self):
        node = self.current_node
        self['accelerator'].set_text(default(node.shortcut, ''))

        buf = self['commands'].get_buffer()
        script = default(''.join(node.get_script()), '')
        buf.set_text(script)
        self.script_hash = self.compute_hash(script)
        contenttype = gio.content_type_guess(data=script)
        lmanager = gedit.get_language_manager()
        language = lmanager.guess_language(content_type=contenttype)

        if language is not None:
            buf.set_language(language)
            buf.set_highlight_syntax(True)
        else:
            buf.set_highlight_syntax(False)

        for nm in ('input', 'output', 'applicability', 'save-files'):
            model = self[nm].get_model()
            piter = model.get_iter_first()
            
            self.set_active_by_name(nm,
                                    default(node.__getattribute__(nm.replace('-', '_')),
                                    model.get_value(piter, self.NAME_COLUMN)))

        self.fill_languages_button()

    def update_remove_revert(self):
        piter, node = self.get_selected_tool()

        removable = node is not None and node.is_local()

        self['remove-tool-button'].set_sensitive(removable)
        self['revert-tool-button'].set_sensitive(removable)

        if node is not None and node.is_global():
            self['remove-tool-button'].hide()
            self['revert-tool-button'].show()
        else:
            self['remove-tool-button'].show()
            self['revert-tool-button'].hide()

    def do_update(self):
        self.update_remove_revert()

        piter, node = self.get_selected_tool()

        if node is not None:
            self.current_node = node
            self.fill_fields()
            self['tool-table'].set_sensitive(True)
        else:
            self.clear_fields()
            self['tool-table'].set_sensitive(False)       

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
            shortcut = node.shortcut
            
            if node.parent.revert_tool(node):
                self.remove_accelerator(node, shortcut)
                self.add_accelerator(node)

                self['revert-tool-button'].set_sensitive(False)
                self.fill_fields()
                
                self.model.row_changed(self.model.get_path(piter), piter)
        else:
            if node.parent.delete_tool(node):
                self.remove_accelerator(node)
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
            self.save_current_tool()

    def on_view_label_cell_editing_started(self, renderer, editable, path):
            piter = self.model.get_iter(path)
            label = self.model.get_value(piter, self.LABEL_COLUMN)

            if isinstance(editable, gtk.Entry):
                editable.set_text(label)
                editable.grab_focus()
    
    def on_view_selection_changed(self, selection, userdata):
        self.save_current_tool()
        self.do_update()

    def accelerator_collision(self, name, node):
        if not name in self.accelerators:
            return []
            
        ret = []
        
        for other in self.accelerators[name]:
            if not other.languages or not node.languages:
                ret.append(other)
                continue
            
            for lang in other.languages:
                if lang in node.languages:
                    ret.append(other)
                    continue
        
        return ret

    def set_accelerator(self, keyval, mod):
        # Check whether accelerator already exists
        self.remove_accelerator(self.current_node)

        name = gtk.accelerator_name(keyval, mod)

        if name == '':
            self.current_node.shorcut = None
            return True
            
        col = self.accelerator_collision(name, self.current_node)
        
        if col:
            dialog = gtk.MessageDialog(self.dialog,
                                       gtk.DIALOG_MODAL,
                                       gtk.MESSAGE_ERROR,
                                       gtk.BUTTONS_OK,
                                       _('This accelerator is already bound to %s') % (', '.join(map(lambda x: x.name, col)),))

            dialog.run()
            dialog.destroy()
            return False

        self.current_node.shortcut = name
        self.add_accelerator(self.current_node)

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
            if self.set_accelerator(event.keyval, mask):
                entry.set_text(default(self.current_node.shortcut, ''))
                self['commands'].grab_focus()
    
            # Capture all `normal characters`
            return True
        elif gtk.gdk.keyval_to_unicode(event.keyval):
            if mask:
                # New accelerator
                if self.set_accelerator(event.keyval, mask):
                    entry.set_text(default(self.current_node.shortcut, ''))
                    self['commands'].grab_focus()
            # Capture all `normal characters`
            return True
        else:
            return False

    def on_accelerator_focus_in(self, entry, event):
        if self.current_node is None:
            return
        if self.current_node.shortcut:
            entry.set_text(_('Type a new accelerator, or press Backspace to clear'))
        else:
            entry.set_text(_('Type a new accelerator'))

    def on_accelerator_focus_out(self, entry, event):
        if self.current_node is not None:
            entry.set_text(default(self.current_node.shortcut, ''))
            
            piter, node = self.get_selected_tool()
            self.model.row_changed(self.model.get_path(piter), piter)

    def on_tool_manager_dialog_response(self, dialog, response):
        if response == gtk.RESPONSE_HELP:
            gedit.help_display(self.dialog, 'gedit', 'gedit-external-tools-plugin')
            return

        self.on_tool_manager_dialog_focus_out(dialog, None)
        self.default_size = [self.dialog.allocation.width, self.dialog.allocation.height]
        
        self.dialog.destroy()
        self.dialog = None
        self.tools = None

    def on_tool_manager_dialog_focus_out(self, dialog, event):
        self.save_current_tool()
        for window in gedit.app_get_default().get_windows():
            helper = window.get_data("ExternalToolsPluginWindowData")
            helper.menu.update()
   
    def get_cell_data_cb(self, column, cell, model, iter):
        lbl = model.get_value(iter, self.LABEL_COLUMN)
        node = model.get_value(iter, self.NODE_COLUMN)

        escaped = saxutils.escape(lbl)

        if node.shortcut:
            markup = '%s (<b>%s</b>)' % (escaped, saxutils.escape(node.shortcut))
        else:
            markup = escaped
            
        cell.set_property('markup', markup)

    def update_languages(self, popup):
        self.current_node.languages = popup.languages()
        self.g()
        
        self.fill_languages_button()

    def on_languages_button_clicked(self, button):
        popup = LanguagesPopup(self.current_node.languages)
        popup.set_transient_for(self.dialog)
        
        origin = button.window.get_origin()
        popup.move(origin[0], origin[1] - popup.allocation.height)
        
        popup.connect('destroy', self.update_languages)

# ex:et:ts=4:
