#ifndef __MENUS_H__
#define __MENUS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef GTK_HAVE_ACCEL_GROUP
   void get_main_menu (GtkWidget **menubar, GtkAccelGroup **group);
#else
   void get_main_menu (GtkWidget **menubar, GtkAcceleratorTable **table);
#endif
   void menus_create(GtkMenuEntry *entries, int nmenu_entries);
   
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENUS_H__ */
