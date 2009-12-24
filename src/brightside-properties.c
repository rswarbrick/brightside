/* BRIGHTSIDE
 * Copyright (C) 2004 Ed Catmur <ed@catmur.co.uk>
 *
 * brightside-properties.c
 *
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 * USA.
 */

#include <config.h>

#include <sys/file.h>
#include <gnome.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>
#include <gdk/gdkx.h>

#include "capplet-util.h"

#include "brightside-properties-shared.h"
#include "brightside-properties.h"
#include "brightside.h"

GConfClient *conf_client;

static gboolean
is_running (void)
{
	gboolean result = FALSE;
	Atom clipboard_atom = gdk_x11_get_xatom_by_name (SELECTION_NAME);

	XGrabServer (GDK_DISPLAY());

	if (XGetSelectionOwner (GDK_DISPLAY(), clipboard_atom) != None)
		result = TRUE;

	XUngrabServer (GDK_DISPLAY());
	gdk_flush();

	return result;
}

static void
update_widgets_sensitive_cb (gpointer *data, GladeXML *dialog)
{
	gboolean corner_delay_state;
	gboolean corner_actions_state;
	gint corner;
	if (gtk_toggle_button_get_active (
				GTK_TOGGLE_BUTTON (WID ("corners_flip")))) {
		corner_delay_state = TRUE;
		corner_actions_state = FALSE;
	} else {
		corner_delay_state = FALSE;
		corner_actions_state = TRUE;
		for (corner = REGION_FIRST_CORNER; REGION_IS_CORNER (corner); 
				++corner)
			corner_delay_state |= gtk_toggle_button_get_active
				(GTK_TOGGLE_BUTTON
				 (WID (corners[corner].enabled_toggle_id)));
	}
	for (corner = REGION_FIRST_CORNER; REGION_IS_CORNER (corner); 
			++corner) {
		gtk_widget_set_sensitive (
				WID (corners[corner].enabled_toggle_id),
				corner_actions_state);
		gtk_widget_set_sensitive (
				WID (corners[corner].action_menu_id),
				corner_actions_state
				&& gtk_toggle_button_get_active
				(GTK_TOGGLE_BUTTON 
				 (WID (corners[corner].enabled_toggle_id))));
	}
	gtk_widget_set_sensitive 
		(WID ("corner_delay_scale"), corner_delay_state);
	gtk_widget_set_sensitive 
		(WID ("edge_wrap_enabled"), 
		 gtk_toggle_button_get_active (
			 GTK_TOGGLE_BUTTON (WID ("corners_flip"))) ||
		 gtk_toggle_button_get_active (
			 GTK_TOGGLE_BUTTON (WID ("edge_flip_enabled"))));
}

static void
dialog_button_clicked_cb (GtkDialog *dialog, gint response_id, 
		GConfChangeSet *changeset)
{
	switch (response_id) {
		case GTK_RESPONSE_NONE:
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_OK:
		case GTK_RESPONSE_CANCEL:
		case GTK_RESPONSE_CLOSE:
			gtk_main_quit ();
			break;
		case GTK_RESPONSE_HELP:
			gnome_url_show (HELP_URL, NULL);
			break;
		default:
			g_assert_not_reached();
	}
}

static void
populate_menus (GladeXML *dialog)
{
	gint corner, action;
	GtkWidget *menu;
	GtkWidget *menuitem;

	for (corner = REGION_FIRST_CORNER; REGION_IS_CORNER (corner); 
			++corner) {
		menu = gtk_menu_new();
		for (action = 0; action < HANDLED_ACTIONS; ++action) {
			menuitem = gtk_menu_item_new_with_label (
					action_descriptions[action]);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		}
		gtk_option_menu_set_menu (GTK_OPTION_MENU (
					WID (corners[corner].action_menu_id)),
				menu);
	}
}

/* CargoCult gnome-control-center/.../sound-properties-capplet.c */
static GladeXML *
create_dialog (void)
{
	GladeXML *dialog;

	dialog = glade_xml_new (BRIGHTSIDE_DATA "brightside-properties.glade", 
			"prefs_widget", NULL);
	g_object_set_data (G_OBJECT (WID ("prefs_widget")), 
			"glade-data", dialog);

	populate_menus (dialog);

	return dialog;
}

int
main (int argc, char *argv[])
{
	GladeXML	*dialog = NULL;
	GtkWidget	*dialog_win;
	
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gnome_program_init ("brightside-properties", VERSION,
			LIBGNOMEUI_MODULE, argc, argv,
			NULL);

	dialog = create_dialog ();

	dialog_win = gtk_dialog_new_with_buttons(
			_("Screen Actions"), NULL, 
			GTK_DIALOG_NO_SEPARATOR,
			GTK_STOCK_HELP, GTK_RESPONSE_HELP,
			GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
			NULL);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_win), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG(dialog_win)->vbox), 2);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog_win), 
			GTK_RESPONSE_CLOSE);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_win)->vbox), 
			WID ("prefs_widget"), TRUE, TRUE, 0);
	gtk_window_set_resizable (GTK_WINDOW (dialog_win), FALSE);
	gtk_window_set_icon_from_file (GTK_WINDOW (dialog_win), 
			BRIGHTSIDE_DATA "brightside.svg", NULL);
	gtk_widget_show_all (dialog_win);
	
	if (is_running () == FALSE)
		g_spawn_command_line_async ("brightside", NULL);
	
	gtk_main ();

	g_object_unref (dialog);

	return 0;
}

