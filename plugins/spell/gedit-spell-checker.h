/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-spell-checker.h
 * This file is part of gedit
 *
 * Copyright (C) 2002 Paolo Maggi 
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
 * Modified by the gedit Team, 2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifndef __GEDIT_SPELL_CHECKER_H__
#define __GEDIT_SPELL_CHECKER_H__

#include <glib.h>
#include <glib-object.h>


G_BEGIN_DECLS

#define GEDIT_TYPE_SPELL_CHECKER            (gedit_spell_checker_get_type ())
#define GEDIT_SPELL_CHECKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_SPELL_CHECKER, GeditSpellChecker))
#define GEDIT_SPELL_CHECKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_SPELL_CHECKER, GeditSpellChecker))
#define GEDIT_IS_SPELL_CHECKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_SPELL_CHECKER))
#define GEDIT_IS_SPELL_CHECKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_SPELL_CHECKER))
#define GEDIT_SPELL_CHECKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_SPELL_CHECKER, GeditSpellChecker))



#define GEDIT_SPELL_CHECKER_ERROR gedit_spell_checker_error_quark()

typedef enum 
{
	GEDIT_SPELL_CHECKER_ERROR_PSPELL
} GeditSpellCheckerError;



typedef struct _GeditLanguage GeditLanguage;

typedef struct _GeditSpellChecker GeditSpellChecker;

typedef struct _GeditSpellCheckerClass GeditSpellCheckerClass;



GType        		 gedit_spell_checker_get_type		(void) G_GNUC_CONST;

GQuark 			 gedit_spell_checker_error_quark	(void);

/* Constructors */
GeditSpellChecker	*gedit_spell_checker_new		(void);

/* GSList contains GeditLanguage* items */
const GSList 		*gedit_spell_checker_get_available_languages (void);


gboolean		 gedit_spell_checker_set_language 	(GeditSpellChecker *spell, 
								 const GeditLanguage *lang,
								 GError **error);
const GeditLanguage 	*gedit_spell_checker_get_language 	(GeditSpellChecker *spell);

gboolean		 gedit_spell_checker_check_word 	(GeditSpellChecker *spell, 
								 const gchar *word, 
								 gint len, 
								 GError **error);

GSList 			*gedit_spell_checker_get_suggestions 	(GeditSpellChecker *spell, 
								 const gchar *word, 
								 gint len, 
								 GError **error);

gboolean		 gedit_spell_checker_add_word_to_personal (GeditSpellChecker *spell, 
								 const gchar *word, 
								 gint len, 
								 GError **error);
gboolean		 gedit_spell_checker_add_word_to_session (GeditSpellChecker *spell, 
								 const gchar *word, 
								 gint len, 
								 GError **error);

gboolean		 gedit_spell_checker_clear_session 	(GeditSpellChecker *spell, 
								 GError **error);

gboolean		 gedit_spell_checker_set_correction 	(GeditSpellChecker *spell, 
								 const gchar *word, 
								 gint w_len, 
								 const gchar *replacement, 
								 gint r_len,
								 GError **error);

gchar 			*gedit_language_to_string 		(const GeditLanguage *lang);



G_END_DECLS

#endif  /* __GEDIT_SPELL_CHECKER_H__ */

