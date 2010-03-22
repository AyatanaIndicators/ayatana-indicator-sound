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

#include <string.h>

/* we intend to test static functions which are not to be exported */
#include "../src/pulse-manager.c"
#include "mockpulse.h"

static pa_sink_info* mock_sink_info();



/*static void test_pa_context_exit()*/
/*{*/
/*    pa_context* context = pa_context_new(NULL, "foo");*/

/*    pa_context_set_state_callback(context, context_state_callback, NULL);*/
/*    // => call context_state_callback(context, NULL);*/
/*    // pa_context_get_state is mocked to return the right FAIL flag.*/
/*    set_pa_context_get_state_result(context, PA_CONTEXT_FAILED);*/
/*    context_state_callback(context, NULL);*/
/*    // Test to ensure context is tidied after failure.*/
/*    g_assert(context == NULL);*/
/*    // 2. then using the same pa_context_get_state we could ensure the manager*/
/*    // is attempting to reconnect to PA. */
/*    //g_assert(PA_CONTEXT_CONNECTING == pa_context_get_state(get_context()));*/
/*    //pa_context_unref(context);*/
/*}*/

static void test_sink_update
{
/*    pa_sink_info *expected = g_new0(pa_sink_info, 1);*/
/*    expected->name = g_strdup("foo");*/
/*    expected->index = 8;*/
/*    expected->description = g_strdup("more details");*/
    // fill it out here more.
    // hook into our pa_context_get_sink_info_by_index to pass exppected to
    // update_sink_info
    // set_pa_context_get_sink_info(expected);
    // update_sink_info is a static method in pulse-manager.c ?
/*    pa_context_get_sink_info_by_index(context, sink_details->index, update_sink_info, NULL);*/
    // the mockinkg lib should then return this mocked up sink_info to the
    // method update_sink_info which tests could be wrote against to make sure
    // everthing is populated correctly. 
/*    pa_context_unref(context);*/
/*    g_free(expected);*/


}

static void test_sink_cache_population()
{
    pa_context* context = pa_context_new(NULL, "foo");
    
    pa_sink_info* mock_sink = mock_sink_info();

    set_pa_context_get_sink_info(mock_sink);

    sink_hash = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, destroy_sink_info);

    pa_context_get_sink_info_by_index(context, mock_sink->index, pulse_sink_info_callback, NULL);

    GList *keys = g_hash_table_get_keys(sink_hash);
    gint position =  g_list_index(keys, GINT_TO_POINTER(mock_sink->index));

    g_assert(position >= 0);

    sink_info* stored_sink_info = g_hash_table_lookup(sink_hash, GINT_TO_POINTER(mock_sink->index));

    g_assert(g_ascii_strncasecmp(mock_sink->name, stored_sink_info->name, strlen(mock_sink->name)) == 0);
    g_assert(g_ascii_strncasecmp(mock_sink->description, stored_sink_info->description, strlen(mock_sink->description)) == 0);
    g_assert(!!mock_sink->mute == stored_sink_info->mute);
    g_assert(mock_sink->index == stored_sink_info->index);
    g_assert(pa_cvolume_equal(&mock_sink->volume, &stored_sink_info->volume));

    g_free(mock_sink);
    g_hash_table_destroy(sink_hash);
    pa_context_unref(context);
}

/**
Helper Methods
**/

static pa_sink_info* 
mock_sink_info() 
{
    pa_sink_info* mock_sink;
    mock_sink = g_new0(pa_sink_info, 1);
    mock_sink->index = 8;
    mock_sink->name = g_strdup("mock_sink");
    mock_sink->description = g_strdup("mock description");
    mock_sink->mute = 0;
    pa_cvolume volume; // nearly full volume:
    pa_cvolume_set(&volume, 1, 30000);
    mock_sink->volume = volume;
    return mock_sink;
}


gint main (gint argc, gchar * argv[])
{
  g_type_init();
  g_test_init(&argc, &argv, NULL);
  
  g_test_add_func("/indicator-sound/pulse-manager/sink-cache-population", test_sink_cache_population);
  //g_test_add_func("/indicator-sound/pulse-manager/pa-context-exit", test_pa_context_exit);
  
  return g_test_run ();
}
