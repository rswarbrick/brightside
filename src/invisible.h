/*
 * Copyright (C) 2009 Rupert Swarbrick <rswarbrick@gmail.com>
 *
 * invisible.h
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

#ifndef __INVISIBLE_H__
#define __INVISIBLE_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <libwnck/libwnck.h>

typedef struct
{
	GtkWidget *left, *top, *right, *bottom;
} InvisibleBorders;

/*
  Add four invisible borders to gscreen, which should be the GdkScreen for
  wscreen.
 */
void screen_add_invisible_borders (WnckScreen* wscreen, GdkScreen* gscreen,
								   InvisibleBorders* b);

/*
  Destroys the windows created and referenced in b and sets the structure
  members to NULL.
 */
void destroy_invisible_borders (InvisibleBorders* b);


#endif /* __INVISIBLE_H__ */

