/*
 * gedit-style-scheme-generator.h
 * This file is part of gedit
 *
 * Copyright (C) 2007 - Paolo Maggi
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifndef __GEDIT_STYLE_SCHEME_GENERATOR_H__
#define __GEDIT_STYLE_SCHEME_GENERATOR_H__

#include <glib-object.h>
#include <gtksourceview/gtksourcestylescheme.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_STYLE_SCHEME_GENERATOR		gedit_style_scheme_generator_get_type()
#define GEDIT_STYLE_SCHEME_GENERATOR(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_STYLE_SCHEME_GENERATOR, GeditStyleSchemeGenerator))
#define GEDIT_STYLE_SCHEME_GENERATOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_STYLE_SCHEME_GENERATOR, GeditStyleSchemeGeneratorClass))
#define GEDIT_IS_STYLE_SCHEME_GENERATOR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_STYLE_SCHEME_GENERATOR))
#define GEDIT_IS_STYLE_SCHEME_GENERATOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_STYLE_SCHEME_GENERATOR))
#define GEDIT_STYLE_SCHEME_GENERATOR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_STYLE_SCHEME_GENERATOR, GeditStyleSchemeGeneratorClass))

typedef struct _GeditStyleSchemeGenerator		GeditStyleSchemeGenerator;
typedef struct _GeditStyleSchemeGeneratorPrivate	GeditStyleSchemeGeneratorPrivate;

struct _GeditStyleSchemeGenerator
{
        GObject parent;
        
        /*< private > */
        GeditStyleSchemeGeneratorPrivate *priv;
};

typedef struct _GeditStyleSchemeGeneratorClass		GeditStyleSchemeGeneratorClass;

struct _GeditStyleSchemeGeneratorClass
{
        GObjectClass base_class;        
};

GType                           gedit_style_scheme_generator_get_type (void)  G_GNUC_CONST;

GeditStyleSchemeGenerator*      gedit_style_scheme_generator_new			(GtkSourceStyleScheme *scheme);

const gchar             	*gedit_style_scheme_generator_get_scheme_name		(GeditStyleSchemeGenerator *generator);
void				 gedit_style_scheme_generator_set_scheme_name		(GeditStyleSchemeGenerator *generator,
											 const gchar               *name);

const gchar             	*gedit_style_scheme_generator_get_scheme_description	(GeditStyleSchemeGenerator *generator);
void				 gedit_style_scheme_generator_set_scheme_description	(GeditStyleSchemeGenerator *generator,
											 const gchar               *description);

gboolean			 gedit_style_scheme_generator_get_text_color		(GeditStyleSchemeGenerator *generator,
											 GdkColor                  *color);
void		     		 gedit_style_scheme_generator_set_text_color		(GeditStyleSchemeGenerator *generator,
											 const GdkColor            *color);

gboolean			 gedit_style_scheme_generator_get_background_color	(GeditStyleSchemeGenerator *generator,
											 GdkColor                  *color);
void		     		 gedit_style_scheme_generator_set_background_color	(GeditStyleSchemeGenerator *generator,
											 const GdkColor            *color);

gboolean			 gedit_style_scheme_generator_get_selected_text_color	(GeditStyleSchemeGenerator *generator,
											 GdkColor                  *color);
void		     		 gedit_style_scheme_generator_set_selected_text_color	(GeditStyleSchemeGenerator *generator,
											 const GdkColor            *color);

gboolean			 gedit_style_scheme_generator_get_selection_color	(GeditStyleSchemeGenerator *generator,
											 GdkColor                  *color);
void		     		 gedit_style_scheme_generator_set_selection_color	(GeditStyleSchemeGenerator *generator,
											 const GdkColor            *color);

gboolean			 gedit_style_scheme_generator_get_current_line_color	(GeditStyleSchemeGenerator *generator,
											 GdkColor                  *color);
void		     		 gedit_style_scheme_generator_set_current_line_color	(GeditStyleSchemeGenerator *generator,
											 const GdkColor            *color);

gboolean			 gedit_style_scheme_generator_get_search_hl_text_color	(GeditStyleSchemeGenerator *generator,
											 GdkColor                  *color);
void		     		 gedit_style_scheme_generator_set_search_hl_text_color	(GeditStyleSchemeGenerator *generator,
											 const GdkColor            *color);

G_END_DECLS

#endif /* __GEDIT_STYLE_SCHEME_GENERATOR_H__ */

