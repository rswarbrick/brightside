/*
 * Copyright (C) 2009 Rupert Swarbrick <rswarbrick@gmail.com>
 *
 * invisible.c
 *
 * This file is part of Brightside.
 *
 * Brightside is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Brightside is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Brightside; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 * USA.
 */

#include <config.h>
#include "invisible.h"
#include "brightside.h"

static gboolean
on_edge_enter (GtkWidget *widget, GdkEventCrossing *event,
			   BrightsideRegionType rt)
{
	/* invisible.c has dealt with the question of whether there was an
	   event. Now punt to brightside.c to deal with the event itself. */
	on_triggered_region (rt);

	return TRUE;
}

static gboolean
on_corner_leave (GtkWidget *widget, GdkEventCrossing *event,
				 BrightsideRegionType rt)
{
	/* Our entering the corner earlier might have started a timed action. Check
	 * whether that is the case and, if so, cancel it.
	 */
	do_region_action (rt, ACTION_STOP);
}


#define MAYBE_RAISE_WINDOW(w) \
	if(w) { gdk_window_raise(gtk_widget_get_window(w)); }

static void
raise_windows(WnckScreen *screen, InvisibleBorders *b)
{
	MAYBE_RAISE_WINDOW (b->left);
	MAYBE_RAISE_WINDOW (b->right);
	MAYBE_RAISE_WINDOW (b->top);
	MAYBE_RAISE_WINDOW (b->bottom);
	MAYBE_RAISE_WINDOW (b->tl);
	MAYBE_RAISE_WINDOW (b->tr);
	MAYBE_RAISE_WINDOW (b->bl);
	MAYBE_RAISE_WINDOW (b->br);
}

static GtkWidget*
make_invisible_window(GdkScreen* screen, BrightsideRegionType rt,
					  int left, int top, int width, int height)
{
	GtkWidget* invis = gtk_invisible_new_for_screen(screen);
	g_assert (invis);

	gtk_widget_add_events (invis, GDK_ENTER_NOTIFY_MASK);

	/* Slight hack, passing rt as a gpointer, but both are int's really! */
	g_signal_connect (G_OBJECT(invis), "enter-notify-event",
					  G_CALLBACK(on_edge_enter), (gpointer)rt);

	if (REGION_IS_CORNER (rt)) {
		gtk_widget_add_events (invis, GDK_LEAVE_NOTIFY_MASK);
		g_signal_connect (G_OBJECT(invis), "leave-notify-event",
						  G_CALLBACK(on_corner_leave), (gpointer)rt);
	}

	gdk_window_move_resize (gtk_widget_get_window(invis),
							left, top, width, height);

	gtk_widget_show (invis);

	return invis;
}

void
screen_add_invisible_borders (WnckScreen* wscreen, GdkScreen* gscreen,
							  InvisibleBorders* b)
{
	g_assert(wscreen);
	g_assert(gscreen);
	g_assert(b);

	int width = gdk_screen_get_width (gscreen),
		height = gdk_screen_get_height (gscreen);

	destroy_invisible_borders (b);

	b->left   = make_invisible_window (gscreen, LEFT,
									   0, 1, 1, height-1);

	b->top    = make_invisible_window (gscreen, TOP,
									   1, 0, width-1, 1);

	b->right  = make_invisible_window (gscreen, RIGHT,
									   width-1, 1, width, height-1);

	b->bottom = make_invisible_window (gscreen, BOTTOM,
									   1, height-1, width-1, height);

	b->tl = make_invisible_window (gscreen, NW, 0, 0, 1, 1);
	b->tr = make_invisible_window (gscreen, NE, width-1, 0, width, 1);
	b->bl = make_invisible_window (gscreen, SW, 0, height-1, 1, height);
	b->br = make_invisible_window (gscreen, SE,
								   width-1, height-1, width, height);

	g_signal_connect (G_OBJECT(wscreen), "window-stacking-changed",
					  G_CALLBACK(raise_windows), b);
}

#define MAYBE_DESTROY_WIDGET(a) if(a) { gtk_widget_destroy(a); a = NULL; }

void
destroy_invisible_borders(InvisibleBorders* b)
{
	MAYBE_DESTROY_WIDGET (b->left);
	MAYBE_DESTROY_WIDGET (b->top);
	MAYBE_DESTROY_WIDGET (b->right);
	MAYBE_DESTROY_WIDGET (b->bottom);
	MAYBE_DESTROY_WIDGET (b->tl);
	MAYBE_DESTROY_WIDGET (b->tr);
	MAYBE_DESTROY_WIDGET (b->bl);
	MAYBE_DESTROY_WIDGET (b->br);
}
