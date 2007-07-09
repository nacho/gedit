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

import os

from Placeholder import *
from Parser import Parser, Token
from Helper import *

class EvalUtilities:
        def __init__(self, view=None):
                self.view = view
                self._init_namespace()
        
        def _init_namespace(self):
                self.namespace = {
                        '__builtins__': __builtins__,
                        'align': self.util_align
                }

        def _real_len(self, s, tablen = 0):
                if tablen == 0:
                        tablen = self.view.get_tabs_width()
                
                return len(s.expandtabs(tablen))

        def util_align(self, items):
                maxlen = []
                tablen = self.view.get_tabs_width()
                
                for row in range(0, len(items)):
                        for col in range(0, len(items[row]) - 1):
                                if row == 0:
                                        maxlen.append(0)
                                
                                items[row][col] += "\t"
                                rl = self._real_len(items[row][col], tablen)
                                
                                if (rl > maxlen[col]):
                                        maxlen[col] = rl

                result = ''
                
                for row in range(0, len(items)):
                        for col in range(0, len(items[row]) - 1):
                                item = items[row][col]
                                
                                result += item + ("\t" * ((maxlen[col] - \
                                                self._real_len(item, tablen)) / tablen))
                        
                        result += items[row][len(items[row]) - 1]
                        
                        if row != len(items) - 1:
                                result += "\n"
                
                return result

class Snippet:
        def __init__(self, data):
                self.data = data
        
        def __getitem__(self, prop):
                return self.data[prop]
        
        def __setitem__(self, prop, value):
                self.data[prop] = value
        
        def accelerator_display(self):
                accel = self['accelerator']

                if accel:
                        keyval, mod = gtk.accelerator_parse(accel)
                        accel = gtk.accelerator_get_label(keyval, mod)
                
                return accel or ''

        def display(self):
                nm = markup_escape(self['description'])
                
                tag = self['tag']
                accel = self.accelerator_display()
                detail = []
                
                if tag and tag != '':
                        detail.append(tag)
                        
                if accel and accel != '':
                        detail.append(accel)
                
                if not detail:
                        return nm
                else:
                        return nm + ' (<b>' + markup_escape(str.join(', ', detail)) + \
                                        '</b>)'

        def _add_placeholder(self, placeholder):
                if self._placeholders.has_key(placeholder.tabstop):
                        if placeholder.tabstop == -1:
                                self._placeholders[-1].append(placeholder)
                elif placeholder.tabstop == -1:
                        self._placeholders[-1] = [placeholder]
                else:
                        self._placeholders[placeholder.tabstop] = placeholder

        def _insert_text(self, text):
                # Insert text keeping indentation in mind
                indented = unicode.join('\n' + unicode(self._indent), spaces_instead_of_tabs(self._view, text).split('\n'))
                self._view.get_buffer().insert(self._begin_iter(), indented)

        def _begin_iter(self):
                return self._view.get_buffer().get_iter_at_mark(self._begin_mark)
                
        def _create_environment(self, data):
                val = ((data in os.environ) and os.environ[data]) or ''
                
                # Get all the current indentation
                all_indent = compute_indentation(self._view, self._begin_iter())
                
                # Substract initial indentation to get the snippet indentation
                indent = all_indent[len(self._indent):]

                # Keep indentation
                return unicode.join('\n' + unicode(indent), val.split('\n'))
        
        def _create_placeholder(self, data):
                tabstop = data['tabstop']
                begin = self._begin_iter()
                
                if tabstop == 0:
                        # End placeholder
                        return PlaceholderEnd(self._view, begin, data['default'])
                elif self._placeholders.has_key(tabstop):
                        # Mirror placeholder
                        return PlaceholderMirror(self._view, tabstop, begin)
                else:
                        # Default placeholder
                        return Placeholder(self._view, tabstop, data['default'], begin)
     
        def _create_shell(self, data):
                begin = self._begin_iter()
                return PlaceholderShell(self._view, data['tabstop'], begin, data['contents'])

        def _create_eval(self, data):
                begin = self._begin_iter()
                return PlaceholderEval(self._view, data['tabstop'], data['dependencies'], begin, data['contents'], self._utils.namespace)
        
        def _create_regex(self, data):
                begin = self._begin_iter()
                return PlaceholderRegex(self._view, data['tabstop'], begin, data['input'], data['pattern'], data['substitution'], data['modifiers'])
                
        def _create_text(self, data):
                return data

        def _invalid_placeholder(self, placeholder, remove):
                buf = self._view.get_buffer()
              
                # Remove the text because this placeholder is invalid
                if placeholder.default and remove:
                        buf.delete(placeholder.begin_iter(), placeholder.end_iter())
                
                placeholder.remove()

                if placeholder.tabstop == -1:
                        index = self._placeholders[-1].index(placeholder)
                        del self._placeholders[-1][index]
                else:
                        del self._placeholders[placeholder.tabstop]

        def _parse(self, view, marks):
                # Initialize current variables
                self._view = view
                self._indent = compute_indentation(view, view.get_buffer().get_iter_at_mark(marks[1]))        
                self._utils = EvalUtilities(view)
                self._placeholders = {}
                self._begin_mark = marks[1]
                
                # Create parser
                parser = Parser(data=self['text'])

                # Parse tokens
                while (True):
                        token = parser.token()
                        
                        if not token:
                                break

                        try:
                                val = {'environment': self._create_environment,
                                        'placeholder': self._create_placeholder,
                                        'shell': self._create_shell,
                                        'eval': self._create_eval,
                                        'regex': self._create_regex,
                                        'text': self._create_text}[token.klass](token.data)
                        except:
                                sys.stderr.write('Token class not supported: %s\n' % token.klass)
                                continue
                        
                        if isinstance(val, basestring):
                                # Insert text
                                self._insert_text(val)
                        else:
                                # Insert placeholder
                                self._add_placeholder(val)

                # Create end placeholder if there isn't one yet
                if not self._placeholders.has_key(0):
                        self._placeholders[0] = PlaceholderEnd(view, view.get_buffer().get_iter_at_mark(marks[1]), None)

                # Make sure run_last is ran for all placeholders and remove any
                # non `ok` placeholders
                for tabstop in self._placeholders.copy():
                        ph = (tabstop == -1 and list(self._placeholders[-1])) or [self._placeholders[tabstop]]

                        for placeholder in ph:
                                placeholder.run_last(self._placeholders)
                                
                                if not placeholder.ok or placeholder.done:
                                        self._invalid_placeholder(placeholder, not placeholder.ok)

                # Remove all the Expand placeholders which have a tabstop because
                # they can be used to mirror, but they shouldn't be real tabstops
                # (if they have mirrors installed). This is problably a bit of 
                # a dirty hack :)
                if not self._placeholders.has_key(-1):
                        self._placeholders[-1] = []

                for tabstop in self._placeholders.copy():
                        placeholder = self._placeholders[tabstop]

                        if tabstop != -1:
                                if isinstance(placeholder, PlaceholderExpand) and placeholder.has_references:
                                        # Add to anonymous placeholders
                                        self._placeholders[-1].append(placeholder)
                                        
                                        # Remove placeholder
                                        del self._placeholders[tabstop]
        
        def insert_into(self, plugin_data):
                buf = plugin_data.view.get_buffer()
                insert = buf.get_iter_at_mark(buf.get_insert())
                last_index = 0
                
                # Find closest mark at current insertion, so that we may insert
                # our marks in the correct order
                (current, next) = plugin_data.next_placeholder()
                
                if current:
                        # Insert AFTER current
                        last_index = plugin_data.placeholders.index(current) + 1
                elif next:
                        # Insert BEFORE next
                        last_index = plugin_data.placeholders.index(next)
                else:
                        # Insert at first position
                        last_index = 0
                
                # lastIndex now contains the position of the last mark
                # Create snippet bounding marks
                marks = (buf.create_mark(None, insert, True), 
                         buf.create_mark(None, insert, False),
                         buf.create_mark(None, insert, False))
                
                # Now parse the contents of this snippet, create Placeholders
                # and insert the placholder marks in the marks array of plugin_data
                self._parse(plugin_data.view, marks)
                
                # So now all of the snippet is in the buffer, we have all our 
                # placeholders right here, what's next, put all marks in the 
                # plugin_data.marks
                k = self._placeholders.keys()
                k.sort(reverse=True)
                
                plugin_data.placeholders.insert(last_index, self._placeholders[0])
                last_iter = self._placeholders[0].end_iter()
                
                for tabstop in k:
                        if tabstop != -1 and tabstop != 0:
                                placeholder = self._placeholders[tabstop]
                                end_iter = placeholder.end_iter()
                                
                                if last_iter.compare(end_iter) < 0:
                                        last_iter = end_iter
                                
                                # Inserting placeholder
                                plugin_data.placeholders.insert(last_index, placeholder)
                
                # Move end mark to last placeholder
                buf.move_mark(marks[1], last_iter)
                
                return (marks[0], marks[1], marks[2], self._placeholders)

# ex:ts=8:et:
