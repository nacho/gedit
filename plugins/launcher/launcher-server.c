/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <config.h>
#include <gnome.h>

#include <gE_plugin.h>

#include <launcher-impl.h>

static void init_plugin (gE_Plugin_Object *, gint);

gint edit_context = 0;

gE_Plugin_Info gedit_plugin_info = {
	"Document Launcher",
	init_plugin,
	NULL,
};

void
init_plugin (gE_Plugin_Object *plugin, gint context)
{
	CORBA_Object	launcher;
	gchar*		objref;

	fprintf (stderr, "Greetings from the plugin with context %d.\n",
		 context);

	launcher = impl_gEdit_launcher__create
		(root_poa, (CORBA_long) context, global_ev);
	corba_exception (global_ev);
    
	objref = CORBA_ORB_object_to_string (global_orb, launcher, global_ev);
	corba_exception (global_ev);
    
	gnome_register_corba_server
		(name_service, launcher, "gedit-launcher",
		 "object", global_ev);
	corba_exception (global_ev);
}
