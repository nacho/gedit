#ifndef __GEDIT_PRINT_DOC_H__
#define __GEDIT_PRINT_DOC_H__

#include <print.h>

void  gedit_print_document                 (PrintJobInfo *pji);
guint gedit_print_document_determine_lines (PrintJobInfo *pji, gboolean real);

#endif /* __GEDIT_PRINT_DOC_H__ */
