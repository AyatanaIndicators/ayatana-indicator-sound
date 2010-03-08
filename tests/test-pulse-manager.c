#include "defintions.h"
// I know this is not nice but I would rather not make everything public ?
#include "pulse-manager.c"

pa_context* context;

static void pa_context_exit()
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
    value->name = "mock_sink";"
    value->description = "mock description"
    value->mute = FALSE
    value->volume = 30000; // almost full
    // update_sink_info is a static method in pulse-manager.c ?
    pa_context_get_sink_info_by_index(context, value->index, update_sink_info);
    // the mockinkg lib should then return this mocked up sink_info to the method update_sink_info which tests could be wrote against to make sure everthing is populated correctly. 
}

