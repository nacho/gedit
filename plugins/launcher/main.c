/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 3 -*- */
#include <config.h>

#include <orb/orbit.h>
#include <libgnorba/gnorba.h>

#include <launcher.h>

void
Exception(CORBA_Environment * ev)
{
   switch (ev->_major) {
     case CORBA_SYSTEM_EXCEPTION:
	fprintf(stderr, "CORBA system exception %s.\n",
		CORBA_exception_id(ev));
	exit(1);
     case CORBA_USER_EXCEPTION:
	fprintf(stderr, "CORBA user exception: %s.\n",
		CORBA_exception_id(ev));
	break;
     default:
	break;
   }
}

int
main(int argc, char *argv[])
{
   CORBA_ORB my_orb;
   CORBA_Environment ev;
   CORBA_Object launcher_server;
   CORBA_long docid;

   int my_argc = 1;
   char *my_argv[2];

   bindtextdomain(PACKAGE, GNOMELOCALEDIR);
   textdomain(PACKAGE);

   my_argv[0] = g_strdup("gedit-launcher");
   my_argv[1] = NULL;

   CORBA_exception_init(&ev);
   my_orb = gnome_CORBA_init("gEdit-Launcher", NULL,
			     &my_argc, my_argv, 0, NULL, &ev);
   Exception(&ev);

   launcher_server = goad_server_activate_with_repo_id
      (0, "IDL:gEdit/launcher:1.0", GOAD_ACTIVATE_REMOTE |
       GOAD_ACTIVATE_EXISTING_ONLY);
   g_assert(launcher_server != CORBA_OBJECT_NIL);
   g_assert(argc == 2);

   docid = gEdit_launcher_open_document(launcher_server, argv[1], &ev);
   Exception(&ev);

   fprintf(stderr, _("Docid is %ld.\n"), docid);
}
