#!/usr/bin/env python

import os
import sys
import getopt

usage = """usage: generate-plugin.py [ options ] plugin-name (words must be separated by '-' )"""

help = """generate scheleton sources for a gedit plugin.

Options:
  -i, --internal        adds the plugin to gedit's configure.ac (TODO)
  -t, --type=TYPE       specify which kind of plugin generate (defauly barebone)
  
types
  barebone - generate code for a simple plugin
  with-ui  - generate code for a plugin which adds menu items and toolbuttons
  sidepane - generate code for a plugin which adds a sidepane page (TODO)

"""


# template info

tmpl_dir = 'plugin_template'

common_tmpl_files = (
    'gedit-xyz-plugin.h',
    'xyz.gedit-plugin.desktop.in',
    'Makefile.am',
)

barebone_tmpl = 'gedit-xyz-plugin.c'
with_ui_tmpl  = 'gedit-xyz-plugin.c'
sidepane_tmpl = 'gedit-xyz-plugin.c'


def copy_template_file(tmpl_file, replacements):
    dest_file = tmpl_file

    for (a, b) in replacements:
        dest_file = dest_file.replace(a, b)

    print "generating " + dest_file + " from " + tmpl_file

    dest_dir = os.path.dirname(dest_file)

    if not os.path.exists(dest_dir):
        os.makedirs(dest_dir)

    r = open(tmpl_file, 'r')
    w = open(dest_file, 'w')

    for line in r:
        for (a, b) in replacements:
            line = line.replace(a, b)
        w.write(line)

    r.close()
    w.close()

# --- main ---

plugin_types = {
    'barebone': barebone_tmpl,
    'with-ui': with_ui_tmpl,
    'sidepane': sidepane_tmpl,
}

#defaults
internal  = False
tmpl_type = barebone_tmpl

try:
    opts, args = getopt.getopt(sys.argv[1:], 'i:t:', ['internal', 'type=', 'help'])
except getopt.error, exc:
    sys.stderr.write('gen-plugin: %s\n' % str(exc))
    sys.stderr.write(usage + '\n')
    sys.exit(1)

for opt, arg in opts:
    if opt == '--help':
        print usage
        print help
        sys.exit(0)
    elif opt in ('-i', '--internal'):
        internal = True
    elif opt in ('-t', '--type'):
        if arg in plugin_types.keys():
            tmpl_type = plugin_types[arg]
        else:
            sys.stderr.write('invalid type, using barebone\n\n')

if len(args) != 1:
    sys.stderr.write(usage + '\n')
    sys.exit(1)

new_name = args[0]

replacements = [('plugin_template', new_name.replace('-', '')),
                ('libxyz', 'lib' + new_name.replace('-', '')),
                ('xyz_', new_name.replace('-', '_') + '_'),
                ('Xyz', new_name.title().replace('-', '')),
                ('XYZ', new_name.upper().replace('-', '_')),
                ('xyz', new_name),
]

root_path = os.path.dirname(__file__)

tmpl_paths = []

for f in common_tmpl_files:
    tmpl_paths.append(os.path.join(root_path, tmpl_dir, f))

tmpl_paths.append(os.path.join(root_path, tmpl_dir, tmpl_type))

#if internal:
#    TODO: edit configure.in
#else
#    tmpl_paths.append(os.path.join(root, "configure.in"))

for f in tmpl_paths:
    copy_template_file(f, replacements)

