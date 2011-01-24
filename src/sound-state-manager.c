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

#include "sound-state-manager.h"

typedef struct _SoundStateManagerPrivate SoundStateManagerPrivate;

struct _SoundStateManagerPrivate
{
  GDBusProxy* dbus_proxy;  
  GHashTable* volume_states;  
  GList* blocked_animation_list;
};

#define SOUND_STATE_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SOUND_TYPE_STATE_MANAGER, SoundStateManagerPrivate))

static GtkImage *speaker_image = NULL;
static gint current_state = 0;
static gint previous_state = 0;
static GtkIconSize design_team_size;
static gint blocked_id;
static gint animation_id;

static GList * blocked_animation_list = NULL;
static GList * blocked_iter = NULL;
static void prepare_blocked_animation();
static gboolean fade_back_to_mute_image();
static gboolean start_animation();
static void reset_mute_blocking_animation();
static void free_the_animation_list();

G_DEFINE_TYPE (SoundStateManager, sound_state_manager, G_TYPE_OBJECT);

static void
sound_state_manager_init (SoundStateManager *object)
{
  /* TODO: Add initialization code here */
}

static void
sound_state_manager_finalize (GObject *object)
{
	/* TODO: Add deinitalization code here */

	G_OBJECT_CLASS (sound_state_manager_parent_class)->finalize (object);
}

static void
sound_state_manager_class_init (SoundStateManagerClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GObjectClass* parent_class = G_OBJECT_CLASS (klass);

	object_class->finalize = sound_state_manager_finalize;
}

/*
Prepare states Array.
*/
void
sound_state_manager_prepare_state_machine(SoundStateManager* self)
{
  SoundStateManagerPrivate* priv = SOUND_STATE_MANAGER_GET_PRIVATE(self);
  priv->volume_states = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free); 
  g_hash_table_insert(priv->volume_states, GINT_TO_POINTER(MUTED), g_strdup("audio-volume-muted-panel"));
  g_hash_table_insert(priv->volume_states, GINT_TO_POINTER(ZERO_LEVEL), g_strdup("audio-volume-low-zero-panel"));
  g_hash_table_insert(priv->volume_states, GINT_TO_POINTER(LOW_LEVEL), g_strdup("audio-volume-low-panel"));
  g_hash_table_insert(priv->volume_states, GINT_TO_POINTER(MEDIUM_LEVEL), g_strdup("audio-volume-medium-panel"));
  g_hash_table_insert(priv->volume_states, GINT_TO_POINTER(HIGH_LEVEL), g_strdup("audio-volume-high-panel"));
  g_hash_table_insert(priv->volume_states, GINT_TO_POINTER(BLOCKED), g_strdup("audio-volume-muted-blocking-panel"));
  g_hash_table_insert(priv->volume_states, GINT_TO_POINTER(UNAVAILABLE), g_strdup("audio-output-none-panel"));
}

/*
prepare_blocked_animation:
Prepares the array of images to be used in the blocked animation.
Only called at startup.
*/
static void
sound_state_manager_prepare_blocked_animation(SoundStateManager* self)
{
  SoundStateManagerPrivate* priv = SOUND_STATE_MANAGER_GET_PRIVATE(self);
  
  gchar* blocked_name = g_hash_table_lookup(priv->volume_states,
                                            GINT_TO_POINTER(BLOCKED));
  gchar* muted_name = g_hash_table_lookup(priv->volume_states,
                                          GINT_TO_POINTER(MUTED));

  GtkImage* temp_image = indicator_image_helper(muted_name);
  GdkPixbuf* mute_buf = gtk_image_get_pixbuf(temp_image);

  temp_image = indicator_image_helper(blocked_name);
  GdkPixbuf* blocked_buf = gtk_image_get_pixbuf(temp_image);

  if (mute_buf == NULL || blocked_buf == NULL) {
    //g_debug("Don bother with the animation, the theme aint got the goods !");
    return;
  }

  int i;

  // sample 51 snapshots - range : 0-256
  for (i = 0; i < 51; i++) {
    gdk_pixbuf_composite(mute_buf, blocked_buf, 0, 0,
                         gdk_pixbuf_get_width(mute_buf),
                         gdk_pixbuf_get_height(mute_buf),
                         0, 0, 1, 1, GDK_INTERP_BILINEAR, MIN(255, i * 5));
    priv->blocked_animation_list = g_list_append(priv->blocked_animation_list,
                                                 gdk_pixbuf_copy(blocked_buf));
  }
  g_object_ref_sink(mute_buf);
  g_object_unref(mute_buf);
  g_object_ref_sink(blocked_buf);
  g_object_unref(blocked_buf);
}

void
sound_state_manager_tidy_up_hash()
{
  g_hash_table_destroy(volume_states);
}

gchar*
sound_state_manager_get_current_icon_name (SoundStateManager* self)
{
  SoundStateManagerPrivate* priv = SOUND_STATE_MANAGER_GET_PRIVATE(self);
  return g_hash_table_lookup (priv->volume_states,
                              GINT_TO_POINTER(priv->current_state));
  
}

static void g_signal_cb ( GDBusProxy* proxy,
                          gchar* sender_name,
                          gchar* signal_name,
                          GVariant* parameters,
                          gpointer user_data)
{
}

static void
sound_state_manager_style_changed_cb(GtkWidget *widget, gpointer user_data)
{
  //g_debug("Just caught a style change event");
  update_state(current_state);
  reset_mute_blocking_animation();
  update_state(current_state);
  free_the_animation_list();
  prepare_blocked_animation();
}

/**
 * volume_widget_new:
 * @returns: a new #VolumeWidget.
 **/
SoundStateManager* 
sound_state_manager_new (GDProxy* proxy)
{
  SoundStateManager* manager = g_object_new (SOUND_TYPE_STATE_MANAGER, NULL);
  SoundStateManagerPrivate* priv = SOUND_STATE_MANAGER_GET_PRIVATE(manager);
  priv->dbus_proxy = proxy;
  return manager;
}
