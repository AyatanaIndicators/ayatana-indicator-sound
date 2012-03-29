/*
Copyright 2010 Canonical Ltd.

Authors:
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
#include <gtk/gtk.h>
#include <libindicator/indicator-object.h>
#include "../src/indicator-sound.h"

static const gint STATE_MUTED = 0;
static const gint STATE_ZERO = 1;
static const gint STATE_LOW = 2;
static const gint STATE_MEDIUM = 3;
static const gint STATE_HIGH = 4;
static const gint STATE_MUTED_WHILE_INPUT = 5;
static const gint STATE_SINKS_NONE = 6;

void test_libindicator_sound_init()
{
	IndicatorObject * sound_menu = indicator_object_new_from_file(TOP_BUILD_DIR "/src/.libs/libsoundmenu.so");
	g_assert(sound_menu != NULL);
	g_object_unref(G_OBJECT(sound_menu));
}

void test_libindicator_determine_state()
{
	IndicatorObject * sound_menu = indicator_object_new_from_file(TOP_BUILD_DIR "/src/.libs/libsoundmenu.so");

    determine_state_from_volume(40);
	g_assert(get_state() == STATE_MEDIUM);

    determine_state_from_volume(0);
	g_assert(get_state() == STATE_ZERO);

    determine_state_from_volume(15);
	g_assert(get_state() == STATE_LOW);

    determine_state_from_volume(70);
	g_assert(get_state() == STATE_HIGH);

	g_object_unref(G_OBJECT(sound_menu));
}

void test_libindicator_image_names()
{

    gchar* muted_name = get_state_image_name(STATE_MUTED);        
    g_assert(g_ascii_strncasecmp("audio-volume-muted-panel", muted_name, strlen("audio-volume-muted-panel")) == 0);

    gchar* zero_name = get_state_image_name(STATE_ZERO);        
    g_assert(g_ascii_strncasecmp("audio-volume-low-zero-panel", zero_name, strlen("audio-volume-low-zero-panel")) == 0);

    gchar* low_name = get_state_image_name(STATE_LOW);        
    g_assert(g_ascii_strncasecmp("audio-volume-low-panel", low_name, strlen("audio-volume-low-panel")) == 0);

    gchar* medium_name = get_state_image_name(STATE_MEDIUM);        
    g_assert(g_ascii_strncasecmp("audio-volume-medium-panel", medium_name, strlen("audio-volume-medium-panel")) == 0);

    gchar* high_name = get_state_image_name(STATE_HIGH);        
    g_assert(g_ascii_strncasecmp("audio-volume-high-panel", high_name, strlen("audio-volume-high-panel")) == 0);

    gchar* blocked_name = get_state_image_name(STATE_MUTED_WHILE_INPUT);        
    g_assert(g_ascii_strncasecmp("audio-volume-muted-blocking-panel", blocked_name, strlen("audio-volume-muted-blocking-panel")) == 0);

    gchar* none_name = get_state_image_name(STATE_SINKS_NONE);        
    g_assert(g_ascii_strncasecmp("audio-output-none-panel", none_name, strlen("audio-output-none-panel")) == 0);
    
    //tidy_up_hash();
}


gint main (gint argc, gchar * argv[])
{
	g_type_init();
	g_test_init(&argc, &argv, NULL);

  g_test_add_func("/indicator-sound/indicator-sound/image_names", test_libindicator_image_names);

	return g_test_run ();
}

