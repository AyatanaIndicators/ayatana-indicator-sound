/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Pete Woods <pete.woods@canonical.com>
 */

#include <unity/gmenuharness/MatchUtils.h>

#include <unity/util/ResourcePtr.h>

using namespace std;
namespace util = unity::util;

namespace unity
{

namespace gmenuharness
{

void waitForCore (GObject * obj, const string& signalName, unsigned int timeout) {
    shared_ptr<GMainLoop> loop(g_main_loop_new(nullptr, false), &g_main_loop_unref);

    /* Our two exit criteria */
    util::ResourcePtr<gulong, function<void(gulong)>> signal(
            g_signal_connect_swapped(obj, signalName.c_str(),
                                     G_CALLBACK(g_main_loop_quit), loop.get()),
            [obj](gulong s)
            {
                g_signal_handler_disconnect(obj, s);
            });

    util::ResourcePtr<guint, function<void(guint)>> timer(g_timeout_add(timeout,
            [](gpointer user_data) -> gboolean
            {
                g_main_loop_quit((GMainLoop *)user_data);
                return G_SOURCE_CONTINUE;
            },
            loop.get()),
            &g_source_remove);

    /* Wait for sync */
    g_main_loop_run(loop.get());
}

void menuWaitForItems(const shared_ptr<GMenuModel>& menu, unsigned int timeout)
{
    waitForCore(G_OBJECT(menu.get()), "items-changed", timeout);
}

void g_object_deleter(gpointer object)
{
    g_clear_object(&object);
}

void gvariant_deleter(GVariant* varptr)
{
    if (varptr != nullptr)
    {
        g_variant_unref(varptr);
    }
}

}   // namespace gmenuharness

}   // namespace unity
