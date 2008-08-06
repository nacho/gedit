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

import os
import gtk
from gtk import gdk
import gio
import gedit
#import gtksourceview
from outputpanel import OutputPanel
from capture import *

def default(val, d):
    if val is not None:
        return val
    else:
        return d

# ==== Capture related functions ====
def capture_menu_action(action, window, node):

    # Get the panel
    panel = window.get_data("ExternalToolsPluginWindowData")._output_buffer
    panel.show()
    panel.clear()

    # Configure capture environment
    try:
        cwd = os.getcwd()
    except OSError:
        cwd = os.getenv('HOME');

    capture = Capture(node.command, cwd)
    capture.env = os.environ.copy()
    capture.set_env(GEDIT_CWD = cwd)

    view = window.get_active_view()
    if view is not None:
        # Environment vars relative to current document
        document = view.get_buffer()
        uri = document.get_uri()
        if uri is not None:
            gfile = gio.File(uri)
            scheme = gfile.get_uri_scheme()
            name = os.path.basename(uri)
            capture.set_env(GEDIT_CURRENT_DOCUMENT_URI    = uri,
                            GEDIT_CURRENT_DOCUMENT_NAME   = name,
                            GEDIT_CURRENT_DOCUMENT_SCHEME = scheme)
            if gedit.utils.uri_has_file_scheme(uri):
                path = gfile.get_path()
                cwd = os.path.dirname(path)
                capture.set_cwd(cwd)
                capture.set_env(GEDIT_CURRENT_DOCUMENT_PATH = path,
                                GEDIT_CURRENT_DOCUMENT_DIR  = cwd)

        documents_uri = [doc.get_uri()
                                 for doc in window.get_documents()
                                 if doc.get_uri() is not None]
        documents_path = [gio.File(uri).get_path()
                                 for uri in documents_uri
                                 if gedit.utils.uri_has_file_scheme(uri)]
        capture.set_env(GEDIT_DOCUMENTS_URI  = ' '.join(documents_uri),
                        GEDIT_DOCUMENTS_PATH = ' '.join(documents_path))

    capture.set_flags(capture.CAPTURE_BOTH)

    # Assign the error output to the output panel
    panel.process = capture

    # Get input text
    input_type = node.input
    output_type = node.output

    if input_type != 'nothing' and view is not None:
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
            start = document.get_iter_at_mark(document.get_insert())
            end = start.copy()
            if not start.starts_line():
                start.set_line_offset(0)
            if not end.ends_line():
                end.forward_to_line_end()
        elif input_type == 'word':
            start = document.get_iter_at_mark(document.get_insert())
            end = start.copy()
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
    if output_type == 'new-document':
        tab = window.create_tab(True)
        view = tab.get_view()
        document = tab.get_document()
        pos = document.get_start_iter()
        capture.connect('stdout-line', capture_stdout_line_document, document, pos)
        document.begin_user_action()
        view.set_editable(False)
        view.set_cursor_visible(False)
    elif output_type != 'output-panel' and view is not None:
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
    else:
        capture.connect('stdout-line', capture_stdout_line_panel, panel)
        document.begin_user_action()

    capture.connect('stderr-line',   capture_stderr_line_panel,   panel)
    capture.connect('begin-execute', capture_begin_execute_panel, panel, node.name)
    capture.connect('end-execute',   capture_end_execute_panel,   panel, view,
                    output_type in ('new-document','replace-document'))

    # Run the command
    view.get_window(gtk.TEXT_WINDOW_TEXT).set_cursor(gdk.Cursor(gdk.WATCH))
    capture.execute()
    document.end_user_action()

def capture_stderr_line_panel(capture, line, panel):
    panel.write(line, panel.error_tag)

def capture_begin_execute_panel(capture, panel, label):
    panel['stop'].set_sensitive(True)
    panel.clear()
    panel.write(_("Running tool:"), panel.italic_tag);
    panel.write(" %s\n\n" % label, panel.bold_tag);

def capture_end_execute_panel(capture, exit_code, panel, view, update_type):
    panel['stop'].set_sensitive(False)

    if update_type:
        doc = view.get_buffer()
        start = doc.get_start_iter()
        end = start.copy()
        end.forward_chars(300)

        mtype = gio.content_type_guess(data=doc.get_text(start, end))
        lmanager = gedit.get_language_manager()
        language = gedit.language_manager_get_language_from_mime_type(lmanager, mtype)
        if language is not None:
            doc.set_language(language)

    view.get_window(gtk.TEXT_WINDOW_TEXT).set_cursor(gdk.Cursor(gdk.XTERM))
    view.set_cursor_visible(True)
    view.set_editable(True)

    if exit_code == 0:
        panel.write("\n" + _("Done.") + "\n", panel.italic_tag)
    else:
        panel.write("\n" + _("Exited") + ":", panel.italic_tag)
        panel.write(" %d\n" % exit_code, panel.bold_tag)

def capture_stdout_line_panel(capture, line, panel):
    panel.write(line)

def capture_stdout_line_document(capture, line, document, pos):
    document.insert(pos, line)

# ex:ts=4:et:
