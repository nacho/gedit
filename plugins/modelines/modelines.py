# -*- coding: utf-8 -*-
# modelines.py - Emacs, Kate and Vim-style modelines support for gedit.
#
# Copyright (C) 2005 - Steve Frécinaux
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

import gedit
import re
import gtk

class ModelinePlugin(gedit.Plugin):
	def __init__(self):
		gedit.Plugin.__init__(self)

	def connect_handlers(self, view):
		doc = view.get_buffer()
		loaded_id = doc.connect("loaded", apply_modeline, view)
		saved_id  = doc.connect("saved", apply_modeline, view)
		doc.set_data("ModelinePluginHandlerIds", (loaded_id, saved_id))
	
	def disconnect_handlers(self, view):
		doc = view.get_buffer()
		loaded_id, saved_id = doc.get_data("ModelinePluginHandlerIds")
		doc.disconnect(loaded_id)
		doc.disconnect(saved_id)
		doc.set_data("ModelinePluginHandlerIds", None)
	
	def activate(self, window):
		for view in window.get_views(): 
			self.connect_handlers(view)
			apply_modeline(doc, None, view)
			
		tab_added_id = window.connect("tab_added",
		                              lambda w, t: self.connect_handlers(t.get_view()))
		window.set_data("ModelinePluginHandlerId", tab_added_id)
	
	def deactivate(self, window):
		tab_added_id = window.get_data("ModelinePluginHandlerId")
		window.disconnect(tab_added_id)
		window.set_data("ModelinePluginHandlerId", None)
		
		for view in window.get_views():
			self.disconnect_handlers(view)

def apply_modeline(doc, error, view):
	if error is None:
		options = get_modeline_options(doc)
		apply_options(options, view)
		doc.set_data("ModelineOptions", options)

def get_modeline_options(doc):
	options = dict()

	# Search the two first lines for emacs- or vim-style modelines
	start = doc.get_start_iter()
	for i in (1, 2):
		end = start.copy()
		if not end.forward_to_line_end():
			return options
		modeline = doc.get_text(start, end, False)
		options.update(parse_modeline_kate(modeline))
		options.update(parse_modeline_vim(modeline))
		options.update(parse_modeline_emacs(modeline))
		start = end

	# Vim can read modeline on the 3rd line
	end = start.copy()
	if not end.forward_to_line_end():
		return options
	modeline = doc.get_text(start, end, False)
	options.update(parse_modeline_kate(modeline))
	options.update(parse_modeline_vim(modeline))
	start = end

	# Kate reads modelines on the 10 first lines
	for i in (4, 5, 6, 7, 8, 9, 10):
		end = start.copy()
		if not end.forward_to_line_end():
			break
		modeline = doc.get_text(start, end, False)
		options.update(parse_modeline_kate(modeline))
		start = end

	# Search the three last lines for vim- and kate-style modelines
	start = doc.get_end_iter()
	start.backward_lines(2)	
	for i in (1, 2, 3):
		end = start.copy()
		if not end.forward_to_line_end():
			return options
		modeline = doc.get_text(start, end, False)
		options.update(parse_modeline_kate(modeline))
		options.update(parse_modeline_vim(modeline))
		start = end

	# Kate reads modelines on the 10 last lines
	start.backward_lines(9)	
	for i in (4, 5, 6, 7, 8, 9, 10):
		end = start.copy()
		if not end.forward_to_line_end():
			return options
		modeline = doc.get_text(start, end, False)
		options.update(parse_modeline_kate(modeline))
		start = end
	
	return options

def parse_modeline_vim(line):
	# First form modeline
	# [text]{white}{vi:|vim:|ex:}[white]{options}
	#
	# Second form
	# [text]{white}{vi:|vim:|ex:}[white]se[t] {options}:[text]

	# Find the modeline
	for i in ('ex:', 'vi:', 'vim:'):
		start = line.find(i)
		if start != -1:	
			start = start + len(i)
			break
	else:
		return {}
	
	if line[start : start + 2] == "se":
		end = line.find(":", start + 3)
		if end == -1:
			return {}
		elif line[start + 2] == 't':
			line = line[start + 4 : end]
		else:
			line = line[start + 3 : end]
	else:
		line = line[start :]
	
	# Get every individual directives
	directives = [x for x in re.split(r'[: ]', line) if x != '']
	options = dict();
	
	for directive in directives:
		# Reformat the directive
		if (directive.find("=") != -1):
			name, value = directive.split("=")
		elif directive[0:2] == "no":
			value = False
			name = directive[2:]
		else:
			value = True
			name = directive
		
		# Set the options according to the directive
		if name == "et" or name == "expandtab":
			options["use-tabs"] = not value
		elif name == "ts" or name == "tabstop":
			options["tabs-width"] = int(value)
		elif name == "wrap":
			options["wrapping"] = value
		elif name == "textwidth":
			options["margin"] = int(value)
	return options

def parse_modeline_emacs(line):
	# cf. http://www.delorie.com/gnu/docs/emacs/emacs_486.html
	# Extract the modeline from the given line
	start = line.find("-*-")
	if start == -1:
		return {}
	else:
		start = start + 3
	end = line.find("-*-", start)
	if end == -1:
		return {}
	
	# Get every individual directives
	directives = [x for x in [i.strip() for i in line[start : end].split(";")] if x != '']
	options = dict()
	
	for directive in directives:
		name, value = ([i.strip() for i in directive.split(":", 1)] + [None])[0:2]
		if name == "tab-width":
			options["tabs-width"] = int(value)
		elif name == "indent-tabs-mode":
			options["use-tabs"] = value != "nil"
		elif name == "autowrap":
			options["wrapping"] = value != "nil"
	return options

def parse_modeline_kate(line):
	# cf. http://wiki.kate-editor.org/index.php/Modelines

	# Extract the modeline from the given line
	start = line.find("kate:")
	if start == -1:
		return {}
	else:
		start = start + 5
	end = line.rfind(";")
	if end == -1:
		return {}
	
	# Get every individual directives
	directives = [x for x in [i.strip() for i in line[start : end].split(";")] if x != '']
	options = dict()
	
	for directive in directives:
		name, value = ([i.strip() for i in directive.split(None, 1)] + [None])[0:2]
		if name == "tab-width" or name == "indent-width":
			options["tabs-width"] = int(value)
		elif name == "space-indent":
			options["use-tabs"] = value not in ("on", "true", "1")
		elif name == "word-wrap":
			options["wrapping"] = value in ("on", "true", "1")
		elif name == "word-wrap-column":
			options["margin"] = int(value)
	return options

def apply_options(options, view):
	if not options:
		return

	if options.has_key("tabs-width"):
		view.set_tabs_width(options["tabs-width"])
	
	if options.has_key("use-tabs"):
		view.set_insert_spaces_instead_of_tabs(not options["use-tabs"])

	if options.has_key("wrapping"):
		if options["wrapping"]:
			view.set_wrap_mode(gtk.WRAP_WORD)
		else:
			view.set_wrap_mode(gtk.WRAP_NONE)

	if options.has_key("margin"):
		if options["margin"] > 0:
			view.set_margin(long(options["margin"]))
			view.set_show_margin(True)
		else:
			view.set_show_margin(False)

# ex:ts=8:noet:
# kate: word-wrap-column 120;
