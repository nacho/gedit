/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * bonobo-mdi-session.h - session managament functions
 *
 * Copyright (C) 2002 Free Software Foundation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 *
 */

/*
 * Originally written by Martin Baulig <martin@home-of-linux.org>
 */

#ifndef __BONOBO_MDI_SESSION_H__
#define __BONOBO_MDI_SESSION_H__

#include <string.h>

#include "bonobo-mdi.h"

/* This function should parse the config string and return a newly
 * created BonoboMDIChild. */
typedef BonoboMDIChild *(*BonoboMDIChildCreator) (const gchar *);

/* bonobo_mdi_restore_state(): call this with the BonoboMDI object, the
 * config section name and the function used to recreate the BonoboMDIChildren
 * from their config strings. */
gboolean	bonobo_mdi_restore_state	(BonoboMDI *mdi, const gchar *section,
					 BonoboMDIChildCreator child_create_func);

/* bonobo_mdi_save_state (): call this with the BonoboMDI object as the
 * first and the config section name as the second argument. */
void		bonobo_mdi_save_state	(BonoboMDI *mdi, const gchar *section);

#endif
