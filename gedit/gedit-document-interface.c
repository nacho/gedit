/*
 * gedit-document-interface.c
 * This file is part of gedit
 *
 * Copyright (C) 2009 - Ignacio Casal Quinteiro
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


#include "gedit-document-interface.h"
#include "gedit-marshal.h"
#include "gedit-enum-types.h"
#include "gedit-utils.h"

enum {
	MODIFIED_CHANGED,
	LOAD,
	LOADING,
	LOADED,
	SAVE,
	SAVING,
	SAVED,
	LAST_SIGNAL
};

static guint document_signals[LAST_SIGNAL] = { 0 };

/* Default implementation */

static GFile *
gedit_document_get_location_default (GeditDocument *doc)
{
	g_return_val_if_reached (NULL);
}

static gchar *
gedit_document_get_uri_default (GeditDocument *doc)
{
	g_return_val_if_reached (NULL);
}

static void
gedit_document_set_uri_default (GeditDocument *doc,
				const gchar   *uri)
{
	g_return_if_reached ();
}

static gchar *
gedit_document_get_uri_for_display_default (GeditDocument *doc)
{
	g_return_val_if_reached (NULL);
}

static gchar *
gedit_document_get_short_name_for_display_default (GeditDocument *doc)
{
	g_return_val_if_reached (NULL);
}

static gchar *
gedit_document_get_content_type_default (GeditDocument *doc)
{
	g_return_val_if_reached (NULL);
}

static gchar *
gedit_document_get_mime_type_default (GeditDocument *doc)
{
	g_return_val_if_reached (NULL);
}

static gboolean
gedit_document_get_readonly_default (GeditDocument *doc)
{
	g_return_val_if_reached (FALSE);
}

static gboolean
gedit_document_load_cancel_default (GeditDocument *doc)
{
	g_return_val_if_reached (FALSE);
}

static gboolean
gedit_document_is_untouched_default (GeditDocument *doc)
{
	g_return_val_if_reached (FALSE);
}

static gboolean
gedit_document_is_untitled_default (GeditDocument *doc)
{
	g_return_val_if_reached (FALSE);
}

static gboolean
gedit_document_is_local_default (GeditDocument *doc)
{
	g_return_val_if_reached (FALSE);
}

static gboolean
gedit_document_get_deleted_default (GeditDocument *doc)
{
	g_return_val_if_reached (FALSE);
}

static gboolean
gedit_document_goto_line_default (GeditDocument *doc,
				  gint           line)
{
	g_return_val_if_reached (FALSE);
}

static gboolean
gedit_document_goto_line_offset_default (GeditDocument *doc,
					 gint           line,
					 gint           line_offset)
{
	g_return_val_if_reached (FALSE);
}

static const GeditEncoding *
gedit_document_get_encoding_default (GeditDocument *doc)
{
	g_return_val_if_reached (NULL);
}

static glong
gedit_document_get_seconds_since_last_save_or_load_default (GeditDocument *doc)
{
	g_return_val_if_reached (0);
}

static gboolean
gedit_document_check_externally_modified_default (GeditDocument *doc)
{
	g_return_val_if_reached (FALSE);
}

static void
gedit_document_undo_default (GeditDocument *doc)
{
	g_return_if_reached ();
}

static void
gedit_document_redo_default (GeditDocument *doc)
{
	g_return_if_reached ();
}

static gboolean
gedit_document_can_undo_default (GeditDocument *doc)
{
	g_return_val_if_reached (FALSE);
}

static gboolean
gedit_document_can_redo_default (GeditDocument *doc)
{
	g_return_val_if_reached (FALSE);
}

static void
gedit_document_begin_not_undoable_action_default (GeditDocument *doc)
{
	g_return_if_reached ();
}

static void
gedit_document_end_not_undoable_action_default (GeditDocument *doc)
{
	g_return_if_reached ();
}

static void
gedit_document_set_text_default (GeditDocument *doc,
				 const gchar   *text,
				 gint           len)
{
	g_return_if_reached ();
}

static void
gedit_document_set_modified_default (GeditDocument *doc,
				     gboolean       setting)
{
	g_return_if_reached ();
}

static gboolean
gedit_document_get_modified_default (GeditDocument *doc)
{
	g_return_val_if_reached (FALSE);
}

static gboolean
gedit_document_get_has_selection_default (GeditDocument *doc)
{
	g_return_val_if_reached (FALSE);
}

static void 
gedit_document_init (GeditDocumentIface *iface)
{
	static gboolean initialized = FALSE;
	
	iface->get_location = gedit_document_get_location_default;
	iface->get_uri = gedit_document_get_uri_default;
	iface->set_uri = gedit_document_set_uri_default;
	iface->get_uri_for_display = gedit_document_get_uri_for_display_default;
	iface->get_short_name_for_display = gedit_document_get_short_name_for_display_default;
	iface->get_content_type = gedit_document_get_content_type_default;
	iface->get_mime_type = gedit_document_get_mime_type_default;
	iface->get_readonly = gedit_document_get_readonly_default;
	iface->load_cancel = gedit_document_load_cancel_default;
	iface->is_untouched = gedit_document_is_untouched_default;
	iface->is_untitled = gedit_document_is_untitled_default;
	iface->is_local = gedit_document_is_local_default;
	iface->get_deleted = gedit_document_get_deleted_default;
	iface->goto_line = gedit_document_goto_line_default;
	iface->goto_line_offset = gedit_document_goto_line_offset_default;
	iface->get_encoding = gedit_document_get_encoding_default;
	iface->get_seconds_since_last_save_or_load = gedit_document_get_seconds_since_last_save_or_load_default;
	iface->check_externally_modified = gedit_document_check_externally_modified_default;
	iface->undo = gedit_document_undo_default;
	iface->redo = gedit_document_redo_default;
	iface->can_undo = gedit_document_can_undo_default;
	iface->can_redo = gedit_document_can_redo_default;
	iface->begin_not_undoable_action = gedit_document_begin_not_undoable_action_default;
	iface->end_not_undoable_action = gedit_document_end_not_undoable_action_default;
	iface->set_text = gedit_document_set_text_default;
	iface->set_modified = gedit_document_set_modified_default;
	iface->get_modified = gedit_document_get_modified_default;
	iface->get_has_selection = gedit_document_get_has_selection_default;
	
	if (!initialized)
	{
		g_object_interface_install_property (iface,
						     g_param_spec_string ("uri",
									  "URI",
									  "The document's URI",
									  NULL,
									  G_PARAM_READABLE |
									  G_PARAM_STATIC_STRINGS));

		g_object_interface_install_property (iface,
						     g_param_spec_string ("shortname",
									  "Short Name",
									  "The document's short name",
									  NULL,
									  G_PARAM_READABLE |
									  G_PARAM_STATIC_STRINGS));

		g_object_interface_install_property (iface,
						     g_param_spec_string ("content-type",
									  "Content Type",
									  "The document's Content Type",
									  NULL,
									  G_PARAM_READABLE |
									  G_PARAM_STATIC_STRINGS));

		g_object_interface_install_property (iface,
						     g_param_spec_boolean ("read-only",
									   "Read Only",
									   "Whether the document is read only or not",
									   FALSE,
									   G_PARAM_READABLE |
									   G_PARAM_STATIC_STRINGS));
	
		document_signals[MODIFIED_CHANGED] =
			g_signal_new ("modified-changed",
				      G_TYPE_FROM_INTERFACE (iface),
				      G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET (GeditDocumentIface, modified_changed),
				      NULL, NULL,
				      gedit_marshal_VOID__VOID,
				      G_TYPE_NONE,
				      0);
		
		/**
		 * GeditDocument::load:
		 * @document: the #GeditDocument.
		 * @uri: the uri where to load the document from.
		 * @encoding: the #GeditEncoding to encode the document.
		 * @line_pos: the line to show.
		 * @create: whether the document should be created if it doesn't exist.
		 *
		 * The "load" signal is emitted when a document is loaded.
		 */
		document_signals[LOAD] =
			g_signal_new ("load",
				      G_TYPE_FROM_INTERFACE (iface),
				      G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET (GeditDocumentIface, load),
				      NULL, NULL,
				      gedit_marshal_VOID__STRING_BOXED_INT_BOOLEAN,
				      G_TYPE_NONE,
				      4,
				      G_TYPE_STRING,
				      /* we rely on the fact that the GeditEncoding pointer stays
				       * the same forever */
				      GEDIT_TYPE_ENCODING | G_SIGNAL_TYPE_STATIC_SCOPE,
				      G_TYPE_INT,
				      G_TYPE_BOOLEAN);


		document_signals[LOADING] =
	   		g_signal_new ("loading",
				      G_TYPE_FROM_INTERFACE (iface),
				      G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET (GeditDocumentIface, loading),
				      NULL, NULL,
				      gedit_marshal_VOID__UINT64_UINT64,
				      G_TYPE_NONE,
				      2,
				      G_TYPE_UINT64,
				      G_TYPE_UINT64);

		document_signals[LOADED] =
	   		g_signal_new ("loaded",
				      G_TYPE_FROM_INTERFACE (iface),
				      G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET (GeditDocumentIface, loaded),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__POINTER,
				      G_TYPE_NONE,
				      1,
				      G_TYPE_POINTER);

		/**
		 * GeditDocument::save:
		 * @document: the #GeditDocument.
		 * @uri: the uri where the document is about to be saved.
		 * @encoding: the #GeditEncoding used to save the document.
		 * @flags: the #GeditDocumentSaveFlags for the save operation.
		 *
		 * The "save" signal is emitted when the document is saved.
		 */
		document_signals[SAVE] =
			g_signal_new ("save",
				      G_TYPE_FROM_INTERFACE (iface),
				      G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET (GeditDocumentIface, save),
				      NULL, NULL,
				      gedit_marshal_VOID__STRING_BOXED_FLAGS,
				      G_TYPE_NONE,
				      3,
				      G_TYPE_STRING,
				      /* we rely on the fact that the GeditEncoding pointer stays
				       * the same forever */
				      GEDIT_TYPE_ENCODING | G_SIGNAL_TYPE_STATIC_SCOPE,
				      GEDIT_TYPE_DOCUMENT_SAVE_FLAGS);

		document_signals[SAVING] =
			g_signal_new ("saving",
				      G_TYPE_FROM_INTERFACE (iface),
				      G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET (GeditDocumentIface, saving),
				      NULL, NULL,
				      gedit_marshal_VOID__UINT64_UINT64,
				      G_TYPE_NONE,
				      2,
				      G_TYPE_UINT64,
				      G_TYPE_UINT64);

		document_signals[SAVED] =
			g_signal_new ("saved",
				      G_TYPE_FROM_INTERFACE (iface),
				      G_SIGNAL_RUN_LAST,
				      G_STRUCT_OFFSET (GeditDocumentIface, saved),
				      NULL, NULL,
				      g_cclosure_marshal_VOID__POINTER,
				      G_TYPE_NONE,
				      1,
				      G_TYPE_POINTER);
	
		initialized = TRUE;
	}
}

GType 
gedit_document_get_type ()
{
	static GType gedit_document_type_id = 0;
	
	if (!gedit_document_type_id)
	{
		static const GTypeInfo g_define_type_info =
		{
			sizeof (GeditDocumentIface),
			(GBaseInitFunc) gedit_document_init, 
			NULL,
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL
		};
		
		gedit_document_type_id = 
			g_type_register_static (G_TYPE_INTERFACE,
						"GeditDocument",
						&g_define_type_info,
						0);
	
		g_type_interface_add_prerequisite (gedit_document_type_id,
						   G_TYPE_OBJECT);
	}
	
	return gedit_document_type_id;
}


GQuark
gedit_document_error_quark (void)
{
	static GQuark quark = 0;

	if (G_UNLIKELY (quark == 0))
		quark = g_quark_from_static_string ("gedit_io_load_error");

	return quark;
}

GFile *
gedit_document_get_location (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_DOCUMENT (doc), NULL);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->get_location (doc);
}

gchar *
gedit_document_get_uri (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_DOCUMENT (doc), NULL);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->get_uri (doc);
}

void
gedit_document_set_uri (GeditDocument *doc,
			const gchar   *uri)
{
	g_return_if_fail (GEDIT_DOCUMENT (doc));
	GEDIT_DOCUMENT_GET_INTERFACE (doc)->set_uri (doc, uri);
}

gchar *
gedit_document_get_uri_for_display (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_DOCUMENT (doc), g_strdup (""));
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->get_uri_for_display (doc);
}

gchar *
gedit_document_get_short_name_for_display (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_DOCUMENT (doc), g_strdup (""));
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->get_short_name_for_display (doc);
}

/* FIXME: Only gedit-text-buffer? */
gchar *
gedit_document_get_content_type (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_DOCUMENT (doc), NULL);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->get_content_type (doc);
}

/* FIXME: Only gedit-text-buffer? */
gchar *
gedit_document_get_mime_type (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_DOCUMENT (doc), g_strdup ("text/plain"));
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->get_mime_type (doc);
}

gboolean
gedit_document_get_readonly (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_DOCUMENT (doc), FALSE);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->get_readonly (doc);
}

/**
 * gedit_document_load_document:
 * @doc: the #GeditDocument.
 * @uri: the uri where to load the document from.
 * @encoding: the #GeditEncoding to encode the document.
 * @line_pos: the line to show.
 * @create: whether the document should be created if it doesn't exist.
 *
 * Load a document. This results in the "load" signal to be emitted.
 */
void
gedit_document_load (GeditDocument       *doc,
		     const gchar         *uri,
		     const GeditEncoding *encoding,
		     gint                 line_pos,
		     gboolean             create)
{
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (uri != NULL);
	g_return_if_fail (gedit_utils_is_valid_uri (uri));
	
	g_signal_emit (doc, document_signals[LOAD], 0, uri, encoding, line_pos, create);
}

/**
 * gedit_document_load_cancel:
 * @doc: the #GeditDocument.
 *
 * Cancel load of a document.
 */
gboolean
gedit_document_load_cancel (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_DOCUMENT (doc), FALSE);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->load_cancel (doc);
}

/**
 * gedit_document_save:
 * @doc: the #GeditDocument.
 * @flags: optionnal #GeditDocumentSaveFlags.
 *
 * Save the document to its previous location. This results in the "save"
 * signal to be emitted.
 */
void
gedit_document_save (GeditDocument       *doc,
		     GeditDocumentSaveFlags flags)
{
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (gedit_document_get_uri (doc) != NULL);

	g_signal_emit (doc,
		       document_signals[SAVE],
		       0,
		       gedit_document_get_uri (doc),
		       gedit_document_get_encoding (doc),
		       flags);
}

/**
 * gedit_document_save_as:
 * @doc: the #GeditDocument.
 * @uri: the uri where to save the document.
 * @encoding: the #GeditEncoding to encode the document.
 * @flags: optionnal #GeditDocumentSaveFlags.
 *
 * Save the document to a new location. This results in the "save" signal
 * to be emitted.
 */
void
gedit_document_save_as (GeditDocument       *doc,
			const gchar         *uri,
			const GeditEncoding *encoding,
			GeditDocumentSaveFlags flags)
{
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (uri != NULL);
	g_return_if_fail (encoding != NULL);

	/* priv->mtime refers to the the old uri (if any). Thus, it should be
	 * ignored when saving as. */
	g_signal_emit (doc,
		       document_signals[SAVE],
		       0,
		       uri,
		       encoding,
		       flags | GEDIT_DOCUMENT_SAVE_IGNORE_MTIME);
}

gboolean
gedit_document_is_untouched (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_DOCUMENT (doc), FALSE);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->is_untouched (doc);
}

gboolean
gedit_document_is_untitled (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_DOCUMENT (doc), FALSE);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->is_untitled (doc);
}

gboolean
gedit_document_is_local (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_DOCUMENT (doc), FALSE);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->is_local (doc);
}

gboolean
gedit_document_get_deleted (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_DOCUMENT (doc), FALSE);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->get_deleted (doc);
}

gboolean
gedit_document_goto_line (GeditDocument *doc,
			  gint           line)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	g_return_val_if_fail (line >= -1, FALSE);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->goto_line (doc, line);
}

gboolean
gedit_document_goto_line_offset (GeditDocument *doc,
				 gint           line,
				 gint           line_offset)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	g_return_val_if_fail (line >= -1, FALSE);
	g_return_val_if_fail (line_offset >= -1, FALSE);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->goto_line_offset (doc, line,
								     line_offset);
}

const GeditEncoding *
gedit_document_get_encoding (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->get_encoding (doc);
}

glong
gedit_document_get_seconds_since_last_save_or_load (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), -1);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->get_seconds_since_last_save_or_load (doc);
}

gboolean
gedit_document_check_externally_modified (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->check_externally_modified (doc);
}

void
gedit_document_undo (GeditDocument *doc)
{
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	GEDIT_DOCUMENT_GET_INTERFACE (doc)->undo (doc);
}

void
gedit_document_redo (GeditDocument *doc)
{
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	GEDIT_DOCUMENT_GET_INTERFACE (doc)->redo (doc);
}

gboolean
gedit_document_can_undo (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->can_undo (doc);
}

gboolean
gedit_document_can_redo (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->can_redo (doc);
}

void
gedit_document_begin_not_undoable_action (GeditDocument *doc)
{
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	GEDIT_DOCUMENT_GET_INTERFACE (doc)->begin_not_undoable_action (doc);
}

void
gedit_document_end_not_undoable_action (GeditDocument *doc)
{
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	GEDIT_DOCUMENT_GET_INTERFACE (doc)->end_not_undoable_action (doc);
}

void
gedit_document_set_text (GeditDocument *doc,
			 const gchar   *text,
			 gint           len)
{
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (text != NULL);
	GEDIT_DOCUMENT_GET_INTERFACE (doc)->set_text (doc, text, len);
}

void
gedit_document_set_modified (GeditDocument *doc,
			     gboolean       setting)
{
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	GEDIT_DOCUMENT_GET_INTERFACE (doc)->set_modified (doc, setting);
}

gboolean
gedit_document_get_modified (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->get_modified (doc);
}

gboolean
gedit_document_get_has_selection (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	return GEDIT_DOCUMENT_GET_INTERFACE (doc)->get_has_selection (doc);
}
