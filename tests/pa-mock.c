
#include <pulseaudio.h>

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "PA-Mock"

G_DEFINE_QUARK("pa-mock-state-cb-list", state_cb);
typedef struct {
	pa_context_notify_cb_t cb;
	gpointer user_data;
} state_cb_t;

/* *******************************
 * context.h
 * *******************************/

static void
context_weak_cb (gpointer user_data, GObject * oldobj)
{
	g_debug("Finalizing context: %p", oldobj);
}

pa_context *
pa_context_new_with_proplist (pa_mainloop_api *mainloop, const char *name, pa_proplist *proplist)
{
	GObject * gctx = g_object_new(G_TYPE_OBJECT);
	pa_context * ctx = (pa_context *)gctx;

	g_debug("Creating new context: %p", ctx);
	g_object_weak_ref(gctx, context_weak_cb, NULL);

	return ctx;
}

void
pa_context_unref (pa_context *c) {
	g_return_if_fail(G_IS_OBJECT(c));
	g_object_unref(G_OBJECT(c));
}

void
pa_context_ref (pa_context *c) {
	g_return_if_fail(G_IS_OBJECT(c));
	g_object_ref(G_OBJECT(c));
}

int
pa_context_connect (pa_context *c, const char *server, pa_context_flags_t flags, const pa_spawn_api *api)
{
	g_return_if_fail(G_IS_OBJECT(c));
	g_debug("Context Connect");
	return 0;
}

void
pa_context_disconnect (pa_context *c)
{
	g_return_if_fail(G_IS_OBJECT(c));
	g_debug("Context Disconnect");
}

int
pa_context_errno (pa_context *c)
{
	g_return_val_if_fail(G_IS_OBJECT(c), -1);

	return 0;
}

static void
state_cb_list_destroy (gpointer data)
{
	GList * statelist = (GList *)data;
	g_list_free_full(statelist, g_free);
}

void
pa_context_set_state_callback (pa_context *c, pa_context_notify_cb_t cb, void *userdata)
{
	g_return_if_fail(G_IS_OBJECT(c));
	g_return_if_fail(cb != NULL);

	state_cb_t * state_cb = g_new0(state_cb_t, 1);
	state_cb->cb = cb;
	state_cb->userdata = userdata;

	GList * statelist = g_object_get_qdata(G_OBJECT(c), state_cb_quark);
	statelist = g_list_append(statelist, state_cb);
	g_object_set_qdata_full(G_OBJECT(c), state_cb_quark, state_cb, state_cb_list_destroy);
}

pa_context_state_t
pa_context_get_state (pa_context *c)
{
	g_return_if_fail(G_IS_OBJECT(c));

	return PA_CONTEXT_READY;
}

/* *******************************
 * introspect.h
 * *******************************/

typedef struct {
	pa_server_info_cb_t cb;
	gpointer userdata;
	pa_context * context;
} get_server_info_t;

static void
get_server_info_free (gpointer data)
{
	get_server_info_t * info = (get_server_info_t *)data;
	g_object_unref(info->context);
	g_free(info);
}

static gboolean
get_server_info_cb (gpointer data)
{
	pa_server_info server = {
		.user_name = "user",
		.host_name = "host",
		.server_version = "1.2.3",
		.server_name = "server",
		.sample_spec = ,
		.default_sink_name = "default-sink",
		.default_source_name = "default-source",
		.cookie = 1234,
		.channel_map = {
			.channels = 0
		}
	};
	get_server_info_t * info = (get_server_info_t *)data;

	info->cb(info->context, &server, info->userdata);

	return G_SOURCE_REMOVE;
}

pa_operation*
pa_context_get_server_info (pa_context *c, pa_server_info_cb_t cb, void *userdata)
{
	g_return_val_if_fail(G_IS_OBJECT(c), NULL);
	g_return_val_if_fail(cb != NULL, NULL);

	get_server_info_t * info = g_new(get_server_info_t, 1);
	info->cb = cb;
	info->userdata = userdata;
	info->context = g_objet_ref(c);

	g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
		get_server_info_cb,
		info,
		get_server_info_free);

	GObject * goper = g_object_new(G_TYPE_OBJECT);
	pa_operation * oper = (pa_operation *)goper;
	return oper;
}

typedef struct {
	pa_sink_info_cb_t cb;
	gpointer userdata;
	pa_context * context;
	uint32_t index;
} get_sink_info_t;

static void
get_sink_info_free (gpointer data)
{
	get_sink_info_t * info = (get_sink_info_t *)data;
	g_object_unref(info->context);
	g_free(info);
}

static gboolean
get_info_info_cb (gpointer data)
{
	pa_sink_info sink = {
		.name = "default-sink",
		.index = 0,
		.description = "Default Sink",
		.channel_map = {
			.channels = 0
		}
	};
	get_sink_info_t * info = (get_sink_info_t *)data;

	info->cb(info->context, &sink, 1, info->userdata);

	return G_SOURCE_REMOVE;
}

pa_operation*
pa_context_get_sink_info_by_name (pa_context *c, const gchar * name, pa_sink_info_cb_t cb, void *userdata)
{
	g_return_val_if_fail(G_IS_OBJECT(c), NULL);
	g_return_val_if_fail(g_strcmp0(name, "default-sink") == 0, NULL);
	g_return_val_if_fail(cb != NULL, NULL);

	get_sink_info_t * info = g_new(get_sink_info_t, 1);
	info->cb = cb;
	info->userdata = userdata;
	info->context = g_objet_ref(c);

	g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
		get_sink_info_cb,
		info,
		get_sink_info_free);

	GObject * goper = g_object_new(G_TYPE_OBJECT);
	pa_operation * oper = (pa_operation *)goper;
	return oper;
}

pa_operation*
pa_context_get_sink_info_list (pa_context *c, pa_sink_info_cb_t cb, void *userdata)
{
	/* Only have one today, so this is the same */
	return pa_context_get_sink_info_by_name(c, "default-sink", cb, userdata);
}


pa_context_get_sink_input_info
pa_context_get_source_info_by_name
pa_context_get_source_output_info
pa_context_get_state

pa_context_set_sink_mute_by_index
pa_context_set_sink_volume_by_index
pa_context_set_source_volume_by_name
pa_context_set_subscribe_callback
pa_context_subscribe

pa_cvolume_init
pa_cvolume_max
pa_cvolume_scale
pa_cvolume_set

/* *******************************
 * glib-mainloop.h
 * *******************************/

struct pa_glib_mainloop {
	GMainContext * context;
};

struct pa_mainloop_api mock_mainloop = { 0 };

pa_mainloop_api *
pa_glib_mainloop_get_api (pa_glib_mainloop * g)
{
	return &mock_mainloop;
}

pa_glib_mainloop *
pa_glib_mainloop_new (GMainContext * c)
{
	pa_glib_mainloop * loop = g_new0(pa_glib_mainloop, 1);

	if (c == NULL)
		loop->context = g_main_context_default();
	else
		loop->context = c;

	g_main_context_ref(loop->context);
	return loop;
}

void
pa_glib_mainloop_free (pa_glib_mainloop * g)
{
	g_main_context_unref(g->context);
	g_free(g);
}

/* *******************************
 * operation.h
 * *******************************/

void
pa_operation_unref (pa_operation * operation)
{
	g_return_if_fail(G_IS_OBJECT(operation));
	g_object_unref(G_OBJECT(operation));
}

pa_proplist_free
pa_proplist_gets
pa_proplist_new
pa_proplist_sets

pa_strerror

pa_sw_volume_from_dB

