/*
Copyright 2011 Canonical Ltd.

Authors:
    Conor Curran <conor.curran@canonical.com>

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License version 3, as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranties of
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _SOUND_STATE_MANAGER_H_
#define _SOUND_STATE_MANAGER_H_

#include <glib.h>
#include "common-defs.h"

G_BEGIN_DECLS

#define SOUND_TYPE_STATE_MANAGER             (sound_state_manager_get_type ())
#define SOUND_STATE_MANAGER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SOUND_TYPE_STATE_MANAGER, SoundStateManager))
#define SOUND_STATE_MANAGER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SOUND_TYPE_STATE_MANAGER, SoundStateManagerClass))
#define SOUND_IS_STATE_MANAGER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SOUND_TYPE_STATE_MANAGER))
#define SOUND_IS_STATE_MANAGER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SOUND_TYPE_STATE_MANAGER))
#define SOUND_STATE_MANAGER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SOUND_TYPE_STATE_MANAGER, SoundStateManagerClass))

typedef struct _SoundStateManagerClass SoundStateManagerClass;
typedef struct _SoundStateManager SoundStateManager;

struct _SoundStateManagerClass
{
	GObjectClass parent_class;
};

struct _SoundStateManager
{
	GObject parent_instance;
};

GType sound_state_manager_get_type (void) G_GNUC_CONST;

void sound_state_manager_style_changed_cb (GtkWidget *widget,
                                           GtkStyle  *previous_style,
                                           gpointer user_data);
GtkImage* sound_state_manager_get_current_icon (SoundStateManager* self);
SoundState sound_state_manager_get_current_state (SoundStateManager* self);
void sound_state_manager_connect_to_dbus (SoundStateManager* self,
                                          GDBusProxy* proxy);
void sound_state_manager_deal_with_disconnect (SoundStateManager* self);
void sound_state_manager_get_state_cb (GObject *object,
                                       GAsyncResult *res,
                                       gpointer user_data);
void sound_state_manager_show_notification (SoundStateManager *self,
                                            double value);


G_END_DECLS

#endif /* _SOUND_STATE_MANAGER_H_ */
