/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-spell-checker.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <pspell/pspell.h>

#include <glib/gstrfuncs.h>
#include <bonobo/bonobo-i18n.h>

#include "gedit-spell-checker.h"

struct _GeditSpellChecker 
{
	GObject parent_instance;
	
	PspellManager *manager;
	const GeditLanguage *active_lang;
};

struct _GeditSpellCheckerClass 
{
	GObjectClass parent_class;	
};

struct _GeditLanguage 
{
	gchar *abrev;
	gchar *name;	
};

/* GObject properties */
enum {
	PROP_0,
	PROP_AVAILABLE_LANGUAGES,
	PROP_LANGUAGE,
	LAST_PROP
};

#define KNOWN_LANGUAGES 22
static GeditLanguage known_languages [KNOWN_LANGUAGES + 1] = 
{
	{"br", N_("Breton")},
	{"ca", N_("Catalan")},
	{"cs", N_("Czech")},
	{"da", N_("Danish")},
	{"de_de", N_("German (Germany)")},
	{"de_ch", N_("German (Swiss)")},
	{"en_us", N_("English (American)")},
	{"en_gb", N_("English (British)")},
	{"en_ca", N_("English (Canadian)")},
	{"eo", N_("Esperanto")},
	{"es", N_("Spanish")},
	{"fo", N_("Faroese")},
	{"fr_fr", N_("French (France)")},
	{"fr_ch", N_("French (Swiss)")},
	{"it", N_("Italian")},
	{"nl", N_("Dutch")},
	{"no", N_("Norwegian")},
	{"pl", N_("Polish")},
	{"pt_pt", N_("Portuguese (Portugal)")},
	{"pt_br", N_("Portuguese (Brazilian)")},
	{"ru", N_("Russian")},
{"sv", N_("Swedish")},
	{NULL, NULL}
};

static void	gedit_spell_checker_class_init 		(GeditSpellCheckerClass * klass);

static void	gedit_spell_checker_init 		(GeditSpellChecker *spell_checker);
static void 	gedit_spell_checker_finalize 		(GObject *object);

static gboolean	set_language_internal 			(GeditSpellChecker *spell, 
							 const GeditLanguage *language, 
							 GError **error);
static const GSList *get_languages 			(void);
static gboolean is_digit 				(const char *text, gint length);

/* FIXME: should be freed by the class finalizer */
static GSList *available_languages = NULL;

static GObjectClass *parent_class = NULL;


GQuark
gedit_spell_checker_error_quark (void)
{
	static GQuark q = 0;

	if (q == 0)
		q = g_quark_from_static_string (
				"gedit-spell-checker-error-quark");

	return q;
}

GType
gedit_spell_checker_get_type (void)
{
	static GType gedit_spell_checker_type = 0;

	if(!gedit_spell_checker_type) 
	{
		static const GTypeInfo gedit_spell_checker_info = 
		{
			sizeof (GeditSpellCheckerClass),
			NULL, /* base init */
			NULL, /* base finalize */
			(GClassInitFunc) gedit_spell_checker_class_init, /* class init */
			NULL, /* class finalize */
			NULL, /* class data */
			sizeof (GeditSpellChecker),
			0,
			(GInstanceInitFunc) gedit_spell_checker_init
		};

		gedit_spell_checker_type = g_type_register_static (G_TYPE_OBJECT,
							"GeditSpellChecker",
							&gedit_spell_checker_info, 0);
	}

	return gedit_spell_checker_type;
}

static void
gedit_spell_checker_set_property (GObject *object,
			   guint prop_id,
			   const GValue *value,
			   GParamSpec *pspec)
{
	/*
	GeditSpellChecker *spell = GEDIT_SPELL_CHECKER (object);
	*/

	switch (prop_id)
	{
		case PROP_LANGUAGE:
			/* TODO */
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
gedit_spell_checker_get_property (GObject *object,
			   guint prop_id,
			   GValue *value,
			   GParamSpec *pspec)
{
	/*
	GeditSpellChecker *spell = GEDIT_SPELL_CHECKER (object);
	*/
	
	switch (prop_id)
	{
		case PROP_AVAILABLE_LANGUAGES:
			/* TODO */

		case PROP_LANGUAGE:
			/* TODO */
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
gedit_spell_checker_class_init (GeditSpellCheckerClass * klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->set_property = gedit_spell_checker_set_property;
	object_class->get_property = gedit_spell_checker_get_property;

	object_class->finalize = gedit_spell_checker_finalize;

	g_object_class_install_property (object_class,
					 PROP_AVAILABLE_LANGUAGES,
					 g_param_spec_pointer ("available_languages",
						 	       "Available languages",
							       "The list of available languages",
							      G_PARAM_READABLE));
	g_object_class_install_property (object_class,
					 PROP_LANGUAGE,
					 g_param_spec_pointer ("language",
						 	      "Language",
							      "The language used by the spell checker",
							      G_PARAM_READWRITE));
}


static void
gedit_spell_checker_init (GeditSpellChecker *spell_checker)
{
	spell_checker->manager = NULL;
	spell_checker->active_lang = NULL;
}

GeditSpellChecker *
gedit_spell_checker_new	(void)
{
	GeditSpellChecker *spell;

	spell = GEDIT_SPELL_CHECKER (
			g_object_new (GEDIT_TYPE_SPELL_CHECKER, NULL));

	g_return_val_if_fail (spell != NULL, NULL);

	return spell;
}


static void 
gedit_spell_checker_finalize (GObject *object)
{
	GeditSpellChecker *spell_checker;

	g_return_if_fail (GEDIT_IS_SPELL_CHECKER (object));
	
	spell_checker = GEDIT_SPELL_CHECKER (object);

	if (spell_checker->manager != NULL)
		delete_pspell_manager (spell_checker->manager);

	g_print ("Spell checker finalized.\n");
}

static gboolean
set_language_internal (GeditSpellChecker *spell, const GeditLanguage *language, GError **error) 
{
	PspellConfig *config;
	PspellCanHaveError *err;
	const gchar *lang;

	g_return_val_if_fail (spell != NULL, FALSE);
		
	if (language == NULL) 
	{
		lang = g_getenv ("LANG");
		
		if (lang != NULL) 
		{
			if (strncmp (lang, "C", 1) == 0)
				lang = NULL;
			else if (lang[0] == 0)
				lang = NULL;
		}
	}
	else
		lang = language->abrev;

	config = new_pspell_config ();
	g_return_val_if_fail (config != NULL, FALSE);
	
	if (lang != NULL)
		pspell_config_replace (config, "language-tag", lang);
	
	pspell_config_replace (config, "encoding", "utf-8");
	
	pspell_config_replace (config, "mode", "url");

	err = new_pspell_manager (config);

	delete_pspell_config (config);

	if (pspell_error_number (err) != 0) 
	{
		g_set_error (error, GEDIT_SPELL_CHECKER_ERROR, GEDIT_SPELL_CHECKER_ERROR_PSPELL,
				"pspell: %s", pspell_error_message (err));
		return FALSE;
	} 
	
	if (spell->manager != NULL)
		delete_pspell_manager (spell->manager);
	
	spell->manager = to_pspell_manager (err);
	g_return_val_if_fail (spell->manager != NULL, FALSE);

	spell->active_lang = language;

	return TRUE;
}

static gboolean
lazy_init (GeditSpellChecker *spell, GError **error) 
{
	g_return_val_if_fail (spell != NULL, FALSE);

	if (spell->manager == NULL)
		return set_language_internal (spell, NULL, error);
	else
		return TRUE;
}

static const GSList *
get_languages (void)
{
	PspellCanHaveError *err;
	PspellConfig  *config;
	PspellManager *manager;
	
	gint i;

	if (available_languages != NULL)
		return available_languages;

	for (i = 0; known_languages [i].abrev; i++) 
	{
		config = new_pspell_config ();
		pspell_config_replace (config, "language-tag", known_languages [i].abrev);
		
		err = new_pspell_manager (config);
		
		if (pspell_error_number (err) == 0) 
		{
			manager = to_pspell_manager (err);
			delete_pspell_manager (manager);
			
			available_languages = g_slist_prepend (available_languages, &known_languages [i]);
		}
	}

	available_languages = g_slist_reverse (available_languages);

	return available_languages;
}

const GSList *
gedit_spell_checker_get_available_languages (GeditSpellChecker *spell)
{
	g_return_val_if_fail (spell != NULL, NULL);
	g_return_val_if_fail (GEDIT_IS_SPELL_CHECKER (spell), NULL);

	return get_languages ();
}

gboolean
gedit_spell_checker_set_language (GeditSpellChecker *spell, 
			const GeditLanguage *lang,
			GError **error)
{
	g_return_val_if_fail (spell != NULL, FALSE);
	g_return_val_if_fail (GEDIT_IS_SPELL_CHECKER (spell), FALSE);

	return set_language_internal (spell, lang, error);
}

const GeditLanguage *
gedit_spell_checker_get_language (GeditSpellChecker *spell)
{
	g_return_val_if_fail (spell != NULL, NULL);
	g_return_val_if_fail (GEDIT_IS_SPELL_CHECKER (spell), NULL);

	return spell->active_lang;
}

gboolean
gedit_spell_checker_check_word (GeditSpellChecker *spell, 
			const gchar *word, 
			gint len, 
			GError **error)
{
	gint pspell_result;
	gboolean res = FALSE;
	
	g_return_val_if_fail (spell != NULL, FALSE);
	g_return_val_if_fail (GEDIT_IS_SPELL_CHECKER (spell), FALSE);
	
	g_return_val_if_fail (word != NULL, FALSE);

	if (!lazy_init (spell, error))
		return FALSE;

	g_return_val_if_fail (spell->manager != NULL, FALSE);
	
	if (len < 0)
		len = -1;

	if (strcmp (word, "gedit") == 0)
		return TRUE;

	if (is_digit (word, len))
		return TRUE;

	pspell_result = pspell_manager_check (spell->manager, word, len);

	switch (pspell_result)
	{
		case -1:
			/* error */
			res = FALSE;

			g_set_error (error, GEDIT_SPELL_CHECKER_ERROR, 
					GEDIT_SPELL_CHECKER_ERROR_PSPELL,
					"pspell: %s", 
					pspell_manager_error_message (spell->manager));
			break;
		case 0:
			/* it is not in the directory */
			res = FALSE;
			break;
		case 1:
			/* is is in the directory */
			res = TRUE;
			break;
		default:
			g_return_val_if_fail (FALSE, FALSE);
	}

	return res;
}


/* return NULL on error or if no suggestions are found */
GSList *
gedit_spell_checker_get_suggestions (GeditSpellChecker *spell, 
			const gchar *word, 
			gint len, 
			GError **error)
{
	const PspellWordList *suggestions;
	PspellStringEmulation *elements;
	GSList *suggestions_list = NULL;
	gint i;
	gint list_len;

	g_return_val_if_fail (spell != NULL, FALSE);
	g_return_val_if_fail (GEDIT_IS_SPELL_CHECKER (spell), FALSE);
	
	g_return_val_if_fail (word != NULL, FALSE);

	if (!lazy_init (spell, error))
		return FALSE;

	g_return_val_if_fail (spell->manager != NULL, FALSE);

	if (len < 0)
		len = -1;

	suggestions = pspell_manager_suggest (spell->manager, word, len);

	if (suggestions == NULL)
	{
		g_set_error (error, GEDIT_SPELL_CHECKER_ERROR, 
					GEDIT_SPELL_CHECKER_ERROR_PSPELL,
					"pspell: %s", 
					pspell_manager_error_message (spell->manager));

		return NULL;
	}

	elements = pspell_word_list_elements (suggestions);
	
	list_len = pspell_word_list_size (suggestions);
	
	if (list_len == 0)
		return NULL;
	
	for (i = 0; i < list_len; i ++)
	{
		suggestions_list = g_slist_prepend (suggestions_list,
				g_strdup (pspell_string_emulation_next (elements)));
	}
	
	delete_pspell_string_emulation (elements);

	suggestions_list = g_slist_reverse (suggestions_list);
	
	return suggestions_list;
}

gboolean
gedit_spell_checker_add_word_to_personal (GeditSpellChecker *spell, 
			const gchar *word, 
			gint len, 
			GError **error)
{
	gint pspell_result;
	
	g_return_val_if_fail (spell != NULL, FALSE);
	g_return_val_if_fail (GEDIT_IS_SPELL_CHECKER (spell), FALSE);
	
	g_return_val_if_fail (word != NULL, FALSE);

	if (!lazy_init (spell, error))
		return FALSE;

	g_return_val_if_fail (spell->manager != NULL, FALSE);
	
	if (len < 0)
		len = -1;

	pspell_result = pspell_manager_add_to_personal (spell->manager, word, len);

	if (pspell_result == 0)
	{
		g_set_error (error, GEDIT_SPELL_CHECKER_ERROR, 
					GEDIT_SPELL_CHECKER_ERROR_PSPELL,
					"pspell: %s", 
					pspell_manager_error_message (spell->manager));

		return FALSE;
	}	

	pspell_manager_save_all_word_lists (spell->manager);

	return TRUE;
}

gboolean
gedit_spell_checker_add_word_to_session (GeditSpellChecker *spell, 
			const gchar *word, 
			gint len, 
			GError **error)
{
	gint pspell_result;
	
	g_return_val_if_fail (spell != NULL, FALSE);
	g_return_val_if_fail (GEDIT_IS_SPELL_CHECKER (spell), FALSE);
	
	g_return_val_if_fail (word != NULL, FALSE);

	if (!lazy_init (spell, error))
		return FALSE;

	g_return_val_if_fail (spell->manager != NULL, FALSE);
	
	if (len < 0)
		len = -1;

	pspell_result = pspell_manager_add_to_session (spell->manager, word, len);

	if (pspell_result == 0)
	{
		g_set_error (error, GEDIT_SPELL_CHECKER_ERROR, 
					GEDIT_SPELL_CHECKER_ERROR_PSPELL,
					"pspell: %s", 
					pspell_manager_error_message (spell->manager));

		return FALSE;
	}
	
	return TRUE;
}

gboolean
gedit_spell_checker_clear_session (GeditSpellChecker *spell, GError **error)
{
	gint pspell_result;
	
	g_return_val_if_fail (spell != NULL, FALSE);
	g_return_val_if_fail (GEDIT_IS_SPELL_CHECKER (spell), FALSE);

	if (spell->manager == NULL)
		return TRUE;
	
	pspell_result = pspell_manager_clear_session (spell->manager);

	if (pspell_result == 0)
	{
		g_set_error (error, GEDIT_SPELL_CHECKER_ERROR, 
					GEDIT_SPELL_CHECKER_ERROR_PSPELL,
					"pspell: %s", 
					pspell_manager_error_message (spell->manager));

		return FALSE;
	}	

	return TRUE;
}

/*
 * Informs dictionary, that word 'word' will be replaced/corrected by word 'replacement'
 *
 */
gboolean
gedit_spell_checker_set_correction (GeditSpellChecker *spell, 
			const gchar *word, gint w_len, 
			const gchar *replacement, gint r_len,
			GError **error)
{
	gint pspell_result;
	
	g_return_val_if_fail (spell != NULL, FALSE);
	g_return_val_if_fail (GEDIT_IS_SPELL_CHECKER (spell), FALSE);
	
	g_return_val_if_fail (word != NULL, FALSE);
	g_return_val_if_fail (replacement != NULL, FALSE);

	if (!lazy_init (spell, error))
		return FALSE;

	g_return_val_if_fail (spell->manager != NULL, FALSE);
	
	if (w_len < 0)
		w_len = -1;

	if (r_len < 0)
		r_len = -1;

	pspell_result = pspell_manager_store_replacement (spell->manager,
				word, w_len,
				replacement, r_len);

	if (pspell_result == 0)
	{
		g_set_error (error, GEDIT_SPELL_CHECKER_ERROR, 
					GEDIT_SPELL_CHECKER_ERROR_PSPELL,
					"pspell: %s", 
					pspell_manager_error_message (spell->manager));

		return FALSE;
	}	

	pspell_manager_save_all_word_lists (spell->manager);

	return TRUE;
}

gchar *
gedit_language_to_string (const GeditLanguage *lang)
{
	if (lang == NULL)
		return g_strdup (_("Default"));

	return g_strdup (_(lang->name));
}


static gboolean 
is_digit (const char *text, gint length)
{
	gunichar c;
	const gchar *p;
 	const gchar *end;

	g_return_val_if_fail (text != NULL, FALSE);

	if (length < 0)
		length = strlen (text);

	p = text;
	end = text + length;

	while (p != end) {
		const gchar *next;
		next = g_utf8_next_char (p);

		c = g_utf8_get_char (p);

		if (!g_unichar_isdigit (c) && c != '.' && c != ',') 
			return FALSE;

		p = next;
	}

	return TRUE;
}
