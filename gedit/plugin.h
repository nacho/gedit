/* plugin.h - plugin header files.
 *
 * Copyright (C) 1998 Chris Lahey.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#if 0
#include <pthread.h>
#endif
#include <unistd.h>
#include <glib.h>

typedef enum {
	PLUGIN_STANDARD,
	PLUGIN_GMODULE
} PluginType;

typedef struct
{
	gint start;
	gint end;
} selection_range;

typedef struct
{
	gchar *menu_location;
	gchar *suggested_accelerator;
	gchar *plugin_name;
	PluginType type;
	gpointer user_data;
} plugin_info;

typedef struct
{
	gint (*create) ( gint context, gchar *title );
	gint (*open) ( gint context, gchar *title );
	void (*show) ( gint id );
	gchar* (*filename) ( gint id );
	gint (*current) ( gint context );
	gboolean (*close) ( gint id );
	gint (*get_position) ( gint id );
	selection_range (*get_selection) ( gint id );
	void (*set_auto_indent) ( gint id, gint auto_indent );
	void (*set_status_bar) ( gint id, gint status_bar );
	void (*set_word_wrap) ( gint id, gint word_wrap );
	void (*set_line_wrap) ( gint id, gint line_wrap );
	void (*set_read_only) ( gint id, gint read_only );
	void (*set_split_screen) ( gint id, gint split_screen );
	/*
	  void (*set_position) ( gint id, gint position );
	  void (*set_selection) ( gint id, selection_range selection );
	  void (*goto_line) ( gint id, gint line );
	*/
} plugin_document_callbacks;

typedef struct
{
	void (*append) ( gint id, gchar *data, gint length );
	void (*insert) ( gint id, gchar *data, gint length, gint position );
	gchar* (*get) ( gint id );
	gchar* (*get_selected_text) ( gint id );
	gchar* (*set_selected_text) ( gint id, gchar *data, gint length );
	/*
	  void (*delete_selected_text) ( gint id );
	*/
} plugin_text_callbacks;

typedef struct
{
	gboolean (*quit) ();
	void (*reg) ( plugin_info *info );
} plugin_program_callbacks;

typedef struct
{
	plugin_document_callbacks document;
	plugin_text_callbacks text;
	plugin_program_callbacks program;
} plugin_callback_struct;

extern plugin_callback_struct pl_callbacks;

typedef struct
{
	int pipe_to;
	int pipe_from;
	int pipe_data;
	int pid;
	char *name;
	int in_call;
	plugin_callback_struct callbacks;
	int context;
#if 0
	pthread_t thread;
#endif
} plugin;

typedef struct
{
	int command[2];
	int command_count;
	int command_current_count;

	int ints[10];
	int int_count;
	int int_current_count;

	gchar *chars[10];
	int char_count;
	int char_current_count;
	gboolean getting_int;
} plugin_parse_state;

typedef struct
{
	char *name;
	char *location; /* or should that be Path? */
	/* Other options may become available later.. Author, date, etc.. */
} plugin_list_data;

extern GList *plugin_list;


typedef void plugin_callback( plugin *, gchar *, int length, gpointer data );

plugin *plugin_new( gchar * );
plugin *plugin_new_with_param( gchar *, int argc, gchar *argv[] );
plugin *plugin_query( gchar * );

plugin *custom_plugin_new (gchar *path, gchar *plugin_name);
plugin *custom_plugin_new_with_param (gchar *path, gchar *plugin_name, int argc, gchar *argv[]);
void custom_plugin_query (gchar *path, gchar *plugin_name, plugin_callback_struct *callbacks);
void custom_plugin_query_all( gchar *, plugin_callback_struct * );

void plugin_query_all( plugin_callback_struct * );
void plugin_send( plugin *, gchar *, gint length );
void plugin_send_int( plugin *, gint );
void plugin_send_data( plugin *, gchar *, gint length );
void plugin_send_data_int( plugin *, gint );
void plugin_get( plugin *, gchar *, gint length );
void plugin_get_all( plugin *, gint length, plugin_callback *finished, gpointer data );
void plugin_register( plugin *, plugin_callback_struct *, gint context );

void plugin_load_list (gchar *app);
void plugin_save_list ();

#endif /* __PLUGIN_H__ */
