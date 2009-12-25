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

GladeXML *dialog = NULL;

static GtkWidget*
named_widget (const gchar* name)
{
	g_assert (dialog);
	GtkWidget* widget = glade_xml_get_widget (dialog, name);
	g_assert (widget);
	return widget;
}

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

/***** Callbacks that respond to external stimuli and update the dialog *******/

static void
on_set_corner_enabled (const struct corner_desc* corner, gboolean enabled)
{
	GConfClient *client = gconf_client_get_default ();

	gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (named_widget (corner->enabled_toggle_id)),
		enabled);
	gtk_widget_set_sensitive (
		named_widget (corner->action_menu_id),
		enabled && !gconf_client_get_bool (
			client, "/apps/brightside/corner_flip", NULL));

	g_object_unref (client);
}

static void
on_set_corner_action (const struct corner_desc* corner, const gchar* action)
{
	gint enum_val;
	GConfClient *client = gconf_client_get_default ();

	if (!gconf_string_to_enum (actions_lookup_table, action, &enum_val)) {
		g_warning ("Invalid corner action stored. Setting to mute.\n");
		gconf_client_set_string (client,
								 corner->action_key,
								 gconf_enum_to_string (actions_lookup_table,
													   MUTE_VOLUME_ACTION),
								 NULL);
		enum_val = MUTE_VOLUME_ACTION;
	}

	gtk_option_menu_set_history (
		GTK_OPTION_MENU (named_widget (corner->action_menu_id)),
		enum_val);

	g_object_unref (client);
}

static void
on_set_corner_flip (gboolean enabled)
{
	GConfClient *client = gconf_client_get_default ();
	int i;

	gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (named_widget (enabled ?
										 "corners_flip" :
										 "corners_configurable")),
		TRUE);

	for (i = 0; i < 4; i++) {
		gtk_widget_set_sensitive (
			named_widget (corners[i].action_menu_id),
			!enabled &&
			gconf_client_get_bool (client, corners[i].enabled_key, NULL));
		gtk_widget_set_sensitive (
			named_widget (corners[i].enabled_toggle_id), !enabled);
	}


	g_object_unref (client);
}

static void
on_set_corner_delay (gint delay)
{
	gtk_range_set_value (GTK_RANGE (named_widget ("corner_delay_scale")),
						 delay);
}

static void
on_set_edge_flip (gboolean flip)
{
	gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (named_widget ("edge_flip_enabled")),
		flip);
	gtk_widget_set_sensitive (named_widget ("edge_delay_scale"), flip);
}

static void
on_set_edge_delay (gint delay)
{
	gtk_range_set_value (GTK_RANGE (named_widget ("edge_delay_scale")),
						 delay);
}

static void
on_set_edge_wrap (gboolean wrap)
{
	gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (named_widget ("edge_wrap_enabled")),
		wrap);
}

/**************** Notification functions called from gconf ********************/
static void
corner_enabled_notify (GConfClient *gconf, guint id, GConfEntry *entry,
					   const struct corner_desc* corner)
{
	on_set_corner_enabled (corner, gconf_value_get_bool (entry->value));
}

static void
corner_action_notify (GConfClient *gconf, guint id, GConfEntry *entry,
					  const struct corner_desc* corner)
{
	on_set_corner_action (corner, gconf_value_get_string (entry->value));
}

static void
corner_flip_notify (GConfClient *gconf, guint id, GConfEntry *entry,
					void* ignored)
{
	on_set_corner_flip (gconf_value_get_bool (entry->value));
}

static void
corner_delay_notify (GConfClient *gconf, guint id, GConfEntry *entry,
					void* ignored)
{
	on_set_corner_delay (gconf_value_get_int (entry->value));
}

static void
edge_flip_notify (GConfClient *gconf, guint id, GConfEntry *entry,
				  void* ignored)
{
	on_set_edge_flip (gconf_value_get_bool (entry->value));
}

static void
edge_delay_notify (GConfClient *gconf, guint id, GConfEntry *entry,
				   void* ignored)
{
	on_set_edge_delay (gconf_value_get_int (entry->value));
}

static void
edge_wrap_notify (GConfClient *gconf, guint id, GConfEntry *entry,
				  void* ignored)
{
	on_set_edge_wrap (gconf_value_get_bool (entry->value));
}

/* Set up callbacks which update the dialog when gconf key values change. */
static void
init_gconf_callbacks ()
{
	GConfClient *client = gconf_client_get_default ();
	gint i;

	gconf_client_add_dir (client, "/apps/brightside",
						  GCONF_CLIENT_PRELOAD_ONELEVEL,
						  NULL);
	for (i = 0; i < 4; i++) {
		gconf_client_notify_add (client, corners[i].enabled_key,
								 (GConfClientNotifyFunc)corner_enabled_notify,
								 (void*)&corners[i], NULL, NULL);
		gconf_client_notify_add (client, corners[i].action_key,
								 (GConfClientNotifyFunc)corner_action_notify,
								 (void*)&corners[i], NULL, NULL);
	}
	gconf_client_notify_add (client, "/apps/brightside/corner_flip",
							 (GConfClientNotifyFunc)corner_flip_notify,
							 NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/brightside/corner_delay",
							 (GConfClientNotifyFunc)corner_delay_notify,
							 NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/brightside/enable_edge_flip",
							 (GConfClientNotifyFunc)edge_flip_notify,
							 NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/brightside/edge_delay",
							 (GConfClientNotifyFunc)edge_delay_notify,
							 NULL, NULL, NULL);
	gconf_client_notify_add (client, "/apps/brightside/edge_wrap",
							 (GConfClientNotifyFunc)edge_wrap_notify,
							 NULL, NULL, NULL);

	g_object_unref (client);
}

static void
on_corner_enabled_toggle (GtkToggleButton *togglebutton,
						  const struct corner_desc *corner)
{
	GConfClient *client = gconf_client_get_default ();

	gconf_client_set_bool (client, corner->enabled_key,
						   gtk_toggle_button_get_active (togglebutton),
						   NULL);

	g_object_unref (client);
}

static void
on_corner_action_changed (GtkOptionMenu *menu,
						  const struct corner_desc *corner)
{
	GConfClient *client = gconf_client_get_default ();
	gint menu_index = gtk_option_menu_get_history (menu);
	const gchar* action = gconf_enum_to_string (actions_lookup_table,
												menu_index);

	if (!action) {
		g_warning ("Got an invalid action. Switching to default.\n");
		action = gconf_enum_to_string (actions_lookup_table,
									   MUTE_VOLUME_ACTION);
		menu_index = MUTE_VOLUME_ACTION;
	}
	else if (menu_index == CUSTOM_ACTION) {
		show_custom_action_dialog (
			GTK_WINDOW (gtk_widget_get_ancestor (
							GTK_WIDGET (menu), GTK_TYPE_DIALOG)),
			GPOINTER_TO_INT (corner));
	}

	gconf_client_set_string (client, corner->action_key, action, NULL);

	g_object_unref (client);
}

static void
on_corners_configurable_toggle (GtkToggleButton *togglebutton,
								gpointer *ignored)
{
	GConfClient *client = gconf_client_get_default ();

	gconf_client_set_bool (client, "/apps/brightside/corner_flip",
						   !gtk_toggle_button_get_active (togglebutton),
						   NULL);

	g_object_unref (client);
}

static void
on_corner_delay_changed (GtkRange *range, gpointer *ignored)
{
	GConfClient *client = gconf_client_get_default ();

	gconf_client_set_int (client, "/apps/brightside/corner_delay",
						  (gint)gtk_range_get_value (range),
						  NULL);

	g_object_unref (client);
}

static void
on_edge_flip_toggle (GtkToggleButton *togglebutton, gpointer *ignored)
{
	GConfClient *client = gconf_client_get_default ();

	gconf_client_set_bool (client, "/apps/brightside/enable_edge_flip",
						  gtk_toggle_button_get_active (togglebutton),
						  NULL);

	g_object_unref (client);
}

static void
on_edge_delay_changed (GtkRange *range, gpointer *ignored)
{
	GConfClient *client = gconf_client_get_default ();

	gconf_client_set_int (client, "/apps/brightside/edge_delay",
						  (gint)gtk_range_get_value (range),
						  NULL);

	g_object_unref (client);
}

static void
on_edge_wrap_toggle (GtkToggleButton *togglebutton, gpointer *ignored)
{
	GConfClient *client = gconf_client_get_default ();

	gconf_client_set_bool (client, "/apps/brightside/edge_wrap",
						  gtk_toggle_button_get_active (togglebutton),
						  NULL);

	g_object_unref (client);
}

/* Set up callbacks to update gconf when dialog properties change */
static void
init_ui_callbacks ()
{
	int i;

	for (i = 0; i < 4; i++) {
		g_signal_connect (
			G_OBJECT (named_widget (corners[i].enabled_toggle_id)),
			"toggled", (GCallback)on_corner_enabled_toggle,
			(gpointer)&corners[i]);
		g_signal_connect (
			G_OBJECT (named_widget (corners[i].action_menu_id)),
			"changed", (GCallback)on_corner_action_changed,
			(gpointer)&corners[i]);
	}

	g_signal_connect (G_OBJECT (named_widget ("corners_configurable")),
					  "toggled",
					  (GCallback)on_corners_configurable_toggle, NULL);
	g_signal_connect (G_OBJECT (named_widget ("corner_delay_scale")),
					  "value-changed",
					  (GCallback)on_corner_delay_changed, NULL);
	g_signal_connect (G_OBJECT (named_widget ("edge_flip_enabled")),
					  "toggled",
					  (GCallback)on_edge_flip_toggle, NULL);
	g_signal_connect (G_OBJECT (named_widget ("edge_delay_scale")),
					  "value-changed",
					  (GCallback)on_edge_delay_changed, NULL);
	g_signal_connect (G_OBJECT (named_widget ("edge_wrap_enabled")),
					  "toggled",
					  (GCallback)on_edge_wrap_toggle, NULL);
}

void
gconf_client_notify_dir (GConfClient *client, const char *dir)
{
	GSList *list = gconf_client_all_entries (client, dir, NULL);
	GSList *elt;
	GConfEntry *entry;

	for (elt = list; elt; elt = g_slist_next(elt)) {
		entry = (GConfEntry*)elt->data;
		gconf_client_notify (client, entry->key);
		gconf_entry_free (entry);
	}
	g_slist_free (list);
}

int
main (int argc, char *argv[])
{
	GtkWidget	*dialog_win;
	GConfClient *client;
	
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

	g_signal_connect (G_OBJECT (dialog_win), "response",
					  (GCallback) dialog_button_clicked_cb, NULL);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_win)->vbox), 
			WID ("prefs_widget"), TRUE, TRUE, 0);
	gtk_window_set_resizable (GTK_WINDOW (dialog_win), FALSE);
	gtk_window_set_icon_from_file (GTK_WINDOW (dialog_win), 
			BRIGHTSIDE_DATA "brightside.svg", NULL);
	gtk_widget_show_all (dialog_win);

	init_gconf_callbacks ();

	client = gconf_client_get_default ();
	gconf_client_notify_dir (client, "/apps/brightside");
	g_object_unref (client);

	init_ui_callbacks ();

	if (is_running () == FALSE)
		g_spawn_command_line_async ("brightside", NULL);
	
	gtk_main ();

	g_object_unref (dialog);

	return 0;
}

