#include <pulse/pulseaudio.h>
#include <glib.h>
#include "sound-service-dbus.h"

//enum SinkInputType {
//    SINK_INPUT_ALL,
//    SINK_INPUT_CLIENT,
//    SINK_INPUT_VIRTUAL
//};

//enum SinkType {
//    SINK_ALL,
//    SINK_HARDWARE,
//    SINK_VIRTUAL,
//};

//enum SourceOutputType {
//    SOURCE_OUTPUT_ALL,
//    SOURCE_OUTPUT_CLIENT,
//    SOURCE_OUTPUT_VIRTUAL
//};

//enum SourceType {
//    SOURCE_ALL,
//    SOURCE_NO_MONITOR,
//    SOURCE_HARDWARE,
//    SOURCE_VIRTUAL,
//    SOURCE_MONITOR,
//};


typedef struct {
    gchar* name;
    gchar* description;
    gchar* icon_name;
    gint index;
    gint device_index;
//    pa_cvolume volume;
//    pa_channel_map channel_map;
    gboolean mute;
    gboolean active_port;
} sink_info;


//void set_volume(gint sink_index, gint volume_percent);
pa_context* get_context(void);
void establish_pulse_activities(SoundServiceDbus *service);
void set_sink_volume(guint percent);
void toggle_global_mute(gboolean mute_value); 



