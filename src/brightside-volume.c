/* brightside-volume.c

   Copyright (C) 2002, 2003 Bastien Nocera

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Bastien Nocera <hadess@hadess.net>
 */

#include "config.h"
#include "brightside-volume.h"
#ifdef HAVE_OSS
#include "brightside-volume-oss.h"
#endif
#ifdef HAVE_ALSA
#include "brightside-volume-alsa.h"
#endif

static GObjectClass *parent_class = NULL;

static void
brightside_volume_class_init (BrightsideVolumeClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
};

static void
brightside_volume_init (BrightsideVolume *vol)
{
}

GType brightside_volume_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (BrightsideVolumeClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) brightside_volume_class_init,
			(GClassFinalizeFunc) NULL,
			NULL /* class_data */,
			sizeof (BrightsideVolume),
			0 /* n_preallocs */,
			(GInstanceInitFunc) brightside_volume_init
		};

		type = g_type_register_static (G_TYPE_OBJECT, "BrightsideVolume",
				&info, (GTypeFlags)0);
	}
	return type;
}

/* On success, set class to be a pointer to the right BrightsideVolumeClass for
 * self. On failure, return zero.
 */
#define MAYBE_SETUP_CLASS_RET(self) \
	BrightsideVolumeClass* class = NULL; \
	g_return_val_if_fail (self != NULL, 0); \
	g_return_val_if_fail (BRIGHTSIDE_IS_VOLUME (self), 0); \
	class = BRIGHTSIDE_VOLUME_GET_CLASS (self);

/* On success, set class to be a pointer to the right BrightsideVolumeClass for
 * self. On failure, return (no value!).
 */
#define MAYBE_SETUP_CLASS(self) \
	BrightsideVolumeClass* class = NULL; \
	g_return_if_fail (self != NULL); \
	g_return_if_fail (BRIGHTSIDE_IS_VOLUME (self)); \
	class = BRIGHTSIDE_VOLUME_GET_CLASS (self);

/* A class that doesn't implement a function sets it to NULL. Return 0 if the
 * function wasn't implemented, outputting a warning message too.
 */
#define CHECK_CLASS_FUNC_RET(func_name) \
	if (!class->func_name) { \
	g_warning( #func_name " not implemented by this audio class.\n" ); \
	return 0; \
	}

/* A class that doesn't implement a function sets it to NULL. Return (no value)
 * if the function wasn't implemented, outputting a warning message too.
 */
#define CHECK_CLASS_FUNC(func_name) \
	if (!class->func_name) { \
	g_warning( #func_name " not implemented by this audio class.\n" ); \
	return; }

/* Combine the previous two operations */
#define MAYBE_SETUP_CLASS_FUNC_RET(self, func_name) \
	MAYBE_SETUP_CLASS_RET (self) CHECK_CLASS_FUNC_RET (func_name)
#define MAYBE_SETUP_CLASS_FUNC(self, func_name) \
	MAYBE_SETUP_CLASS (self) CHECK_CLASS_FUNC (func_name)

int
brightside_volume_get_volume (BrightsideVolume *self)
{
	MAYBE_SETUP_CLASS_FUNC_RET (self, get_volume);
	return class->get_volume (self);
}

void
brightside_volume_set_volume (BrightsideVolume *self, int val)
{
	MAYBE_SETUP_CLASS_FUNC (self, set_volume);
	class->set_volume (self, val);
}

gboolean
brightside_volume_get_mute (BrightsideVolume *self)
{
	MAYBE_SETUP_CLASS_FUNC_RET (self, get_mute);
	return class->get_mute (self);
}

void
brightside_volume_set_mute (BrightsideVolume *self, gboolean val)
{
	MAYBE_SETUP_CLASS_FUNC (self, set_mute);
	class->set_mute (self, val);
}

gboolean
brightside_volume_get_use_pcm (BrightsideVolume *self)
{
	MAYBE_SETUP_CLASS_FUNC_RET (self, get_use_pcm);
	return class->get_use_pcm (self);
}

void
brightside_volume_set_use_pcm (BrightsideVolume *self, gboolean val)
{
	MAYBE_SETUP_CLASS_FUNC (self, set_use_pcm);
	class->set_use_pcm (self, val);
}

void
brightside_volume_mute_toggle (BrightsideVolume * self)
{
	gboolean muted;
	MAYBE_SETUP_CLASS_FUNC (self, get_mute);
	CHECK_CLASS_FUNC (set_mute);

	muted = class->get_mute (self);
	class->set_mute (self, !muted);
}

void
brightside_volume_mute_toggle_fade (BrightsideVolume *self, guint duration)
{
	gboolean muted;
	MAYBE_SETUP_CLASS_FUNC (self, get_mute);
	CHECK_CLASS_FUNC (set_mute_fade);

	muted = class->get_mute (self);
	class->set_mute_fade (self, !muted, duration);
}

BrightsideVolume *brightside_volume_new (void)
{
	BrightsideVolume *vol;

#ifdef HAVE_OSS
	vol = BRIGHTSIDE_VOLUME  (g_object_new (brightside_volume_oss_get_type (), NULL));
	return vol;
#endif
#ifdef HAVE_ALSA
	vol = BRIGHTSIDE_VOLUME  (g_object_new (brightside_volume_alsa_get_type (), NULL));
	if (vol != NULL && BRIGHTSIDE_VOLUME_ALSA (vol)->_priv != NULL)
		return vol;
	if (BRIGHTSIDE_VOLUME_ALSA (vol)->_priv == NULL)
		g_object_unref (vol);
#endif
	return BRIGHTSIDE_VOLUME  (g_object_new (brightside_volume_get_type (), NULL));
}
