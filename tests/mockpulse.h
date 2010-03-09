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

/**
 * Control interface for the mocked pa-glib-mainloop test library 
 */
#include <pulse/glib-mainloop.h>

void set_pa_context_get_state_result(pa_context *, pa_context_state_t state);
