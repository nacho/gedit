#ifndef __GEDIT_PRINT_UTIL_H__
#define __GEDIT_PRINT_UTIL_H__

int      gedit_print_show (GnomePrintContext *pc, char const *text);
gboolean gedit_print_verify_fonts   (void);



void gedit_print_progress_start (PrintJobInfo *pji);
void gedit_print_progress_tick  (PrintJobInfo *pji, gint page);
void gedit_print_progress_end   (PrintJobInfo *pji);

#endif /* __GEDIT_PRINT_UTIL_H__ */
