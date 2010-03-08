/*
Copyright 2010 Canonical Ltd.

Authors:
    Robert Collins <robert.collins@canonical.com>
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

/* we test static functions */
#include "../src/pulse-manager.c"

pa_context* context;

static void test_pa_context_exit()
{
    pa_context_set_state_callback(context, context_state_callback, NULL);
    // => call context_state_callback(context, NULL);
    // pa_context_get_state needs to be mocked to return the right FAIL flag.
    // 1. test to make sure relevant variables are tidied up
    // 2. then using the same pa_context_get_state we could ensure the manager is attempting to reconnect tp PA. 
}

static void test_sink_insert()
{
    sink_info *value;
    value = g_new0(sink_info, 1);
    value->index = 8;
    value->name = "mock_sink";
    value->description = "mock description";
    value->mute = FALSE;
    pa_cvolume volume; // nearly full volume:
    pa_cvolume_set(&volume, 1, 30000);
    value->volume = volume;
    // update_sink_info is a static method in pulse-manager.c ?
    pa_context_get_sink_info_by_index(context, value->index, update_sink_info, NULL);
    // the mockinkg lib should then return this mocked up sink_info to the method update_sink_info which tests could be wrote against to make sure everthing is populated correctly. 
}


gint main (gint argc, gchar * argv[])
{
  g_type_init();
  g_test_init(&argc, &argv, NULL);
  
  g_test_add_func("/indicator-sound/pulse-manager/sink-insert", test_sink_insert);
  g_test_add_func("/indicator-sound/pulse-manager/pa-context-exit", test_pa_context_exit);
  
  return g_test_run ();
}
