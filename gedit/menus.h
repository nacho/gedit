#ifndef __MENUS_H__
#define __MENUS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

   void gE_menus_init (gE_window *window, gE_data *data);
   void menus_create(GtkMenuEntry *entries, int nmenu_entries);
   
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MENUS_H__ */
