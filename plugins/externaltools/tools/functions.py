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

import ElementTree as et
import os
import gtk
from gtk import gdk
import gnomevfs
import gedit
from gettext import gettext as _
from outputpanel import OutputPanel
from capture import *

# ==== Data storage related functions ====

class ToolsTree:
	def __init__(self):
		self.xml_location = os.path.join(os.environ['HOME'], '.gnome2/gedit')

		if not os.path.isdir(self.xml_location):
			os.mkdir(self.xml_location)

		self.xml_location = os.path.join(self.xml_location, 'gedit-tools.xml')
		
		try:
			if not os.path.isfile(self.xml_location):
				self.tree = et.parse(self.get_stock_file())
			else:
				self.tree = et.parse(self.xml_location)
			self.root = self.tree.getroot()
		except:
			self.root = et.Element('tools')
			self.tree = et.ElementTree(self.root)
	
	def __iter__(self):
		return iter(self.root)
	
	def get_stock_file(self):
		dirs = os.getenv('XDG_DATA_DIR')
		if dirs:
			dirs = dirs.split(os.pathsep)
		else:
			dirs = ('/usr/local/share', '/usr/share')
		
		for d in dirs:
			f = os.path.join(d, 'gedit-2/plugins/externaltools/stock-tools.xml')
			if os.path.isfile(f):
				return f

		return None
	
	def save(self):
		self.tree.write(self.xml_location)
		
	def get_tool_from_accelerator(self, keyval, mod, ignore = None):
		if not self.root:
			return None
		
		for tool in self.root:
			if tool != ignore:
				skey, smod = gtk.accelerator_parse(tool.get('accelerator'))
				if skey == keyval and smod & mod == smod:
					return tool
		return None


def default(val, d):
	if val is not None:
		return val
	else:
		return d

# ==== UI related functions ====
def insert_tools_menu(window, tools = None):
	window_data = dict()	
	window.set_data("ToolsPluginCommandsData", window_data)

	if tools is None:
		tools = ToolsTree()

	manager = window.get_ui_manager()

	window_data['action_group'] = gtk.ActionGroup("GeditToolsPluginCommandsActions")
	window_data['action_group'].set_translation_domain('gedit')
	window_data['ui_id'] = manager.new_merge_id()

	i = 0;
	for tool in tools:
		menu_id = "ToolCommand%06d" % i
		window_data['action_group'].add_actions([(menu_id,
		                                          None,
		                                          tool.get('label'),
		                                          tool.get('accelerator'),
		                                          tool.get('description'),
		                                          capture_menu_action)],
		                                        (window, tool))
		manager.add_ui(window_data['ui_id'],
		               '/MenuBar/ToolsMenu/ToolsOps_4',
		               menu_id, 
		               menu_id,
		               gtk.UI_MANAGER_MENUITEM,
		               False)
		i = i + 1

	manager.insert_action_group(window_data['action_group'], -1)

def remove_tools_menu(window):
	window_data = window.get_data("ToolsPluginCommandsData")

	manager = window.get_ui_manager()
	manager.remove_ui(window_data['ui_id'])
	manager.remove_action_group(window_data['action_group'])
	window.set_data("ToolsPluginCommandsData", None)

def update_tools_menu(tools = None):
	for window in gedit.gedit_app_get_default().get_windows():
		remove_tools_menu(window)
		insert_tools_menu(window, tools)
		window.get_ui_manager().ensure_update()

# ==== Capture related functions ====
def capture_menu_action(action, window, node):
	# Get the panel
	panel = window.get_data("ToolsPluginWindowData")["output_buffer"]
	panel.show()
	panel.clear()

	view = window.get_active_view()
	document = view.get_buffer()
	if document is None:
		# :TODO: Allow command on no document
		panel.write("No document\n", panel.command_tag)
		return

	uri = document.get_uri()
	if uri is None:
		# :TODO: Allow command on unnamed documents
		panel.write("Current document has no file name\n", panel.command_tag)
		return

	# Configure capture environment
	path = gnomevfs.get_local_path_from_uri(uri)
 	cwd = os.path.dirname(path)

 	capture = Capture(node.text, cwd)
 	capture.set_flags(capture.CAPTURE_BOTH)

	documents_uri = [document.get_uri()
	                         for document in window.get_documents()
	                         if document.get_uri() is not None]
	documents_path = [gnomevfs.get_local_path_from_uri(uri)
	                         for uri in documents_uri]

 	capture.env = os.environ.copy()
 	capture.set_env(GEDIT_CURRENT_DOCUMENT_URI  = uri,
 	                GEDIT_CURRENT_DOCUMENT_PATH = path,
 	                GEDIT_CURRENT_DOCUMENT_NAME = os.path.basename(path),
 	                GEDIT_CURRENT_DOCUMENT_DIR  = cwd,
 	                GEDIT_DOCUMENTS_URI         = ' '.join(documents_uri),
 	                GEDIT_DOCUMENTS_PATH        = ' '.join(documents_path))
 	
	# Assign the error output to the output panel
	panel.process = capture
	
	capture.connect('stderr-line',   capture_stderr_line_panel,   panel)
	capture.connect('begin-execute', capture_begin_execute_panel, panel, node.get('label'))
	capture.connect('end-execute',   capture_end_execute_panel,   panel, view)

	# Get input text
	input_type = default(node.get('input'), 'nothing')
	output_type = default(node.get('output'), 'output-panel')

	if input_type != 'nothing':
		if input_type == 'document':
			start, end = document.get_bounds()
		elif input_type == 'selection':
			try:
				start, end = document.get_selection_bounds()
			except ValueError:
				start, end = document.get_bounds()
				if output_type == 'replace-selection':
					document.select_range(start, end)
		elif input_type == 'line':
			start = document.get_insert()
			end = insert.copy()
			if not start.starts_line():
				start.backward_line()
			if not end.ends_line():
				end.forward_to_line_end()
		elif input_type == 'word':
			start = document.get_insert()
			end = insert.copy()
			if not start.inside_word():
				panel.write(_('You must be inside a word to run this command'),
				            panel.command_tag)
				return
			if not start.starts_word():
				start.backward_word_start()
			if not end.ends_word():
				end.forward_word_end()
			
		input_text = document.get_text(start, end)
		capture.set_input(input_text)
	
	# Assign the standard output to the chosen "file"
	if output_type == 'output-panel':
		capture.connect('stdout-line', capture_stdout_line_panel, panel)
		document.begin_user_action()
	elif output_type == 'new-document':
		tab = window.create_tab(True)
		view = tab.get_view()
		document = tab.get_document()
		pos = document.get_start_iter()
		capture.connect('stdout-line', capture_stdout_line_document, document, pos)
		document.begin_user_action()
		view.set_editable(False)
		view.set_cursor_visible(False)
	else:
		document.begin_user_action()
		view.set_editable(False)
		view.set_cursor_visible(False)
		
		if output_type == 'insert':
			pos = document.get_iter_at_mark(document.get_mark('insert'))
		elif output_type == 'replace-selection':
			document.delete_selection(False, False)
			pos = document.get_iter_at_mark(document.get_mark('insert'))
		elif output_type == 'replace-document':
			document.set_text('')
			pos = document.get_end_iter()
		else:
			pos = document.get_end_iter()
		capture.connect('stdout-line', capture_stdout_line_document, document, pos)
	
	# Run the command
	view.get_window(gtk.TEXT_WINDOW_TEXT).set_cursor(gdk.Cursor(gdk.WATCH))
	capture.execute()
	document.end_user_action()

def capture_stderr_line_panel(capture, line, panel):
	panel.write(line, panel.error_tag)

def capture_begin_execute_panel(capture, panel, label):
	panel['stop'].set_sensitive(True)
	panel.clear()
	panel.write("Running %s...\n" % label, panel.command_tag)

def capture_end_execute_panel(capture, exit_code, panel, view):
	panel.write("Exit code: %s\n" % exit_code, panel.command_tag)
	panel['stop'].set_sensitive(False)
	view.get_window(gtk.TEXT_WINDOW_TEXT).set_cursor(gdk.Cursor(gdk.XTERM))
	view.set_cursor_visible(True)
	view.set_editable(True)

def capture_stdout_line_panel(capture, line, panel):
	panel.write(line)

def capture_stdout_line_document(capture, line, document, pos):
	document.insert(pos, line)

