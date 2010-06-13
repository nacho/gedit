# -*- coding: utf-8 -*- ex:set ts=4 et:

from gi.repository import GObject, Peas

class PySampleExtension(GObject.Object, Peas.Activatable):
    __gtype_name__ = "PySampleExtension"

    def do_activate(self, window):
        print self, "activate", window
    
    def do_deactivate(self, window):
        print self, "deactivate", window

    def do_update_state(self, window):
        print self, "update_state", window
