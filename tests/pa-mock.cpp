
#include <atomic>
#include <functional>
#include <vector>

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <gio/gio.h>
#include <math.h>

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#endif
#define G_LOG_DOMAIN "PA-Mock"

/* Core class of the PA Mock state */
class PAMockContext {
public:
	/* Accounting */
	std::atomic<unsigned int> refcnt;

	/* State stuff */
	std::vector<std::function<void(void)>> stateCallbacks;
	pa_context_state_t currentState;
	pa_context_state_t futureState;

	/* Event stuff */
	std::vector<std::function<void(pa_subscription_event_type_t, uint32_t)>> eventCallbacks;
	pa_subscription_mask_t eventMask;

	PAMockContext ()
		: refcnt(1)
		, currentState(PA_CONTEXT_UNCONNECTED)
		, futureState(PA_CONTEXT_UNCONNECTED)
		, eventMask(PA_SUBSCRIPTION_MASK_NULL)
	{
		g_debug("Creating Context: %p", this);
	}

	~PAMockContext () {
		g_debug("Destroying Context: %p", this);
	}

	/* Ref counting */
	void ref () {
		refcnt++;
	}

	void unref () {
		refcnt--;
		if (refcnt == 0)
			delete this;
	}

	/* C/C++ boundry */
	operator pa_context *() {
		return reinterpret_cast<pa_context *>(this);
	}

	/* State Stuff */
	void setState (pa_context_state_t state)
	{
		if (state == currentState)
			return;

		currentState = state;
		for (auto callback : stateCallbacks) {
			callback();
		}
	}

	void idleOnce (std::function<void(void)> idleFunc) {
		auto allocated = new std::function<void(void)>(idleFunc);

		g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
			[](gpointer data) -> gboolean {
				std::function<void(void)> * pidleFunc = reinterpret_cast<std::function<void(void)> *>(data);
				(*pidleFunc)();
				return G_SOURCE_REMOVE;
			},
			allocated,
			[](gpointer data) -> void {
				std::function<void(void)> * pidleFunc = reinterpret_cast<std::function<void(void)> *>(data);
				delete pidleFunc;
			});
	}

	void queueState (pa_context_state_t state)
	{
		idleOnce([this, state](){
			setState(state);
		});
	}

	pa_context_state_t getState (void)
	{
		return currentState;
	}

	void addStateCallback (std::function<void(void)> &callback)
	{
		stateCallbacks.push_back(callback);
	}

	/* Event Stuff */
	void setMask (pa_subscription_mask_t mask)
	{
		eventMask = mask;
	}

	void addEventCallback (std::function<void(pa_subscription_event_type_t, uint32_t)> &callback)
	{
		eventCallbacks.push_back(callback);
	}
};

/* *******************************
 * context.h
 * *******************************/

pa_context *
pa_context_new_with_proplist (pa_mainloop_api *mainloop, const char *name, pa_proplist *proplist)
{
	return *(new PAMockContext());
}

void
pa_context_unref (pa_context *c) {
	reinterpret_cast<PAMockContext*>(c)->unref();
}

pa_context *
pa_context_ref (pa_context *c) {
	reinterpret_cast<PAMockContext*>(c)->ref();
	return c;
}

int
pa_context_connect (pa_context *c, const char *server, pa_context_flags_t flags, const pa_spawn_api *api)
{
	reinterpret_cast<PAMockContext*>(c)->queueState(PA_CONTEXT_READY);
	return 0;
}

void
pa_context_disconnect (pa_context *c)
{
	reinterpret_cast<PAMockContext*>(c)->queueState(PA_CONTEXT_UNCONNECTED);
}

int
pa_context_errno (pa_context *c)
{
	return 0;
}

void
pa_context_set_state_callback (pa_context *c, pa_context_notify_cb_t cb, void *userdata)
{
	std::function<void(void)> cppcb([c, cb, userdata]() {
		cb(c, userdata);
	});
	reinterpret_cast<PAMockContext*>(c)->addStateCallback(cppcb);
}

pa_context_state_t
pa_context_get_state (pa_context *c)
{
	return reinterpret_cast<PAMockContext*>(c)->getState();
}

/* *******************************
 * introspect.h
 * *******************************/

static pa_operation *
dummy_operation (void)
{
	GObject * goper = (GObject *)g_object_new(G_TYPE_OBJECT, nullptr);
	pa_operation * oper = (pa_operation *)goper;
	return oper;
}

pa_operation*
pa_context_get_server_info (pa_context *c, pa_server_info_cb_t cb, void *userdata)
{
	reinterpret_cast<PAMockContext*>(c)->idleOnce(
	[c, cb, userdata]() {
		if (cb == nullptr)
			return;

		pa_server_info server{
			.user_name = "user",
			.host_name = "host",
			.server_version = "1.2.3",
			.server_name = "server",
			.sample_spec = {
				.format = PA_SAMPLE_U8,
				.rate = 44100,
				.channels = 1
			},
			.default_sink_name = "default-sink",
			.default_source_name = "default-source",
			.cookie = 1234,
			.channel_map = {
				.channels = 0
			}
		};

		cb(c, &server, userdata);
	});

	return dummy_operation();
}

pa_operation*
pa_context_get_sink_info_by_name (pa_context *c, const gchar * name, pa_sink_info_cb_t cb, void *userdata)
{
	reinterpret_cast<PAMockContext*>(c)->idleOnce(
	[c, name, cb, userdata]() {
		if (cb == nullptr)
			return;

		pa_sink_port_info active_port = {0};
		active_port.name = "speaker";

		pa_sink_info sink = {0};
		sink.name = "default-sink";
		sink.index = 0;
		sink.description = "Default Sink";
		sink.channel_map.channels = 0;
		sink.active_port = &active_port;

		cb(c, &sink, 1, userdata);
	});

	return dummy_operation();
}

pa_operation*
pa_context_get_sink_info_list (pa_context *c, pa_sink_info_cb_t cb, void *userdata)
{
	/* Only have one today, so this is the same */
	return pa_context_get_sink_info_by_name(c, "default-sink", cb, userdata);
}

pa_operation *
pa_context_get_sink_input_info (pa_context *c, uint32_t idx, pa_sink_input_info_cb_t cb, void * userdata)
{
	reinterpret_cast<PAMockContext*>(c)->idleOnce(
	[c, idx, cb, userdata]() {
		if (cb == nullptr)
			return;

		pa_sink_input_info sink = { 0 };

		cb(c, &sink, 1, userdata);
	});

	return dummy_operation();
}

pa_operation*
pa_context_get_source_info_by_name (pa_context *c, const char * name, pa_source_info_cb_t cb, void *userdata)
{
	reinterpret_cast<PAMockContext*>(c)->idleOnce(
	[c, name, cb, userdata]() {
		if (cb == nullptr)
			return;

		pa_source_info source = {
			.name = "default-source"
		};

		cb(c, &source, 1, userdata);
	});

	return dummy_operation();
}

pa_operation*
pa_context_get_source_output_info (pa_context *c, uint32_t idx, pa_source_output_info_cb_t cb, void *userdata)
{
	reinterpret_cast<PAMockContext*>(c)->idleOnce(
	[c, idx, cb, userdata]() {
		if (cb == nullptr)
			return;

		pa_source_output_info source = {0};
		source.name = "default source";

		cb(c, &source, 1, userdata);
	});

	return dummy_operation();
}

pa_operation*
pa_context_set_sink_mute_by_index (pa_context *c, uint32_t idx, int mute, pa_context_success_cb_t cb, void *userdata)
{
	reinterpret_cast<PAMockContext*>(c)->idleOnce(
	[c, idx, mute, cb, userdata]() {
		if (cb != nullptr)
			cb(c, 1, userdata);
	});

	return dummy_operation();
}

pa_operation*
pa_context_set_sink_volume_by_index (pa_context *c, uint32_t idx, const pa_cvolume * cvol, pa_context_success_cb_t cb, void *userdata)
{
	reinterpret_cast<PAMockContext*>(c)->idleOnce(
	[c, idx, cvol, cb, userdata]() {
		if (cb != nullptr)
			cb(c, 1, userdata);
	});

	return dummy_operation();
}

pa_operation*
pa_context_set_source_volume_by_name (pa_context *c, const char * name, const pa_cvolume * cvol, pa_context_success_cb_t cb, void *userdata)
{
	reinterpret_cast<PAMockContext*>(c)->idleOnce(
	[c, name, cvol, cb, userdata]() {
		if (cb != nullptr)
			cb(c, 1, userdata);
	});

	return dummy_operation();
}

/* *******************************
 * subscribe.h
 * *******************************/

pa_operation *
pa_context_subscribe (pa_context * c, pa_subscription_mask_t mask, pa_context_success_cb_t callback, void * userdata)
{
	reinterpret_cast<PAMockContext*>(c)->idleOnce(
	[c, mask, callback, userdata]() {
		reinterpret_cast<PAMockContext*>(c)->setMask(mask);
		if (callback != nullptr)
			callback(c, 1, userdata);
	});

	return dummy_operation();
}

void
pa_context_set_subscribe_callback (pa_context * c, pa_context_subscribe_cb_t callback, void * userdata)
{
	std::function<void(pa_subscription_event_type_t, uint32_t)> cppcb([c, callback, userdata](pa_subscription_event_type_t event, uint32_t index) {
		if (callback != nullptr)
			callback(c, event, index, userdata);
	});

	reinterpret_cast<PAMockContext*>(c)->addEventCallback(cppcb);
}

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

	if (c == nullptr)
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

/* *******************************
 * proplist.h
 * *******************************/

pa_proplist *
pa_proplist_new (void)
{
	return (pa_proplist *)g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

void
pa_proplist_free (pa_proplist * p)
{
	g_return_if_fail(p != nullptr);
	g_hash_table_destroy((GHashTable *)p);
}

const char *
pa_proplist_gets (pa_proplist * p, const char * key)
{
	g_return_val_if_fail(p != nullptr, nullptr);
	g_return_val_if_fail(key != nullptr, nullptr);
	return (const char *)g_hash_table_lookup((GHashTable *)p, key);
}

int
pa_proplist_sets (pa_proplist *p, const char * key, const char * value)
{
	g_return_val_if_fail(p != nullptr, -1);
	g_return_val_if_fail(key != nullptr, -1);
	g_return_val_if_fail(value != nullptr, -1);

	g_hash_table_insert((GHashTable *)p, g_strdup(key), g_strdup(value));
	return 0;
}

/* *******************************
 * error.h
 * *******************************/

const char *
pa_strerror (int error)
{
	return "This is error text";
}

/* *******************************
 * volume.h
 * *******************************/

pa_volume_t
pa_sw_volume_from_dB (double f)
{
	double linear = pow(10.0, f / 20.0);

	pa_volume_t calculated = lround(cbrt(linear) * PA_VOLUME_NORM);

	if (G_UNLIKELY(calculated > PA_VOLUME_MAX)) {
		return PA_VOLUME_MAX;
	} else if (G_UNLIKELY(calculated < PA_VOLUME_MUTED)) {
		return PA_VOLUME_MUTED;
	} else {
		return calculated;
	}
}

pa_cvolume *
pa_cvolume_init (pa_cvolume * cvol)
{
	g_return_val_if_fail(cvol != nullptr, nullptr);

	cvol->channels = 0;

	unsigned int i;
	for (i = 0; i < PA_CHANNELS_MAX; i++)
		cvol->values[i] = PA_VOLUME_INVALID;

	return cvol;
}

pa_cvolume *
pa_cvolume_set (pa_cvolume * cvol, unsigned channels, pa_volume_t volume)
{
	g_return_val_if_fail(cvol != nullptr, nullptr);
	g_warn_if_fail(channels > 0);
	g_return_val_if_fail(channels <= PA_CHANNELS_MAX, nullptr);

	cvol->channels = channels;

	unsigned int i;
	for (i = 0; i < channels; i++) {
		if (G_UNLIKELY(volume > PA_VOLUME_MAX)) {
			cvol->values[i] = PA_VOLUME_MAX;
		} else if (G_UNLIKELY(volume < PA_VOLUME_MUTED)) {
			cvol->values[i] = PA_VOLUME_MUTED;
		} else {
			cvol->values[i] = volume;
		}
	}

	return cvol;
}

pa_volume_t
pa_cvolume_max (const pa_cvolume * cvol)
{
	g_return_val_if_fail(cvol != nullptr, PA_VOLUME_MUTED);
	pa_volume_t max = PA_VOLUME_MUTED;

	unsigned int i;
	for (i = 0; i < cvol->channels; i++)
		max = MAX(max, cvol->values[i]);
	
	return max;
}

pa_cvolume *
pa_cvolume_scale (pa_cvolume * cvol, pa_volume_t max)
{
	g_return_val_if_fail(cvol != nullptr, nullptr);

	pa_volume_t originalmax = pa_cvolume_max(cvol);

	if (originalmax <= PA_VOLUME_MUTED)
		return pa_cvolume_set(cvol, cvol->channels, max);

	unsigned int i;
	for (i = 0; i < cvol->channels; i++) {
		pa_volume_t calculated = (cvol->values[i] * max) / originalmax;

		if (G_UNLIKELY(calculated > PA_VOLUME_MAX)) {
			cvol->values[i] = PA_VOLUME_MAX;
		} else if (G_UNLIKELY(calculated < PA_VOLUME_MUTED)) {
			cvol->values[i] = PA_VOLUME_MUTED;
		} else {
			cvol->values[i] = calculated;
		}
	}

	return cvol;
}

