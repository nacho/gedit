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

#include <glib/gi18n.h>
#include <glib/gstrfuncs.h>

#include "gedit-spell-checker.h"

/* FIXME - Rename the marshal file - Paolo */
#include "gedit-spell-checker-dialog-marshal.h"

struct _GeditSpellChecker 
{
	GObject parent_instance;
	
	PspellManager *manager;
	const GeditLanguage *active_lang;
};

struct _GeditLanguage 
{
	gchar *abrev;
	gchar *name;	
};

/* GObject properties */
enum {
	PROP_0 = 0,
	PROP_AVAILABLE_LANGUAGES,
	PROP_LANGUAGE,
	LAST_PROP
};

/* Signals */
enum {
	ADD_WORD_TO_PERSONAL = 0,
	ADD_WORD_TO_SESSION,
	SET_LANGUAGE,
	CLEAR_SESSION,
	LAST_SIGNAL
};

static GeditLanguage known_languages [] = 
{
	{"af", N_("Afrikaans")},
	{"am", N_("Amharic")},
	{"az", N_("Azerbaijani")},
	{"be", N_("Belarusian")},
	{"bg", N_("Bulgarian")},
	{"bn", N_("Bengali")},
	{"br", N_("Breton")},
	{"ca", N_("Catalan")},
	{"cs", N_("Czech")},
	{"cy", N_("Welsh")},
	{"da", N_("Danish")},
	{"de_AT", N_("German (Austria)")},
	{"de_DE", N_("German (Germany)")},
	{"de_CH", N_("German (Swiss)")},
	{"el", N_("Greek")},
	{"en_US", N_("English (American)")},
	{"en_GB", N_("English (British)")},
	{"en_CA", N_("English (Canadian)")},
	{"eo", N_("Esperanto")},
	{"es", N_("Spanish")},
	{"et", N_("Estonian")},
	{"fa", N_("Persian")},	
	{"fi", N_("Finnish")},
	{"fo", N_("Faroese")},
	{"fr_FR", N_("French (France)")},
	{"fr_CH", N_("French (Swiss)")},
	{"ga", N_("Irish")},
	{"gd", N_("Scottish Gaelic")},
	{"gl", N_("Gallegan")},
	{"gv", N_("Manx Gaelic")},
	{"he", N_("Hebrew")},
	{"hi", N_("Hindi")},
	{"hr", N_("Croatian")},
	{"hsb", N_("Upper Sorbian")},
	{"hu", N_("Hungarian")},
	{"ia", N_("Interlingua (IALA)")},
	{"id", N_("Indonesian")},
	{"is", N_("Icelandic")},
	{"it", N_("Italian")},
	{"ku", N_("Kurdish")},
	{"la", N_("Latin")},
	{"lt", N_("Lithuanian")},
	{"lv", N_("Latvian")},
	{"mg", N_("Malagasy")},
	{"mi", N_("Maori")},
	{"mk", N_("Macedonian")},
	{"mn", N_("Mongolian")},
	{"mr", N_("Marathi")},
	{"ms", N_("Malay")},
	{"mt", N_("Maltese")},
	{"nb", N_("Norwegian Bokmal")},
	{"nl", N_("Dutch")},
	{"nn", N_("Norwegian Nynorsk")},
	{"no", N_("Norwegian")},
	{"ny", N_("Nyanja")},
	{"pl", N_("Polish")},
	{"pt_PT", N_("Portuguese (Portugal)")},
	{"pt_BR", N_("Portuguese (Brazilian)")},
	{"qu", N_("Quechua")},
	{"ro", N_("Romanian")},
	{"ru", N_("Russian")},
	{"rw", N_("Kinyarwanda")},
	{"sc", N_("Sardinian")},
	{"sk", N_("Slovak")},
	{"sl", N_("Slovenian")},
	{"sv", N_("Swedish")},
	{"sw", N_("Swahili")},
	{"ta", N_("Tamil")},
	{"tet", N_("Tetum")},
	{"tl", N_("Tagalog")},
	{"tn", N_("Tswana")},
	{"tr", N_("Turkish")},
	{"uk", N_("Ukrainian")},
	{"uz", N_("Uzbek")},
	{"vi", N_("Vietnamese")},
	{"wa", N_("Walloon")},
	{"yi", N_("Yiddish")},
	{"zu", N_("Zulu")},
 	{NULL, NULL}
};

static void	gedit_spell_checker_class_init 		(GeditSpellCheckerClass * klass);

static void	gedit_spell_checker_init 		(GeditSpellChecker *spell_checker);
static void 	gedit_spell_checker_finalize 		(GObject *object);

static gboolean is_digit 				(const char *text, gint length);

/* FIXME: should be freed by the class finalizer */
static GSList *available_languages = NULL;

static GObjectClass *parent_class = NULL;

static guint signals[LAST_SIGNAL] = { 0 };

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

	signals[ADD_WORD_TO_PERSONAL] =
	    g_signal_new ("add_word_to_personal",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GeditSpellCheckerClass, add_word_to_personal),
			  NULL, NULL,
			  gedit_marshal_VOID__STRING_INT,
			  G_TYPE_NONE, 
			  2, 
			  G_TYPE_STRING,
			  G_TYPE_INT);

	signals[ADD_WORD_TO_SESSION] =
	    g_signal_new ("add_word_to_session",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GeditSpellCheckerClass, add_word_to_session),
			  NULL, NULL,
			  gedit_marshal_VOID__STRING_INT,
			  G_TYPE_NONE, 
			  2, 
			  G_TYPE_STRING,
			  G_TYPE_INT);

	signals[SET_LANGUAGE] =
	    g_signal_new ("set_language",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GeditSpellCheckerClass, set_language),
			  NULL, NULL,
			  gedit_marshal_VOID__POINTER,
			  G_TYPE_NONE, 
			  1, 
			  G_TYPE_POINTER);

	signals[CLEAR_SESSION] =
	    g_signal_new ("clear_session",
			  G_OBJECT_CLASS_TYPE (object_class),
			  G_SIGNAL_RUN_LAST,
			  G_STRUCT_OFFSET (GeditSpellCheckerClass, clear_session),
			  NULL, NULL,
			  gedit_marshal_VOID__VOID,
			  G_TYPE_NONE, 
			  0); 
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
}

static const GeditLanguage *
get_language_from_abrev (const gchar *abrev)
{
	const GSList *langs;

	g_return_val_if_fail (abrev != NULL, NULL);

	langs = gedit_spell_checker_get_available_languages ();

	while (langs != NULL)
	{
		const GeditLanguage *l = (const GeditLanguage *)langs->data;

		if (g_ascii_strncasecmp (abrev, l->abrev, strlen (l->abrev)) == 0)
			return l;

		langs = g_slist_next (langs);
	}

	return NULL;
}

gchar *
gedit_language_to_key (const GeditLanguage *lang)
{
	return g_strdup (lang->abrev);
}

const GeditLanguage *
gedit_language_from_key	(const gchar *key)
{
	return get_language_from_abrev (key);
}

static gboolean
lazy_init (GeditSpellChecker *spell, const GeditLanguage *language, GError **error) 
{
	PspellConfig *config;
	PspellCanHaveError *err;
	
	g_return_val_if_fail (spell != NULL, FALSE);

	if (spell->manager != NULL)
		return TRUE;

	config = new_pspell_config ();
	g_return_val_if_fail (config != NULL, FALSE);
	
/*
	g_print ("lazy_init: Language: %s\n", gedit_language_to_string (language));
*/
	pspell_config_replace (config, "encoding", "utf-8");
	
	pspell_config_replace (config, "mode", "url");

	if (language != NULL)
	{
		if (get_language_from_abrev (language->abrev) != NULL)
		{
			spell->active_lang = language;
		}
	}
	else	
	{
		/* First try to get a default language */	
		const gchar *language_tag;

		language_tag = pspell_config_retrieve (config, "language-tag");

		if (language_tag != NULL)
		{
			spell->active_lang = get_language_from_abrev (language_tag);
		}

/*
		g_print ("Language tag: %s\n", language_tag);
*/
	}	

	/* Second try to get a default language */

	if (spell->active_lang == NULL)
		spell->active_lang = get_language_from_abrev ("en_us");

	/* Last try to get a default language */
	if (spell->active_lang == NULL)
	{
		const GSList *langs;
		langs = gedit_spell_checker_get_available_languages ();
		if (langs != NULL)
			spell->active_lang = (const GeditLanguage *)langs->data;
	}

	if (spell->active_lang != NULL)
	{
		pspell_config_replace (config, "language-tag", spell->active_lang->abrev);
	}
/*
	g_print ("Language: %s\n", gedit_language_to_string (spell->active_lang ));
*/	
	err = new_pspell_manager (config);

	delete_pspell_config (config);

	if (pspell_error_number (err) != 0) 
	{
		spell->active_lang = NULL;

		if (error != NULL)
			g_set_error (error, GEDIT_SPELL_CHECKER_ERROR, GEDIT_SPELL_CHECKER_ERROR_PSPELL,
				"pspell: %s", pspell_error_message (err));

		return FALSE;
	} 
	
	if (spell->manager != NULL)
		delete_pspell_manager (spell->manager);
	
	spell->manager = to_pspell_manager (err);
	g_return_val_if_fail (spell->manager != NULL, FALSE);
	
	return TRUE;
}

const GSList *
gedit_spell_checker_get_available_languages (void)
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

gboolean
gedit_spell_checker_set_language (GeditSpellChecker *spell, 
			const GeditLanguage *language,
			GError **error)
{
	gboolean ret;
	
	g_return_val_if_fail (spell != NULL, FALSE);
	g_return_val_if_fail (GEDIT_IS_SPELL_CHECKER (spell), FALSE);

	if (spell->manager != NULL)
	{
		delete_pspell_manager (spell->manager);
		spell->manager = NULL;
	}

	ret = lazy_init (spell, language, error);

	if (ret)
		g_signal_emit (G_OBJECT (spell), signals[SET_LANGUAGE], 0, language);

	return ret;
}

const GeditLanguage *
gedit_spell_checker_get_language (GeditSpellChecker *spell)
{
	g_return_val_if_fail (spell != NULL, NULL);
	g_return_val_if_fail (GEDIT_IS_SPELL_CHECKER (spell), NULL);

	if (!lazy_init (spell, spell->active_lang, NULL))
		return NULL;

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

	if (!lazy_init (spell, spell->active_lang, error))
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
			g_return_val_if_reached (FALSE);
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

	if (!lazy_init (spell, spell->active_lang, error))
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

	if (!lazy_init (spell, spell->active_lang, error))
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

	g_signal_emit (G_OBJECT (spell), signals[ADD_WORD_TO_PERSONAL], 0, word, len);
	
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

	if (!lazy_init (spell, spell->active_lang, error))
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
	
	g_signal_emit (G_OBJECT (spell), signals[ADD_WORD_TO_SESSION], 0, word, len);

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

	g_signal_emit (G_OBJECT (spell), signals[CLEAR_SESSION], 0);

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

	if (!lazy_init (spell, spell->active_lang, error))
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
