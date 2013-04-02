/*
Copyright 2013 Canonical Ltd.

Authors:
    Marco Trevisan <marco.trevisan@canonical.com>

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

[DBus (name = "org.gtk.Application")]
public interface DBusGtkApplication : Object {
  public abstract void Activate(GLib.HashTable<string, Variant?> platform_data) throws IOError;
}

public class PlayerActivator : GLib.Object
{
  public PlayerController owner {get; construct;}

  private bool gtk_application_searched = false;
  private DBusGtkApplication gtk_application;
  private Bamf.Application bamf_application;

  private const uint MAX_BAMF_APPLICATION_WAIT_MS = 1000;
  private int64 last_check_time;

  public PlayerActivator(PlayerController ctrl)
  {
    GLib.Object(owner: ctrl);
  }

  public void activate(uint timestamp)
  {
    if (!activate_gtk_appplication(timestamp)) {
      if (!activate_bamf_appplication(timestamp)) {
        // Let's wait BAMF to update its windows list
        this.last_check_time = get_monotonic_time();

        Idle.add(() => {
          bool activated = activate_bamf_appplication(timestamp);
          int64 waited = (get_monotonic_time() - this.last_check_time) / 1000;
          return !activated && waited < MAX_BAMF_APPLICATION_WAIT_MS;
        });
      }
    }
  }

  private bool activate_gtk_appplication(uint timestamp)
  {
    this.setup_gtk_application();

    if (this.gtk_application == null) {
      return false;
    }

    var context = Gdk.Display.get_default().get_app_launch_context();
    context.set_timestamp(timestamp);

    var data = new GLib.HashTable<string, Variant?>(str_hash, str_equal);
    data["desktop-startup-id"] = context.get_startup_notify_id(this.owner.app_info, new GLib.List<GLib.File>());

    try {
        this.gtk_application.Activate(data);
    }
    catch (IOError e) {
      return false;
    }

    return true;
  }

  private void setup_gtk_application()
  {
    if (owner.current_state != PlayerController.state.CONNECTED)
      return;

    if (this.gtk_application != null || this.gtk_application_searched)
      return;

    try {
      var connection = Bus.get_sync(BusType.SESSION);
      var name = this.owner.dbus_name;
      string gtk_application_path;
      this.find_iface_path(connection, name, "/", "org.gtk.Application", out gtk_application_path);
      this.gtk_application_searched = true;

      if (gtk_application_path != null) {
        this.gtk_application = Bus.get_proxy_sync(BusType.SESSION, this.owner.dbus_name, gtk_application_path);
      }
    } catch (Error e) {
      return;
    }
  }

  private void find_iface_path(DBusConnection connection, string name, string path, string target_iface, out string found_path)
  {
    found_path = null;
    DBusNodeInfo node = null;

    try {
      unowned string xml_string;
      var xml = connection.call_sync(name, path, "org.freedesktop.DBus.Introspectable", "Introspect", null, new VariantType("(s)"), DBusCallFlags.NONE, 1000);
      xml.get("(&s)", out xml_string);
      node = new DBusNodeInfo.for_xml(xml_string);
    } catch (Error e) {
      return;
    }

    if (node == null) {
      return;
    }

    foreach (var iface in node.interfaces) {
      if (iface.name == target_iface) {
        found_path = path;
        return;
      }
    }

    bool is_root = (path == "/");

    foreach (var subnode in node.nodes) {
      string new_path = path;

      if (!is_root) {
        new_path += "/";
      }

      new_path += subnode.path;

      find_iface_path(connection, name, new_path, target_iface, out found_path);

      if (found_path != null) {
        return;
      }
    }
  }

  private void setup_bamf_application()
  {
    this.bamf_application = null;
    var desktop_app = this.owner.app_info as DesktopAppInfo;

    if (desktop_app == null)
      return;

    foreach (var app in Bamf.Matcher.get_default().get_applications()) {
      if (app.get_desktop_file() == desktop_app.get_filename()) {
        this.bamf_application = app;
        break;
      }
    }
  }

  private bool activate_bamf_appplication(uint timestamp)
  {
    this.setup_bamf_application();

    if (this.bamf_application == null)
      return false;

    bool focused = false;
    var dpy = Gdk.Display.get_default();

    foreach (var win in this.bamf_application.get_windows()) {
      if (win.get_window_type() != Bamf.WindowType.NORMAL)
        continue;

      var xwin = Gdk.X11Window.foreign_new_for_display(dpy, win.get_xid());
      xwin.focus(timestamp);
      focused = true;
    }

    return focused;
  }
}