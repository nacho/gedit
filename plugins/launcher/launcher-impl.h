/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef _LAUNCHER_IMPL_H
#define _LAUNCHER_IMPL_H 1

#include <config.h>
#include <gnome.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <orb/orbit.h>
#include <libgnorba/gnorba.h>

#include <launcher.h>

BEGIN_GNOME_DECLS

/*** App-specific servant structures ***/
typedef struct {
	POA_gEdit_launcher	servant;
	PortableServer_POA	poa;
	CORBA_long		context;
} impl_POA_gEdit_launcher;

/*** Implementation stub prototypes ***/
gEdit_launcher
impl_gEdit_launcher__create (PortableServer_POA poa, CORBA_long context,
			     CORBA_Environment * ev);

CORBA_long
impl_gEdit_launcher_open_file (impl_POA_gEdit_launcher * servant,
			       CORBA_char *filename,
			       CORBA_Environment *ev);

END_GNOME_DECLS

#endif
