#ifndef __DEBUG_H__
#define __DEBUG_H__

typedef enum {
	GEDIT_DEBUG_VIEW,
	GEDIT_DEBUG_UNDO,
	GEDIT_DEBUG_SEARCH,
	GEDIT_DEBUG_PRINT,
	GEDIT_DEBUG_PREFS,
	GEDIT_DEBUG_PLUGINS,
	GEDIT_DEBUG_FILE,
	GEDIT_DEBUG_DOCUMENT,
	GEDIT_DEBUG_RECENT,
	GEDIT_DEBUG_COMMANDS,
	GEDIT_DEBUG_WINDOW
} DebugSection;

extern gint debug;
extern gint debug_view;
extern gint debug_undo;
extern gint debug_search;
extern gint debug_print;
extern gint debug_prefs;
extern gint debug_plugins;
extern gint debug_file;
extern gint debug_document;
extern gint debug_commands;
extern gint debug_recent;
extern gint debug_window;

/* __FUNCTION_ is not defined in Irix according to David Kaelbling <drk@sgi.com>*/
#ifndef __GNUC__
#define __FUNCTION__   ""
#endif

#define	DEBUG_VIEW	GEDIT_DEBUG_VIEW,    __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_UNDO	GEDIT_DEBUG_UNDO,    __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_SEARCH	GEDIT_DEBUG_SEARCH,  __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_PRINT	GEDIT_DEBUG_PRINT,   __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_PREFS	GEDIT_DEBUG_PREFS,   __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_PLUGINS	GEDIT_DEBUG_PLUGINS, __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_FILE	GEDIT_DEBUG_FILE,    __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_DOCUMENT	GEDIT_DEBUG_DOCUMENT,__FILE__, __LINE__, __FUNCTION__
#define	DEBUG_RECENT	GEDIT_DEBUG_RECENT,  __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_COMMANDS	GEDIT_DEBUG_COMMANDS,__FILE__, __LINE__, __FUNCTION__
#define	DEBUG_WINDOW	GEDIT_DEBUG_WINDOW,  __FILE__, __LINE__, __FUNCTION__

void gedit_debug (gint section, gchar *file,
		  gint line, gchar* function, gchar* format, ...);

#endif /* __DEBUG_H__ */
