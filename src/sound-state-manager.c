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

#include <libindicator/indicator-image-helper.h>
#include "sound-state-manager.h"

typedef struct _SoundStateManagerPrivate SoundStateManagerPrivate;

// TODO ensure all the relevant below are initialized to null in init
struct _SoundStateManagerPrivate
{
  GDBusProxy* dbus_proxy;  
  GHashTable* volume_states;  
  GList* blocked_animation_list;
  SoundState current_state;
  GtkImage* speaker_image;
};

#define SOUND_STATE_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), SOUND_TYPE_STATE_MANAGER, SoundStateManagerPrivate))
G_DEFINE_TYPE (SoundStateManager, sound_state_manager, G_TYPE_OBJECT);

static GtkIconSize design_team_size;
static gint blocked_id;
static gint animation_id;
static GList* blocked_iter = NULL;

static void sound_state_manager_prepare_blocked_animation(SoundStateManager* self);
static gboolean sound_state_manager_start_animation (SoundStateManager* self);
static gboolean sound_state_manager_fade_back_to_mute_image (gpointer user_data);
static void sound_state_manager_reset_mute_blocking_animation (SoundStateManager* self);
static void sound_state_manager_free_the_animation_list (SoundStateManager* self);
static void sound_state_manager_prepare_state_image_names (SoundStateManager* self);


static void
sound_state_manager_init (SoundStateManager* self)
{
  SoundStateManagerPrivate* priv = SOUND_STATE_MANAGER_GET_PRIVATE(self);

  sound_state_manager_prepare_state_image_names (self);
  sound_state_manager_prepare_blocked_animation (self);

  priv->current_state = UNAVAILABLE;
  priv->speaker_image = indicator_image_helper (g_hash_table_lookup (priv->volume_states,
                                                                     GINT_TO_POINTER(priv->current_state)));
}

static void
sound_state_manager_finalize (GObject *object)
{
	/* TODO: Add deinitalization code here */

	G_OBJECT_CLASS (sound_state_manager_parent_class)->finalize (object);
}

static void
sound_state_manager_dispose (GObject *object)
{
  SoundStateManager* self = SOUND_STATE_MANAGER (object);
  SoundStateManagerPrivate* priv = SOUND_STATE_MANAGER_GET_PRIVATE(self);
  
  g_hash_table_destroy (priv->volume_states);

  sound_state_manager_free_the_animation_list (self);
  G_OBJECT_CLASS (sound_state_manager_parent_class)->dispose (object);
}
 

static void
sound_state_manager_class_init (SoundStateManagerClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	//GObjectClass* parent_class = G_OBJECT_CLASS (klass);

	object_class->finalize = sound_state_manager_finalize;
  object_class->dispose = sound_state_manager_dispose;
  design_team_size = gtk_icon_size_register("design-team-size", 22, 22);  
}

/*
Prepare states versus images names hash.
*/
static void
sound_state_manager_prepare_state_image_names (SoundStateManager* self)
{
  SoundStateManagerPrivate* priv = SOUND_STATE_MANAGER_GET_PRIVATE(self);
  priv->volume_states = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free); 
  g_hash_table_insert (priv->volume_states, GINT_TO_POINTER(MUTED), g_strdup("audio-volume-muted-panel"));
  g_hash_table_insert (priv->volume_states, GINT_TO_POINTER(ZERO_LEVEL), g_strdup("audio-volume-low-zero-panel"));
  g_hash_table_insert (priv->volume_states, GINT_TO_POINTER(LOW_LEVEL), g_strdup("audio-volume-low-panel"));
  g_hash_table_insert (priv->volume_states, GINT_TO_POINTER(MEDIUM_LEVEL), g_strdup("audio-volume-medium-panel"));
  g_hash_table_insert (priv->volume_states, GINT_TO_POINTER(HIGH_LEVEL), g_strdup("audio-volume-high-panel"));
  g_hash_table_insert (priv->volume_states, GINT_TO_POINTER(BLOCKED), g_strdup("audio-volume-muted-blocking-panel"));
  g_hash_table_insert (priv->volume_states, GINT_TO_POINTER(UNAVAILABLE), g_strdup("audio-output-none-panel"));
}

/*
prepare_blocked_animation:
Prepares the array of images to be used in the blocked animation.
Only called at startup.
*/
static void
sound_state_manager_prepare_blocked_animation (SoundStateManager* self)
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


GtkImage*
sound_state_manager_get_current_icon (SoundStateManager* self)
{
  SoundStateManagerPrivate* priv = SOUND_STATE_MANAGER_GET_PRIVATE(self);
  return priv->speaker_image;  
}

SoundState
sound_state_manager_get_current_state (SoundStateManager* self)
{
  SoundStateManagerPrivate* priv = SOUND_STATE_MANAGER_GET_PRIVATE(self);
  return priv->current_state;
}

static void 
sound_state_signal_cb ( GDBusProxy* proxy,
                        gchar* sender_name,
                        gchar* signal_name,
                        GVariant* parameters,
                        gpointer user_data)
{
  g_return_if_fail (SOUND_IS_STATE_MANAGER (user_data));
  SoundStateManager* self = SOUND_STATE_MANAGER (user_data);
  SoundStateManagerPrivate* priv = SOUND_STATE_MANAGER_GET_PRIVATE(self);

  g_variant_ref (parameters);
  GVariant *value = g_variant_get_child_value (parameters, 0);
  gint update = g_variant_get_int32 (value);

  g_debug ( "!!! signal_cb with value %i", update);

  priv->current_state = (SoundState)update;

  g_variant_unref (parameters);

  
  /*if (g_strcmp0(signal_name, INDICATOR_SOUND_SIGNAL_SINK_AVAILABLE_UPDATE) == 0){
    react_to_signal_sink_availability_update ( input, self );
  }
  else if (g_strcmp0(signal_name, INDICATOR_SOUND_SIGNAL_SINK_MUTE_UPDATE) == 0){
    react_to_signal_sink_mute_update ( input, self );
  }
  else if (g_strcmp0(signal_name, INDICATOR_SOUND_SIGNAL_SINK_INPUT_WHILE_MUTED) == 0){
    react_to_signal_sink_input_while_muted ( input, self );
  }*/
  
}

void
sound_state_manager_style_changed_cb(GtkWidget *widget, gpointer user_data)
{
  //g_debug("Just caught a style change event");
  g_return_if_fail (SOUND_IS_STATE_MANAGER (user_data));
  SoundStateManager* self = SOUND_STATE_MANAGER (user_data);
  sound_state_manager_reset_mute_blocking_animation (self);
  sound_state_manager_free_the_animation_list (self);
  sound_state_manager_prepare_blocked_animation (self);
}

static void
sound_state_manager_reset_mute_blocking_animation (SoundStateManager* self)
{
  if (animation_id != 0) {
    //g_debug("about to remove the animation_id callback from the mainloop!!**");
    g_source_remove(animation_id);
    animation_id = 0;
  }
  if (blocked_id != 0) {
    //g_debug("about to remove the blocked_id callback from the mainloop!!**");
    g_source_remove(blocked_id);
    blocked_id = 0;
  }
}

static void
sound_state_manager_free_the_animation_list (SoundStateManager* self)
{
  SoundStateManagerPrivate* priv = SOUND_STATE_MANAGER_GET_PRIVATE(self);
  
  if (priv->blocked_animation_list != NULL) {
    g_list_foreach (priv->blocked_animation_list, (GFunc)g_object_unref, NULL);
    g_list_free (priv->blocked_animation_list);
    priv->blocked_animation_list = NULL;
  }
}

/*static void
update_state(const gint state)
{
  previous_state = current_state;
  current_state = state;
  gchar* image_name = g_hash_table_lookup(volume_states, GINT_TO_POINTER(current_state));
  indicator_image_helper_update(speaker_image, image_name);
}*/

static gboolean
sound_state_manager_start_animation (SoundStateManager* self)
{
  SoundStateManagerPrivate* priv = SOUND_STATE_MANAGER_GET_PRIVATE(self);
  
  blocked_iter = priv->blocked_animation_list;
  blocked_id = 0;
  animation_id = g_timeout_add (50,
                                sound_state_manager_fade_back_to_mute_image,
                                self);
  return FALSE;
}

static gboolean
sound_state_manager_fade_back_to_mute_image (gpointer user_data)
{
  g_return_val_if_fail (SOUND_IS_STATE_MANAGER (user_data), FALSE);
  SoundStateManagerPrivate* priv = SOUND_STATE_MANAGER_GET_PRIVATE( SOUND_STATE_MANAGER (user_data) );

  if (blocked_iter != NULL) {
    gtk_image_set_from_pixbuf (priv->speaker_image, blocked_iter->data);
    blocked_iter = blocked_iter->next;
    return TRUE;
  } else {
    animation_id = 0;
    //g_debug("exit from animation now\n");
    return FALSE;
  }
}

/**
 * sound_state_manager_connect_to_dbus:
 * @returns: void
 * When ready the indicator-sound calls this method to enable state communication 
 * between the indicator and the service. 
 **/
void
sound_state_manager_connect_to_dbus (SoundStateManager* self, GDBusProxy* proxy)
{
  SoundStateManagerPrivate* priv = SOUND_STATE_MANAGER_GET_PRIVATE(self);
  priv->dbus_proxy = proxy;
  g_signal_connect (priv->dbus_proxy, "g-signal",
                    G_CALLBACK (sound_state_signal_cb), self);
  
}
