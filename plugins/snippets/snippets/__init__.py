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
    
import gedit
import gtk
from gtk import gdk
import sys
import os
import shutil
from xml.dom import minidom
import xml
from SnippetPluginInstance import SnippetsPluginInstance
from SnippetsDialog import SnippetsDialog
from Snippet import Snippet

class SnippetsPlugin(gedit.Plugin):
	XML_FILE = 'gedit-snippets.xml'
	
	def __init__(self):
		gedit.Plugin.__init__(self)
		self.dlg = None
		self.xml_error_message = \
				'An error has occured during loading the ' + self.XML_FILE + \
				' file. The file is probably corrupt. Please fix or ' \
				'remove	the file. To try to load the file again select ' \
				'the Tools -> Snippets menu item.'

		self.show_xml_error = not self.load_xml()
	
	def idle_error_message(self, window):
		message_dialog(window, gtk.MESSAGE_ERROR, self.xml_error_message)
		return False

	def activate(self, window):
		data = SnippetsPluginInstance(self)
		window._snippets_plugin_data = data
		data.run(window)
		
		if self.show_xml_error:
			self.show_xml_error = None
			gobject.idle_add(self.idle_error_message, window)

	def deactivate(self, window):
		window._snippets_plugin_data.stop()
		window._snippets_plugin_data = None
		
	def update_ui(self, window):
		window._snippets_plugin_data.ui_changed()
	
	def create_configure_dialog(self):
		if not self.xmldoc and not self.plugin.load_xml():
			message_dialog(None, gtk.MESSAGE_ERROR, \
					self.xml_error_message)
			return None
		
		if not self.dlg:
			self.dlg = SnippetsDialog(self)
		else:
			self.dlg.run()
		
		window = gedit.gedit_app_get_default().get_active_window()
		
		if window:
			self.dlg.dlg.set_transient_for(window)
		
		return self.dlg.dlg
	
	def save_xml(self):
		if not self.xmldoc:
			return

		dest = open(self.xml_location + '/' + self.XML_FILE, 'w')
		self.xmldoc.writexml(dest)
		dest.close()
	
	def copy_system_snippets(self):
		# Copies the system snippets file to the users home dir locations
		# Only a temporary solution to having a system wide snippets collection
		dirs = None
		
		if 'XDG_DATA_DIRS' in os.environ:
			dirs = os.environ['XDG_DATA_DIRS']
		
		if not dirs:
			dirs = '/usr/local/share:/usr/share'

		dirs = dirs.split(':')
		
		for d in dirs:
			f = d + '/gedit-2/plugins/snippets/' + self.XML_FILE
			# print 'Checking: ' + f
			if os.path.isfile(f):
				try:
					# Copy the system file to the user directory
					shutil.copyfile(f, self.xml_location + '/' + self.XML_FILE)
					# print 'Copied: ' + f
				except IOError:
					# print 'Error: ' + f
					return False

				return True
		
		# print 'not found'
		return False			
	
	def load_xml(self):
		self.xml_location = os.environ['HOME'] + '/.gnome2/gedit'
		self.languages = {}
		self.accel_groups = {}
		self.language_all = []

		if not os.path.isdir(self.xml_location):
			# try creating it
			os.mkdir(self.xml_location)
			
		if not os.path.isfile(self.xml_location + '/' + self.XML_FILE) and \
				not self.copy_system_snippets():
			# This really shouldn't happen, but it could
			self.xmldoc = minidom.Document()
			self.xmldoc.appendChild(self.xmldoc.createElement('snippets'))
			self.save_xml()
		else:
			try:
				self.xmldoc = minidom.parse(self.xml_location + '/' + self.XML_FILE)
			except xml.parsers.expat.ExpatError:
				self.xmldoc = None
				return False
			
			for ln in self.xmldoc.firstChild.childNodes:
				if ln.localName == 'snippet':
					self.language_all.append(ln)
		
		return True

	def dummy(group, window, keyval, mod):
		return
	
	def create_accel_group(self, nm):
		accel_group = gtk.AccelGroup()
		snippets = None
		
		if nm == None:
			snippets = self.language_all
		else:
			lang = self.lookup_language(nm)
			
			if lang:
				snippets = lang.getElementsByTagName('snippet')
		
		if snippets:
			for snippet in snippets:
				# Create the binding
				s = Snippet(snippet, self)
				(keyval, mod) = gtk.accelerator_parse(s['accelerator'])
				
				if gtk.accelerator_valid(keyval, mod):
					accel_group.connect_group(keyval, mod, 0, \
							lambda a, b, c, d: None)

		return accel_group
	
	def get_snippet_from_accelerator(self, snippets, keyval, mod, ignore = None):
		if snippets:
			for snippet in snippets:
				if snippet == ignore:
					continue

				s = Snippet(snippet, self)
				(skey, smod) = gtk.accelerator_parse(s['accelerator'])
				
				if skey == keyval and smod & mod == smod:
					return s
		
		return None
	
	def language_accel_group(self, nm):
		if self.xmldoc == None:
			return None
		
		if nm in self.accel_groups:
			return self.accel_groups[nm]
		
		accel_group = self.create_accel_group(nm)
		self.accel_groups[nm] = accel_group
		
		return accel_group
		
	def lookup_language(self, nm):
		if self.xmldoc == None:
			return None
			
		if nm in self.languages:
			return self.languages[nm]

		lang = None
		
		for ln in self.xmldoc.getElementsByTagName('language'):
			if ln.attributes.has_key('id') and \
					ln.attributes['id'].nodeValue == nm:
				lang = self.languages[nm] = ln
				break
		
		if not lang:
			# this just means that we've looked it up and it didn't exist.
			# next time we don't have to look it up, it just doesn't exist.
			self.languages[nm] = None

		return lang
