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

__all__ = ('Capture', )

import os, sys, signal
import locale
import subprocess
import gobject

class Capture(gobject.GObject):
    CAPTURE_STDOUT = 0x01
    CAPTURE_STDERR = 0x02
    CAPTURE_BOTH   = 0x03

    __gsignals__ = {
        'stdout-line'  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING,)),
        'stderr-line'  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING,)),
        'begin-execute': (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, tuple()),
        'end-execute'  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_INT,))
    }

    def __init__(self, command, cwd = None, env = {}):
        gobject.GObject.__init__(self)
        self.pipe = None
        self.env = env
        self.cwd = cwd
        self.flags = self.CAPTURE_BOTH
        self.command = command
        self.input_text = None

    def set_env(self, **values):
        self.env.update(**values)

    def set_command(self, command):
        self.command = command

    def set_flags(self, flags):
        self.flags = flags

    def set_input(self, text):
        self.input_text = text

    def set_cwd(self, cwd):
        self.cwd = cwd

    def execute(self):
        if self.command is None:
            return

        # Initialize pipe
        popen_args = {
            'cwd'  : self.cwd,
            'shell': True,
            'env'  : self.env
        }

        if self.input_text is not None:
            popen_args['stdin'] = subprocess.PIPE
        if self.flags & self.CAPTURE_STDOUT:
            popen_args['stdout'] = subprocess.PIPE
        if self.flags & self.CAPTURE_STDERR:
            popen_args['stderr'] = subprocess.PIPE

        self.pipe = subprocess.Popen(self.command, **popen_args)

        # Signal
        self.emit('begin-execute')

        # IO
        if self.input_text is not None:
            self.pipe.stdin.write(self.input_text)
            self.pipe.stdin.close()
        if self.flags & self.CAPTURE_STDOUT:
            gobject.io_add_watch(self.pipe.stdout,
                                 gobject.IO_IN | gobject.IO_HUP,
                                 self.on_output)
        if self.flags & self.CAPTURE_STDERR:
            gobject.io_add_watch(self.pipe.stderr,
                                 gobject.IO_IN | gobject.IO_HUP,
                                 self.on_output)

        # Wait for the process to complete
        gobject.child_watch_add(self.pipe.pid, self.on_child_end)

    def on_output(self, source, condition):
        line = source.readline()

        if len(line) > 0:
            try:
                line = unicode(line, 'utf-8')
            except:
                line = unicode(line,
                               locale.getdefaultlocale()[1],
                               'replace')

            if source == self.pipe.stdout:
                self.emit('stdout-line', line)
            else:
                self.emit('stderr-line', line)
            return True

        return False

    def stop(self, error_code = -1):
        if self.pipe is not None:
            os.kill(self.pipe.pid, signal.SIGTERM)
            self.pipe = None

    def on_child_end(self, pid, error_code):
        # In an idle, so it is emitted after all the std*-line signals
        # have been intercepted
        gobject.idle_add(self.emit, 'end-execute', error_code)

# ex:ts=4:et:
