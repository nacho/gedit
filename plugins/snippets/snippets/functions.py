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

import gtk
from xml.sax import saxutils

def message_dialog(par, typ, msg):
	d = gtk.MessageDialog(par, gtk.DIALOG_MODAL, typ, \
			gtk.BUTTONS_OK, msg)
	d.run()
	d.destroy()

class outfile:
	def __init__(self, fn):
		self.__fn = fn
		self.__text = ''
		
	def close(self): pass
	flush = close
	def fileno(self):    return self.__fn
	def isatty(self):    return 0
	def read(self, a):   return ''
	def readline(self):  return ''
	def readlines(self):
		l = self.__text.split('\n')
		self.__text = ''
		return l
	def write(self, s):
		#stdout.write(str(self.__w.get_point()) + '\n')
		self.__text += s
	def writelines(self, l):
		self.__text += str.join('', l)
	def seek(self, a):   raise IOError, (29, 'Illegal seek')
	def tell(self):      raise IOError, (29, 'Illegal seek')
	truncate = tell

def compute_indentation(view, piter):
	line = piter.get_line()
	start = view.get_buffer().get_iter_at_line(line)
	end = start.copy()
	
	ch = end.get_char()
	
	while (ch.isspace() and ch != '\r' and ch != '\n' and \
			end.compare(piter) < 0):
		if not end.forward_char():
			break;
		
		ch = end.get_char()
	
	if start.equal(end):
		return ''
	
	return start.get_slice(end)

def markup_escape(text):
	return saxutils.escape(text)

def insert_with_indent(view, piter, text, indentfirst = True):
	lines = text.split('\n')

	if len(lines) == 1:
		view.get_buffer().insert(piter, text)
	else:
		# Compute indentation
		indent = compute_indentation(view, piter)
		text = ''
		
		for i in range(0, len(lines)):
			if indentfirst or i > 0:
				text += indent + lines[i] + '\n'
			else:
				text += lines[i] + '\n'
		
		view.get_buffer().insert(piter, text[:-1])
