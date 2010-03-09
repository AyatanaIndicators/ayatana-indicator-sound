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
