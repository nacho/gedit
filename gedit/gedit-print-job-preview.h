/*
 * gedit-print-job-preview.h
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi 
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
 */
 
/*
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */
 
/* This is a modified version of the file gnome-print-job-preview.f file
 * of the library libgnomeprintui. Here the original copyright assignment.
 */
 
/*
 *  Copyright (C) 2000-2002 Ximian Inc.
 *
 *  Authors: Michael Zucchi <notzed@ximian.com>
 *           Miguel de Icaza (miguel@gnu.org)
 */

#ifndef __GEDIT_PRINT_MASTER_PREVIEW_H__
#define __GEDIT_PRINT_MASTER_PREVIEW_H__

#include <gtk/gtkwindow.h>
#include <libgnomeprint/gnome-print-job.h>
#include <libgnomeprintui/gnome-print-preview.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_PRINT_JOB_PREVIEW         (gedit_print_job_preview_get_type ())
#define GEDIT_PRINT_JOB_PREVIEW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GEDIT_TYPE_PRINT_JOB_PREVIEW, GeditPrintJobPreview))
#define GEDIT_PRINT_JOB_PREVIEW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k),    GEDIT_TYPE_PRINT_JOB_PREVIEW, GeditPrintJobPreviewClass))
#define GEDIT_IS_PRINT_JOB_PREVIEW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GEDIT_TYPE_PRINT_JOB_PREVIEW))
#define GEDIT_IS_PRINT_JOB_PREVIEW_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k),    GEDIT_TYPE_PRINT_JOB_PREVIEW))
#define GEDIT_PRINT_JOB_PREVIEW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  GEDIT_TYPE_PRINT_JOB_PREVIEW, GeditPrintJobPreviewClass))

typedef struct _GeditPrintJobPreview        GeditPrintJobPreview;
typedef struct _GeditPrintJobPreviewClass   GeditPrintJobPreviewClass;
typedef struct _GeditPrintJobPreviewPrivate GeditPrintJobPreviewPrivate;

struct _GeditPrintJobPreview
{
	GtkVBox vbox;

	GeditPrintJobPreviewPrivate *priv;
};

struct _GeditPrintJobPreviewClass 
{
	GtkVBoxClass parent_class;
};

GType		 gedit_print_job_preview_get_type	(void) G_GNUC_CONST;

GtkWidget	*gedit_print_job_preview_new		(GnomePrintJob	*gpm);

G_END_DECLS

#endif /* __GEDIT_PRINT_MASTER_PREVIEW_H__ */

