/* BRIGHTSIDE
 * Copyright (C) 2004 Ed Catmur <ed@catmur.co.uk>
 *
 * custom-action.c
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

#include "custom-action.h"
#include "brightside.h"
#include "brightside-properties-shared.h"

/* Global "dialog" object. There should only be able to be one custom dialog at
 * once, since it's modal, but we set the object to NULL when not in use and
 * check for this at the entry point.
 */
static GladeXML *dialog = NULL;

static GtkWidget*
named_widget (const gchar* name)
{
	g_assert (dialog);
	GtkWidget* widget = glade_xml_get_widget (dialog, name);
	g_assert (widget);
	return widget;
}

/*
 * Set the value of a named GtkEntry when an string gconf entry changes.
 */
void
set_entry_by_string (GConfClient *gconf, guint id, GConfEntry *entry,
					 const gchar* entry_name)
{
	if (entry->value != NULL) {
		gtk_entry_set_text (GTK_ENTRY (named_widget (entry_name)),
							gconf_value_get_string (entry->value));
	}
}

static void
custom_kill_notify (GConfClient *gconf, guint id, GConfEntry *entry,
					gpointer *p)
{
	gboolean kill = gconf_value_get_bool (entry->value);

	gtk_toggle_button_set_active (
		GTK_TOGGLE_BUTTON (named_widget (kill ?
										 "terminate_radio" : "run_radio")),
		TRUE);
	gtk_widget_set_sensitive (named_widget ("out_entry"), !kill);
}

static void
init_gconf_callbacks (const struct corner_desc *corner)
{
	GConfClient *client = gconf_client_get_default ();

	gconf_client_notify_add (client, corner->custom_in_key,
							 (GConfClientNotifyFunc)set_entry_by_string,
							 "in_entry", NULL, NULL);
	gconf_client_notify_add (client, corner->custom_out_key,
							 (GConfClientNotifyFunc)set_entry_by_string,
							 "out_entry", NULL, NULL);
	gconf_client_notify_add (client, corner->custom_kill_key,
							 (GConfClientNotifyFunc)custom_kill_notify,
							 NULL, NULL, NULL);

	g_object_unref (client);
}

void
on_editable_with_key (GtkEditable *editable, const gchar *key)
{
	GConfClient *client = gconf_client_get_default ();
	const gchar *txt = gtk_entry_get_text (GTK_ENTRY (editable));

	gconf_client_set_string (client, key, txt, NULL);

	g_object_unref (client);
}

static void
on_kill_toggle (GtkToggleButton *togglebutton, const struct corner_desc *corner)
{
	GConfClient *client = gconf_client_get_default ();

	gconf_client_set_bool (client, corner->custom_kill_key,
						   gtk_toggle_button_get_active (togglebutton),
						   NULL);

	g_object_unref (client);
}

static void
init_ui_callbacks (const struct corner_desc *corner)
{
	g_signal_connect (G_OBJECT (named_widget ("in_entry")),
					  "changed", (GCallback)on_editable_with_key,
					  (gpointer)corner->custom_in_key);
	g_signal_connect (G_OBJECT (named_widget ("terminate_radio")),
					  "toggled", (GCallback)on_kill_toggle, (gpointer)corner);
	g_signal_connect (G_OBJECT (named_widget ("out_entry")),
					  "changed", (GCallback)on_editable_with_key,
					  (gpointer)corner->custom_out_key);
}

static void
on_response (GtkDialog *dialogwin, gint response_id, gpointer *ignored)
{
	if (response_id == GTK_RESPONSE_HELP)
		gnome_url_show (HELP_URL, NULL);
	else {
		gtk_widget_destroy (GTK_WIDGET (dialogwin));
		dialog = NULL;
	}
}

void
show_custom_action_dialog (GtkWindow *parent,
						   const struct corner_desc *corner)
{
	GtkWidget *dialog_win;
	GConfClient *client = gconf_client_get_default ();

	if (dialog) {
		g_warning ("Trying to run two custom dialogs at once.\n");
		return;
	}

	dialog = glade_xml_new (BRIGHTSIDE_DATA "custom-action.glade", 
							"action_widget", NULL);

	init_gconf_callbacks (corner);

	gconf_client_notify (client, corner->custom_in_key);
	gconf_client_notify (client, corner->custom_out_key);
	gconf_client_notify (client, corner->custom_kill_key);

	init_ui_callbacks (corner);

	dialog_win = gtk_dialog_new_with_buttons(
		_("Custom action"), parent, 
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT
		| GTK_DIALOG_NO_SEPARATOR,
		GTK_STOCK_HELP, GTK_RESPONSE_HELP, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		NULL);

	gtk_container_set_border_width (GTK_CONTAINER (dialog_win), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog_win)->vbox), 2);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog_win),
									 GTK_RESPONSE_CLOSE);

	g_signal_connect (G_OBJECT (dialog_win), "response", 
					  (GCallback) on_response, NULL);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_win)->vbox), 
						named_widget ("action_widget"),
						TRUE, TRUE, 0);
	gtk_window_set_resizable (GTK_WINDOW (dialog_win), FALSE);
	gtk_window_set_icon (GTK_WINDOW (dialog_win), 
						 gtk_widget_render_icon (dialog_win, 
												 GTK_STOCK_EXECUTE,
												 GTK_ICON_SIZE_DIALOG, NULL));
	gtk_widget_show_all (dialog_win);

	g_object_unref (client);
}

