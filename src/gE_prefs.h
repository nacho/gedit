#ifndef __GEPREFS_H__
#define __GEPREFS_H__

#include <gtk/gtk.h>
/*
#define PREFS_FILE	"./gE_rc";


enum keyword_type {
  KEYWORD_INT,
  KEYWORD_STRING
};

struct keyword {
  char *name;
  enum keyword_type required;
  gpointer target;
};


#define BUFFER_SIZE	1024
#define STATE_SPACE	0
#define STATE_TOKEN	1
#define STATE_INT	2
#define STATE_STRING	3
#define	STATE_COMMENT	4

#define TOKEN_EOF	0
#define TOKEN_KEYWORD	1
#define TOKEN_INT	2
#define TOKEN_STRING	3
#define TOKEN_COMMA	4
#define TOKEN_EOL	5
*/
int wordwrap;
char *textfont;

char *gettime(char *buf);

/*
static struct keyword keywords[] = {
   { "wwrap",	KEYWORD_INT, &wordwrap },
   { "tfont",	KEYWORD_STRING, &textfont },
   
   { NULL,	0,	NULL }
 };

char *strdup_strip (const char *);

int gE_parse ();
int gE_pref_save ();*/

#endif /* __GEPREFS_H__ */
