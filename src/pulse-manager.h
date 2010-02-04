#include <pulse/pulseaudio.h>
#include <glib.h>
#include "sound-service-dbus.h"


typedef struct {
    gchar* name;
    gchar* description;
    gchar* icon_name;
    gint index;
    gint device_index;
    pa_cvolume volume;
    pa_channel_map channel_map;
    gboolean mute;
    gboolean active_port;
    pa_volume_t base_volume;
} sink_info;


pa_context* get_context(void);
void establish_pulse_activities(SoundServiceDbus *service);
void set_sink_volume(gdouble percent);
void toggle_global_mute(gboolean mute_value); 



