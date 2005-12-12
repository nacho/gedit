import gedit

class ConsolePlugin(gedit.Plugin):

	def __init__(self):
		gedit.Plugin.__init__(self)
			
	def activate(self, window):
		# Build the console widget
		cons = init_console(window)
		
		bottom = window.get_bottom_panel()
		image = gtk.Image()
		image.set_from_icon_name("gnome-mime-text-x-python", gtk.ICON_SIZE_BUTTON)
		bottom.add_item(cons, "Python Console", image)
			
		# Store data in the window object
		window._pythonconsole_window_data = (cons)
	
	def deactivate(self, window):
		# Retreive the data of the window object
		cons = window._pythonconsole_window_data
		del window._pythonconsole_window_data
		
		# Shutdown the console
		bottom = window.get_bottom_panel()
		bottom.remove_item(cons)
		
	def update_ui(self, window):
		pass

def init_console(window, quit_cb=None):
	console = Console(namespace={'__builtins__' : __builtins__, 'window' : window}, quit_cb=quit_cb)
	console.init()
	console.line.set_text("import gedit")
	console.eval()
	console.line.set_text("print 'You can use the window name:', window")
	console.eval()
	return console
		
## Begin shameless code stealing from ephy's console.py extension
## Code is taken as-is and can thus be updated at will by copy/pasting
## File: epiphany-extensions/extensions/python-console/console.py

#   Interactive Python-GTK Console
#   Copyright (C), 1998 James Henstridge <james@daa.com.au>
#   Copyright (C), 2005 Adam Hooper <adamh@densi.com>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

# This module implements an interactive python session in a GTK window.  To
# start the session, use the gtk_console command.  Its specification is:
#   gtk_console(namespace, title)
# where namespace is a dictionary representing the namespace of the session,
#       title     is the title on the window and
#
# As well as the starting attributes in namespace, the session will also
# have access to the list __history__, which is the command history.

import sys, string, traceback

import gobject
import gtk
import pango
import gtk.keysyms

stdout = sys.stdout

if not hasattr(sys, 'ps1'): sys.ps1 = '>>> '
if not hasattr(sys, 'ps2'): sys.ps2 = '... '

# some functions to help recognise breaks between commands
def remQuotStr(s):
	'''Returns s with any quoted strings removed (leaving quote marks)'''
	r = ''
	inq = 0
	qt = ''
	prev = '_'
	while len(s):
		s0, s = s[0], s[1:]
		if inq and (s0 != qt or prev == '\\'):
			prev = s0
			continue
		prev = s0
		if s0 in '\'"':
			if inq:
				inq = 0
			else:
				inq = 1
				qt = s0
		r = r + s0
	return r

def bracketsBalanced(s):
	'''Returns true iff the brackets in s are balanced'''
	b = filter(lambda x: x in '([{', s)
	e = filter(lambda x: x in ')]}', s)
	return len(e) >= len(b)

class gtkoutfile:
	'''A fake output file object.  It sends output to a TK test widget,
	and if asked for a file number, returns one set on instance creation'''
	def __init__(self, w, fn, tag):
		self.__fn = fn
		self.__w = w
		self.__tag = tag
	def close(self): pass
	flush = close
	def fileno(self):    return self.__fn
	def isatty(self):    return 0
	def read(self, a):   return ''
	def readline(self):  return ''
	def readlines(self): return []
	def write(self, s):
		#stdout.write(str(self.__w.get_point()) + '\n')
		self.__w.text_insert(self.__tag, s)
	def writelines(self, l):
		self.__w.text_insert(self.__tag, l)
	def seek(self, a):   raise IOError, (29, 'Illegal seek')
	def tell(self):      raise IOError, (29, 'Illegal seek')
	truncate = tell

class Console(gtk.VBox):
	def __init__(self, namespace={}, quit_cb=None):
		gtk.VBox.__init__(self, spacing=12)
		self.set_border_width(12)

		self.quit_cb = quit_cb

		self.inp = gtk.HBox()
		self.pack_start(self.inp)
		self.inp.show()

		self.sw = gtk.ScrolledWindow()
		self.sw.set_policy(gtk.POLICY_NEVER, gtk.POLICY_AUTOMATIC);
		self.sw.set_shadow_type(gtk.SHADOW_IN)
		self.inp.pack_start(self.sw, padding=1)
		self.sw.show()

		self.text = gtk.TextView()
		self.text.modify_font(pango.FontDescription('Monospace'))
		self.text.set_editable(False)
		self.text.set_wrap_mode (gtk.WRAP_WORD)
		self.sw.add(self.text)
		self.text.show()

		buffer = self.text.get_buffer()
		self.normal = buffer.create_tag("normal")
		self.error = buffer.create_tag("error")
		self.error.set_property("foreground", "red")
		self.command = buffer.create_tag('command')
		self.command.set_property("foreground", "blue")
 
		self.inputbox = gtk.HBox(spacing=2)
		self.pack_end(self.inputbox, expand=False)
		self.inputbox.show()

		self.prompt = gtk.Label(sys.ps1)
		self.prompt.set_padding(2, 0)
		self.prompt.modify_font(pango.FontDescription('Monospace 10'))
		self.inputbox.pack_start(self.prompt, expand=False)
		self.prompt.show()

		self.line = gtk.Entry()
		self.line.modify_font(pango.FontDescription('Monospace'))
		self.line.connect("key_press_event", self.key_function)
		self.inputbox.pack_start(self.line, padding=2)
		self.line.show()

		self.namespace = namespace

		self.cmd = ''
		self.cmd2 = ''

		# set up hooks for standard output.
		self.stdout = gtkoutfile(self, sys.stdout.fileno(),
					 self.normal)
		try :
			# this will mostly fail on win32 ...
			self.stderr = gtkoutfile(self, sys.stderr.fileno(),
						 self.error)
		except :
			# ... but gtkoutfile is not using the fileno anyway
			self.stderr = gtkoutfile(self, -1, self.error)
		# set up command history
		self.history = ['']
		self.histpos = 0
		self.namespace['__history__'] = self.history

	def scroll_to_end (self):
		iter = self.text.get_buffer().get_end_iter()
		self.text.scroll_to_iter(iter, 0.0)
		return False  # don't requeue this handler

	def text_insert(self, tag, s):
		buffer = self.text.get_buffer()
		iter = buffer.get_end_iter()
		buffer.insert_with_tags (iter, s, tag)
		gobject.idle_add(self.scroll_to_end)

	def init(self):
		self.text.fg = self.text.style.fg[gtk.STATE_NORMAL]
		self.text.bg = self.text.style.white

		self.line.grab_focus()

	def quit(self, *args):
		#self.destroy()
		if self.quit_cb: self.quit_cb()

	def key_function(self, entry, event):
		if event.keyval == gtk.keysyms.Return:
			self.line.emit_stop_by_name("key_press_event")
			self.eval()
		elif event.keyval == gtk.keysyms.Tab:
		        self.line.emit_stop_by_name("key_press_event")
			self.line.insert_text('\t')
			gobject.idle_add(self.focus_text)
		elif event.keyval in (gtk.keysyms.KP_Up, gtk.keysyms.Up) \
		     or (event.keyval in (gtk.keysyms.P, gtk.keysyms.p) and
			 event.state & gtk.gdk.CONTROL_MASK):
			self.line.emit_stop_by_name("key_press_event")
			self.historyUp()
			gobject.idle_add(self.focus_text)
		elif event.keyval in (gtk.keysyms.KP_Down, gtk.keysyms.Down) \
		     or (event.keyval in (gtk.keysyms.N, gtk.keysyms.n) and
			 event.state & gtk.gdk.CONTROL_MASK):
			self.line.emit_stop_by_name("key_press_event")
			self.historyDown()
			gobject.idle_add(self.focus_text)
		elif event.keyval in (gtk.keysyms.L, gtk.keysyms.l) and \
		     event.state & gtk.gdk.CONTROL_MASK:
			self.text.get_buffer().set_text('')
		elif event.keyval in (gtk.keysyms.D, gtk.keysyms.d) and \
		     event.state & gtk.gdk.CONTROL_MASK:
			self.line.emit_stop_by_name("key_press_event")
			self.ctrld()

	def focus_text(self):
		self.line.grab_focus()
		return False  # don't requeue this handler

	def ctrld(self):
		self.quit()
		pass

	def historyUp(self):
		if self.histpos > 0:
			l = self.line.get_text()
			if len(l) > 0 and l[0] == '\n': l = l[1:]
			if len(l) > 0 and l[-1] == '\n': l = l[:-1]
			self.history[self.histpos] = l
			self.histpos = self.histpos - 1
			self.line.set_text(self.history[self.histpos])
			
	def historyDown(self):
		if self.histpos < len(self.history) - 1:
			l = self.line.get_text()
			if len(l) > 0 and l[0] == '\n': l = l[1:]
			if len(l) > 0 and l[-1] == '\n': l = l[:-1]
			self.history[self.histpos] = l
			self.histpos = self.histpos + 1
			self.line.set_text(self.history[self.histpos])

	def eval(self):
		l = self.line.get_text() + '\n'
		if len(l) > 1 and l[0] == '\n': l = l[1:]
		self.histpos = len(self.history) - 1
		if len(l) > 0 and l[-1] == '\n':
			self.history[self.histpos] = l[:-1]
		else:
			self.history[self.histpos] = l
		self.line.set_text('')
		self.text_insert(self.command, self.prompt.get() + l)
		if l == '\n':
			self.run(self.cmd)
			self.cmd = ''
			self.cmd2 = ''
			return
		self.histpos = self.histpos + 1
		self.history.append('')
		self.cmd = self.cmd + l
		self.cmd2 = self.cmd2 + remQuotStr(l)
		l = string.rstrip(l)
		if not bracketsBalanced(self.cmd2) or l[-1] == ':' or \
				l[-1] == '\\' or l[0] in ' \11':
			self.prompt.set_text(sys.ps2)
			self.prompt.queue_draw()
			return
		self.run(self.cmd)
		self.cmd = ''
		self.cmd2 = ''

	def run(self, cmd):
		sys.stdout, self.stdout = self.stdout, sys.stdout
		sys.stderr, self.stderr = self.stderr, sys.stderr
		try:
			try:
				r = eval(cmd, self.namespace, self.namespace)
				if r is not None:
					print `r`
			except SyntaxError:
				exec cmd in self.namespace
		except:
			if hasattr(sys, 'last_type') and \
					sys.last_type == SystemExit:
				self.quit()
			else:
				traceback.print_exc()
		self.prompt.set_text(sys.ps1)
		self.prompt.queue_draw()
		#adj = self.text.get_vadjustment()
		#adj.set_value(adj.upper - adj.page_size)
		sys.stdout, self.stdout = self.stdout, sys.stdout
		sys.stderr, self.stderr = self.stderr, sys.stderr
