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

public class GtkApplicationPlayer : GLib.Object
{
  public PlayerController owner {get; construct;}

  private bool gtk_application_searched = false;
  private DBusGtkApplication gtk_application;

  public GtkApplicationPlayer(PlayerController ctrl)
  {
    GLib.Object(owner: ctrl);
  }

  public void activate(uint timestamp)
  {
    this.setup_gtk_application();

    if (this.gtk_application == null) {
      return;
    }

    var context = Gdk.Display.get_default().get_app_launch_context();
    context.set_timestamp(timestamp);

    var data = new GLib.HashTable<string, Variant?>(str_hash, str_equal);
    data["desktop-startup-id"] = context.get_startup_notify_id(this.owner.app_info, new GLib.List<GLib.File>());

    try {
    	this.gtk_application.Activate(data);
    }
    catch (IOError e) {}
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
}