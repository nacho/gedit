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

import gtk
from gtk import glade
import os.path
from gettext import gettext as _

from functions import *
import ElementTree as et

class Manager:
	GLADE_FILE = os.path.join(os.path.dirname(__file__), "tools.glade")
	GCONF_DIR = '/apps/gedit-2/plugins/tools'


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
			('output-panel'     , _('Insert in output panel')),
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

	def __init__(self):
		if Manager.__shared_state is not None:
			self.__dict__ = Manager.__shared_state
			if self.dialog: return
		else:
			Manager.__shared_state = self.__dict__
		
		callbacks = {
			'on_new_tool_button_clicked'     : self.on_new_tool_button_clicked,
			'on_remove_tool_button_clicked'  : self.on_remove_tool_button_clicked,
			'on_tool_manager_dialog_response': self.on_tool_manager_dialog_response,
			'on_accelerator_key_press'       : self.on_accelerator_key_press,
			'on_accelerator_focus_in'        : self.on_accelerator_focus_in,
			'on_accelerator_focus_out'       : self.on_accelerator_focus_out
		}
		
		# Load the "main-window" widget from the glade file.
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
		self.tools = ToolsTree()
		self.current_node = None

		self.model = gtk.ListStore(str, object)
		self.view.set_model(self.model)

		for item in self.tools:
			self.model.append([item.get('label'), item])
		
		self.row_changed_id = self.model.connect('row-changed', self.on_tools_model_row_changed)

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
	
	def __getitem__(self, key):
		"""Convenience function to get a widget from its name"""
		return self.ui.get_widget(key)

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

	def save_current_tool(self):
		if self.current_node is None:
			return

		buf = self['commands'].get_buffer()
		self.current_node.text = buf.get_text(buf.get_start_iter(),
		                                      buf.get_end_iter())

		self.current_node.set('description',
			              self['description'].get_text())

		for name in ('input', 'output', 'applicability'):
			combo = self[name]
			self.current_node.set(name,
			                      combo.get_model().get_value(
			                             combo.get_active_iter(),
			                             self.NAME_COLUMN))

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
		self['description'].set_text(default(node.get('description'), _('A Brand New Tool')))
		self['accelerator'].set_text(default(node.get('accelerator'), ''))
		self['commands'].get_buffer().set_text(default(node.text, ''))
		
		self.set_active_by_name('input',
		                        default(node.get('input'),
		                                Manager.combobox_items['input'][0][0]))
		self.set_active_by_name('output',
		                        default(node.get('output'),
		                                Manager.combobox_items['output'][0][0]))
		self.set_active_by_name('applicability',
		                        default(node.get('applicability'),
		                                Manager.combobox_items['applicability'][0][0]))
		self['title'].set_label(_('Edit tool <i>%s</i>:') % node.get('label'))

	def on_new_tool_button_clicked(self, button):
		self.save_current_tool()

		# block handlers while inserting a new item
		self.model.handler_block(self.row_changed_id)
		self.view.get_selection().handler_block(self.selection_changed_id)

		self.current_node = et.Element('tool');
		self.current_node.set('label', _('New tool'))
		self.tools.root.append(self.current_node)
		piter = self.model.append([self.current_node.get('label'),
		                           self.current_node])
		self.view.set_cursor(self.model.get_path(piter),
		                     self.view.get_column(self.LABEL_COLUMN),
		                     True)
		self.fill_fields()
		self['tool-table'].set_sensitive(True)

		self.view.get_selection().handler_unblock(self.selection_changed_id)
		self.model.handler_unblock(self.row_changed_id)

	def on_remove_tool_button_clicked(self, button):
		piter, node = self.get_selected_tool()
		self.tools.root.remove(node)
		self.current_node = None
		if self.model.remove(piter):
			self.view.set_cursor(self.model.get_path(piter),
			                     self.view.get_column(self.LABEL_COLUMN),
			                     False)
			self.view.grab_focus()

	def on_view_label_cell_edited(self, cell, path, new_text):
		if new_text != '':
			piter = self.model.get_iter(path)
			node = self.model.get_value(piter, self.NODE_COLUMN)
			node.set("label", new_text)
			self.model.set(piter, self.LABEL_COLUMN, new_text)
			self['title'].set_label(_('Edit tool <i>%s</i>:') % new_text)

	def on_view_selection_changed(self, selection, userdata):
		self.save_current_tool()
		piter, node = self.get_selected_tool()
		
		self['remove-tool-button'].set_sensitive(piter is not None)

		if node is not None:
			self.current_node = node
			self.fill_fields()
			self['tool-table'].set_sensitive(True)
		else:
			self.clear_fields()
			self['tool-table'].set_sensitive(False)

	def on_tools_model_row_changed(self, model, path, piter):
		tool = model.get_value(piter, self.NODE_COLUMN)
		if tool is not self.tools.root[path[0]]:
			if tool in self.tools.root.items():
				self.tools.root.remove(tool)
			self.tools.root.insert(path[0], tool)

	def set_accelerator(self, keyval, mod):
		# Check whether accelerator already exists
		
 		tool = self.tools.get_tool_from_accelerator(keyval,
 		                                            mod, 
 		                                            self.current_node)
 		if tool is not None:
 			message_dialog(self.dlg, gtk.MESSAGE_ERROR,
 			               __('This accelerator is already bound to %s') %
 			                  self.current_node.get('label'))
 			return False
 		
		name = gtk.accelerator_name(keyval, mod)
 		if name != '':
			self.current_node.set('accelerator', name)
		else:
			self.current_node.set('accelerator', None)
		return True

	def on_accelerator_key_press(self, entry, event):
		mask = event.state & gtk.accelerator_get_default_mod_mask()

		if event.keyval == gtk.keysyms.Escape:
			entry.set_text(self.current_node.get('accelerator'))
			self['commands'].grab_focus()
			return True
		elif event.keyval == gtk.keysyms.Delete \
		  or event.keyval == gtk.keysyms.BackSpace:
			entry.set_text('')
			self.current_node.set('accelerator', '')
			self['commands'].grab_focus()
			return True
		elif event.keyval in range(gtk.keysyms.F1, gtk.keysyms.F12):
			# New accelerator
			self.set_accelerator(event.keyval, mask)
			entry.set_text(self.current_node.get('accelerator'))
			self['commands'].grab_focus()
			# Capture all `normal characters`
			return True			
		elif gtk.gdk.keyval_to_unicode(event.keyval):
			if mask:
				# New accelerator
				self.set_accelerator(event.keyval, mask)
				entry.set_text(self.current_node.get('accelerator'))
				self['commands'].grab_focus()
			# Capture all `normal characters`
			return True
		else:
			return False
	
	def on_accelerator_focus_in(self, entry, event):
		if self.current_node is None:
			return
		if self.current_node.get('accelerator'):
			entry.set_text('Type a new accelerator, or press Backspace to clear')
		else:
			entry.set_text('Type a new accelerator')
	
	def on_accelerator_focus_out(self, entry, event):
		if self.current_node is not None:
			entry.set_text(default(self.current_node.get('accelerator'), ''))

	def on_tool_manager_dialog_response(self, dialog, response):
		if response == gtk.RESPONSE_HELP:
			# Show help
			return

		self.save_current_tool()

		self.dialog.destroy()
		self.dialog = None

		self.tools.save()
		update_tools_menu(tools = self.tools)
		self.tools = None

# ex:noet:ts=8:
