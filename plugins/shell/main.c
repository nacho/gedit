#include <config.h>
#include <gnome.h>

#include <gedit/gE_plugin.h>

#include <pwd.h>
#include <zvt/zvtterm.h>

static void init_plugin (gE_Plugin_Object *, gint);

gint edit_context = 0;

gE_Plugin_Info gedit_plugin_info = {
	"Shell Plugin",
	init_plugin,
};

#define DEFAULT_FONT "-misc-fixed-medium-r-normal--20-200-75-75-c-100-iso8859-1"

void
init_plugin (gE_Plugin_Object *plugin, gint context)
{
	struct passwd *pw;
	GtkWidget *container, *zterm;
	gint docid;

	container = gE_plugin_create_widget (context, _("Shell"), NULL);

	fprintf (stderr,
		 "Greetings from the plugin with context %d - widget %p.\n",
		 context, container);

	zterm = zvt_term_new ();
	gtk_widget_ensure_style (zterm);

	zvt_term_set_scrollback (ZVT_TERM (zterm), 5000);
	zvt_term_set_font_name (ZVT_TERM (zterm), DEFAULT_FONT);

	switch (zvt_term_forkpty (ZVT_TERM (zterm))) {
	case -1:
		perror ("ERROR: unable to fork:");
		exit (1);
		break;
	case 0:
		pw = getpwuid (getpid());
		if (pw) {
			execl (pw->pw_shell, rindex (pw->pw_shell, '/'), 0);
		} else {
			execl ("/bin/bash", "bash", 0);
		}
		perror ("ERROR: Cannot exec command:");
		exit (1);
	default:
	}

	gtk_container_add (GTK_CONTAINER (container), zterm);
	gtk_widget_show_all (container);
}
