/*
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */ 
/******************************************************************/
/* This tool is written by Mikael Hermansson <mikeh@algonet.se>   */
/*                    Copyright 1998                              */
/******************************************************************/

#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include "maincode.h"
#include "msgbox.h"
#include "client.h"
#define INCLUDE_DIR1 "/usr/include"
#define INCLUDE_DIR2 "/usr/local/include"
/*///////////////////////// function defines   */
prj_manager_window* prj_create_window();
void create_menubar(prj_manager_menubar *);
void prj_create_signals(prj_manager_window *);
void prj_callback_fileopen(GtkWidget *o,gpointer ptr);
void fileopen_callback_ok(GtkWidget *o,gpointer ptr);
void prj_create_treelist(gchar *fname,prj_manager_window *ptr);
void prj_tree_callback_buttonclick (GtkWidget *pt,  GdkEventButton *ev,gpointer p);
void prj_tree_callback_selchange(GtkCTree *pt,GtkCTreeNode *row,gint col,gpointer p);
/*/////////////////////////////////////*/
void destroy(GtkWidget *w,gpointer ptr)
{
	g_free((prj_manager_window *)ptr);
	gtk_main_quit();
}
void data_callback_destroy(GtkWidget *widget,gpointer dat)
{		
	prj_data *data=(prj_data *)dat;
	data->ready=TRUE;
}
void data_callback_cancel(GtkWidget *o,gpointer dat)
{
	prj_data *data=(prj_data *)dat;
	data->ready=TRUE;
}
void data_callback_ok(GtkWidget *o,gpointer dat)
{
	prj_data *data=(prj_data*)dat;
	
	data->ok=TRUE;
	data->ready=TRUE;
}
gint close_window(GtkWidget *w,GdkEvent *e,gpointer d)
{
	return (FALSE);        /* ok to close */
}
void modal_loop(prj_data *data)
{	
	data->ready=FALSE;
	gtk_grab_add(data->widget);
	while (!data->ready) {
		gtk_main_iteration_do(TRUE);
	};
	gtk_grab_remove(data->widget);
}
int main(int argc,char *argv[])
{
	prj_manager_window *ptr;
	gint context;
	client_info info = empty_info;
	context=0;
	
	info.menu_location="[Plugins]Project Manager";
	#ifdef PLUGIN
		context=client_init(&argc,&argv,&info);
	#endif
	gtk_init(&argc,&argv);
	if((ptr=prj_create_window())){	
		ptr->plugin_context=context;		
		gtk_main();
		g_free(ptr);
	}
	exit( 0);
}
prj_manager_window * prj_create_window()
{
	prj_manager_window *ptr=g_malloc(sizeof(prj_manager_window));
	if(!ptr)
	{	g_print("Cannot aloc memory"); return FALSE; }
	ptr->menubar=g_malloc(sizeof(prj_manager_menubar));	
	
	strcpy(ptr->test,"only checkptr");
	ptr->window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(ptr->window),PROGRAMTITLE);
	gtk_widget_set_usize(GTK_WIDGET(ptr->window),150,250);	
	ptr->vbox=gtk_vbox_new(FALSE,FALSE);
	gtk_container_add(GTK_CONTAINER(ptr->window),ptr->vbox);
	ptr->tree=gtk_ctree_new(1,0);
	create_menubar(ptr->menubar);
	
	gtk_box_pack_start(GTK_BOX(ptr->vbox),ptr->menubar->bar,FALSE,FALSE,0);
	gtk_box_pack_start(GTK_BOX(ptr->vbox),ptr->tree,TRUE,TRUE,0);
	
	prj_create_signals(ptr);
	
	gtk_widget_show(ptr->menubar->bar);
	gtk_widget_show(ptr->window);
	gtk_widget_show(ptr->vbox);
	gtk_widget_show(ptr->tree);
		
	return ptr;		
}
void create_menubar(prj_manager_menubar *mb)
{
	mb->bar=gtk_menu_bar_new();
	mb->fileroot=gtk_menu_new();
	mb->filemenu=gtk_menu_item_new_with_label("File");
	mb->filemenu_open=gtk_menu_item_new_with_label("Open");
	gtk_menu_append(GTK_MENU(mb->fileroot),mb->filemenu_open);
	
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mb->filemenu),mb->fileroot);
	gtk_menu_bar_append(GTK_MENU_BAR(mb->bar),mb->filemenu);
	gtk_widget_show(mb->filemenu);
	gtk_widget_show(mb->filemenu_open);
}
void prj_create_signals(prj_manager_window *ptr)
{
	gtk_signal_connect(GTK_OBJECT(ptr->window),"delete_event",
				GTK_SIGNAL_FUNC(close_window),ptr);
	gtk_signal_connect(GTK_OBJECT(ptr->window),"destroy",
				GTK_SIGNAL_FUNC(destroy),ptr);

	gtk_signal_connect(GTK_OBJECT(ptr->menubar->filemenu_open),"activate",
				GTK_SIGNAL_FUNC(prj_callback_fileopen),ptr);
	
	gtk_signal_connect(GTK_OBJECT(ptr->tree),"tree_select_row",
		GTK_SIGNAL_FUNC(prj_tree_callback_selchange),ptr);

	gtk_signal_connect(GTK_OBJECT(ptr->tree),"button_press_event",
		GTK_SIGNAL_FUNC(prj_tree_callback_buttonclick),ptr);	
}
void prj_tree_callback_selchange(GtkCTree *pt,GtkCTreeNode* row,gint col,gpointer p)
{
	prj_manager_window *ptr=(prj_manager_window *)p;
	ptr->rowptr=row;
}
void prj_tree_callback_buttonclick (GtkWidget *pt,  GdkEventButton *ev,gpointer p)
{
	prj_tree_data *treedata;
	GList *rowptr; 
	prj_manager_window *ptr=(prj_manager_window *)p; 
	gchar *fname;
	if (ev->type == GDK_BUTTON_PRESS && ev->button==3 
			|| ev->type==GDK_2BUTTON_PRESS) { 
	
  	  treedata = (prj_tree_data*)gtk_ctree_node_get_row_data 
	    (GTK_CTREE (pt), GTK_CTREE_NODE(ptr->rowptr));
  	  if (!treedata) 
   	 	return;
 	  if(treedata->treedataroot)
 	  {
   	 	if(!treedata->is_includefile)
   	 	  fname=g_strdup(treedata->filename);
   	 	else	{
				/* crapcode only testing */
		  	if(treedata->filename[0]=='<') {   /* its a system include file */   	 	
		   	 	int hfile;
   	 		    fname=g_strconcat(INCLUDE_DIR1,"/",&treedata->filename[1],NULL);
		   		fname[strlen(fname)-1]=0;
		    	hfile=open(fname,O_RDONLY);
		    	if(hfile==-1){
		    		g_free(fname);
 					fname=g_strconcat(INCLUDE_DIR2,"/",&treedata->filename[1],NULL);
					fname[strlen(fname)-1]=0;
		      		hfile=open(fname,O_RDONLY);
		    	}
		    	if(hfile!=-1)
		    		  close(hfile);
		    	else
		    	{	g_print("Could not open %s\n",fname);g_free(fname); return;	}
			}
   		 	else {  
   		 		fname=g_strconcat(((prj_tree_data*)treedata->treedataroot)->path,
   	 						&treedata->filename[1],NULL);
   	 				
   	 			fname[strlen(fname)-1]=0;
   	 	  }	
	
   	 	}	
		
   	 	 if(ptr->plugin_context) {
 			gint docid=client_document_open(ptr->plugin_context,fname); 
  			client_document_show(docid);
 		}
 		else
 			g_print("%s\n",fname);
 		
 		g_free(fname);
  	}	
  	else
  		MessageBox("this is the project main","Testcode",MB_OK); 
     } 
}
void prj_callback_fileopen(GtkWidget *o,gpointer ptr)
{	
	gchar *fname;
	prj_data *data=g_malloc0(sizeof(prj_data));
	data->ptr=(prj_manager_window*)ptr;
	data->widget=gtk_file_selection_new("Select directory for the project");
	gtk_signal_connect(GTK_OBJECT(data->widget),"destroy",
				GTK_SIGNAL_FUNC(data_callback_destroy),data);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(data->widget)->cancel_button),
			"clicked",GTK_SIGNAL_FUNC(data_callback_cancel),data);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(data->widget)->ok_button),"clicked",
			GTK_SIGNAL_FUNC(data_callback_ok),data);
	
	gtk_widget_show(data->widget);
		
	modal_loop(data);		/* sleep untill ok callback set to ready */
	if(data->ok)
	{
		struct stat st;
		fname=gtk_file_selection_get_filename(GTK_FILE_SELECTION(data->widget));
		stat((char *)fname,&st);
		if(!S_ISDIR(st.st_mode)){	
				sMessageBox("You must select a directory");  
				modal_loop(data);
		}
		else
			prj_create_treelist(fname,data->ptr);
	}
	
	gtk_widget_destroy(data->widget);	
	g_free(data);
}	
void prj_create_treelist(gchar *fname,prj_manager_window *ptr)
{	
	prj_tree_data *treedataroot,*treedata;
	gchar *buf;
	gint returnflag;
	int hfile;
	GtkWidget *subtree,*subsubtree;
	gchar *cols[2];
	gint len;
	DIR *dir;
	struct dirent *ddata;
	
	buf=NULL;
	cols[1]="";
	if(!fname || !ptr)
		return;
	
	dir=opendir(fname);
	if(!dir)
	{	MessageBox(fname,"Could not open dir",MB_OK);	return ;}	

	
	treedataroot=g_malloc0(sizeof(prj_tree_data));
	treedataroot->path=fname;
	cols[0]=fname;
	ptr->treeroot=gtk_ctree_insert_node (GTK_CTREE(ptr->tree),NULL,NULL,cols,5,NULL,
										NULL,NULL,NULL,FALSE,TRUE);

	gtk_ctree_node_set_row_data_full (GTK_CTREE(ptr->tree), ptr->treeroot,
					   treedataroot,g_free);

	ddata=readdir(dir);
	while(ddata)
	{	
		len=strlen(ddata->d_name);
		len--;
		if(ddata->d_name[len]=='c' )
		{
			treedata=g_malloc(sizeof(prj_tree_data));
			treedata->treedataroot=(gpointer)treedataroot;
			treedata->is_includefile=FALSE;
			treedata->filename=g_strconcat (fname, ddata->d_name, NULL);
			cols[0]=ddata->d_name;
			subtree	=gtk_ctree_insert_node (GTK_CTREE(ptr->tree),ptr->treeroot,NULL,cols,5,NULL,
										NULL,NULL,NULL,FALSE,FALSE);
		    gtk_ctree_node_set_row_data_full (GTK_CTREE(ptr->tree), subtree,
					   treedata,g_free);
					   
			
			/* I know this code can be done much mush much better */
			/* But this is only a test */
			#ifdef DEBUG
				g_print("%s\n",filename);
			#endif
			
			hfile=open(treedata->filename,O_RDONLY);
			if(hfile!=-1 )
			{
				len=lseek(hfile,0,SEEK_END);
				lseek(hfile,0,SEEK_SET);
				if(!buf)
					buf=g_malloc(2048);
				
				if(len>2047)
					len=2046;          
				
				read(hfile,buf,len);
				buf[len+1]=0;       /* make sure its 0 terminated if file is greater that 2047 bytes*/
				
				len=0;
				returnflag=TRUE;  
				while(buf[len])
				{
				  bufloopstart:
					if(buf[len]=='\n')
						returnflag=TRUE; 
					else if(returnflag && buf[len]=='#')	{
						returnflag=FALSE;           /* dont compare again before new return */
						#ifdef DEBUG
							g_print("found # at pos %ld\n",len);
						#endif
						if(strncmp(&buf[len],"#include ",strlen("#include "))==0)	
						{	
							gint old;
							gchar *includename;
							
							len+=strlen("#include ");
							old=len;
							while(buf[len]!='\n' && buf[len]!=0)     
							{	len++;	};
							
							includename=g_malloc((len-old)+1);
							strncpy(includename,&buf[old],len-old);
							includename[len-old]=0;   /* make sure its null terminated */
							cols[0]=includename;
						
							subsubtree=gtk_ctree_insert_node (GTK_CTREE(ptr->tree),subtree,NULL,cols,5,NULL,
										NULL,NULL,NULL,FALSE,FALSE);
							
							treedata=g_malloc(sizeof(prj_tree_data));
							treedata->treedataroot=treedataroot;
							treedata->is_includefile=TRUE;
							treedata->filename=includename;
							
						    gtk_ctree_node_set_row_data_full (GTK_CTREE(ptr->tree), subsubtree,
										   treedata,g_free);
							
							goto bufloopstart;   /* make sure bufmainloop will compare
															 last char again */    
						}
					}
					len++;
				};
			}	
		}
		ddata=readdir(dir);
	};
	closedir(dir);
	if(!buf)
		g_free(buf);
}
