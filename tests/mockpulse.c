/*
Copyright 2010 Canonical Ltd.

Authors:
    Robert Collins <robert.collins@canonical.com>

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


#include <pulse/glib-mainloop.h>
#include <pulse/context.h>
#include <pulse/operation.h>
#include <pulse/introspect.h>
#include "mockpulse.h"

struct pa_context {
  int refcount;
  pa_context_notify_cb_t cb;
  void *cbdata;
  pa_context_state_t state;
};

pa_glib_mainloop *
pa_glib_mainloop_new(GMainContext *c)
{
    return NULL;
}

pa_context *
pa_context_new(pa_mainloop_api *loop, char const *name) {
  struct pa_context * result = g_new(pa_context, 1);
  result->refcount = 1;
  return result;
}

void
pa_context_unref(pa_context * context) {
  context->refcount--;
  if (!context->refcount)
    g_free(context);
}

void
pa_context_set_state_callback(pa_context *c, pa_context_notify_cb_t cb, void *userdata)
{
  c->cb = cb;
  c->cbdata = userdata;
}

void set_pa_context_get_state_result(pa_context *c, pa_context_state_t state)
{
  c->state = state;
}

pa_context_state_t
pa_context_get_state(pa_context *c)
{
  return c->state;
}

struct pa_operation {
  int refcount;
};


/*void pa_context_connect(pa_context* c, const char *server, pa_context_flags_t flags, const pa_spawn_api *api)*/
/*{*/
/*    set_pa_context_get_state_result(c, PA_CONTEXT_CONNECTING);*/
/*}*/

/* Can be made into a list if we need multiple callbacks */
static pa_sink_info *next_sink_info;

void
set_pa_context_get_sink_info(pa_sink_info *info) {
  next_sink_info = info;
}

pa_operation *
pa_context_get_sink_info_by_index(pa_context *c, uint32_t idx, pa_sink_info_cb_t cb, void * userdata)
{
  pa_operation *result = g_new(pa_operation, 1);
  result->refcount = 1;
  cb(c, next_sink_info, 0, userdata);
  return result;
}

void
pa_operation_unref(pa_operation * foo)
{
  foo->refcount--;
  if (!foo->refcount)
    g_free(foo);
}
