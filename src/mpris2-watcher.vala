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

using Xml;

public class Mpris2Watcher : GLib.Object
{
  DBusConnection session_bus;

  public signal void client_appeared ( string desktop_file_name,
                                       string dbus_name,
                                       bool use_playlists );
  public signal void client_disappeared ( string dbus_name );

  public Mpris2Watcher ()
  {
  }

  construct
  {
    const string match_rule = "type='signal'," +
                              "sender='org.freedesktop.DBus'," +
                              "interface='org.freedesktop.DBus'," +
                              "member='NameOwnerChanged'," +
                              "path='/org/freedesktop/DBus'," +
                              "arg0namespace='org.mpris.MediaPlayer2'";
    try {
      this.session_bus = Bus.get_sync (BusType.SESSION);

      this.session_bus.call_sync ("org.freedesktop.DBus",
                                  "/",
                                  "org.freedesktop.DBus",
                                  "AddMatch",
                                  new Variant ("(s)", match_rule),
                                  VariantType.TUPLE,
                                  DBusCallFlags.NONE,
                                  -1);

      this.session_bus.signal_subscribe ("org.freedesktop.DBus",
                                         "org.freedesktop.DBus",
                                         "NameOwnerChanged",
                                         "/org/freedesktop/DBus",
                                         null,
                                         DBusSignalFlags.NO_MATCH_RULE,
                                         this.name_owner_changed);

      this.check_for_active_clients.begin();
    }
    catch (GLib.Error e) {
      warning ("unable to set up name watch for mrpis clients: %s", e.message);
    }
  }

  // At startup check to see if there are clients up that we are interested in
  // More relevant for development and daemon's like mpd. 
  public async void check_for_active_clients()
  {
    Variant interfaces;

    try {
      interfaces = yield this.session_bus.call ("org.freedesktop.DBus",
                                                "/",
                                                "org.freedesktop.DBus",
                                                "ListNames",
                                                null,
                                                new VariantType ("(as)"),
                                                DBusCallFlags.NONE,
                                                -1);
    }
    catch (GLib.Error e) {
      warning ("unable to search for existing mpris clients: %s ", e.message);
      return;
    }

    foreach (var val in interfaces.get_child_value (0)) {
      var address = (string) val;
      if (address.has_prefix (MPRIS_PREFIX)){
        MprisRoot? mpris2_root = this.create_mpris_root(address);
        if (mpris2_root == null) return;
        bool use_playlists = this.supports_playlists ( address );
        client_appeared (mpris2_root.DesktopEntry + ".desktop", address, use_playlists);
      }
    }
  }

  public void name_owner_changed (DBusConnection con, string sender, string object_path,
                                  string interface_name, string signal_name, Variant parameters)
  {
    string name, previous_owner, current_owner;

	message ("xyxp");

    parameters.get ("(sss)", out name, out previous_owner, out current_owner);

    MprisRoot? mpris2_root = this.create_mpris_root (name);
    if (mpris2_root == null) return;

    if (previous_owner != "" && current_owner == "") {
      debug ("Client '%s' gone down", name);
      client_disappeared (name);
    }
    else if (previous_owner == "" && current_owner != "") {
      debug ("Client '%s' has appeared", name);
      bool use_playlists = this.supports_playlists ( name );
      client_appeared (mpris2_root.DesktopEntry + ".desktop", name, use_playlists);
    }
  }

  private MprisRoot? create_mpris_root ( string name ){
    MprisRoot mpris2_root = null;
    if ( name.has_prefix (MPRIS_PREFIX) ){
      try {
        mpris2_root = Bus.get_proxy_sync (  BusType.SESSION,
                                            name,
                                            MPRIS_MEDIA_PLAYER_PATH );
      }
      catch (IOError e){
        warning( "Mpris2watcher could not create a root interface: %s",
                  e.message );
      }
    }
    return mpris2_root;
  }

  private bool supports_playlists ( string name )
  {
    FreeDesktopIntrospectable introspectable;

    try {
      /* The dbusproxy flag parameter is needed to ensure Banshee does not 
       blow up. I suspect the issue is that if you
       try to instantiate a dbus object which does not have any properties 
       associated  with it, gdbus will attempt to fetch the properties (this is
       in the documentation) but the banshee mpris dbus object more than likely 
       causes a crash because it doesn't check for the presence of properties 
       before attempting to access them.
      */
      introspectable = Bus.get_proxy_sync (  BusType.SESSION,
                                             name,
                                             MPRIS_MEDIA_PLAYER_PATH, 
                                             GLib.DBusProxyFlags.DO_NOT_LOAD_PROPERTIES);
      var results = introspectable.Introspect();
      return this.parse_interfaces (results);
    }
    catch (IOError e){
      warning( "Could not create an introspectable object: %s",
                e.message );
    }
    return false;
  }

  private bool parse_interfaces( string interface_info )
  {
    //parse the document from path
    bool result = false;
    Xml.Doc* xml_doc = Parser.parse_doc (interface_info);
    if (xml_doc == null) {
      warning ("Mpris2Watcher - parse-interfaces - failed to instantiate xml doc");
      return false;
    }
    //get the root node. notice the dereferencing operator -> instead of .
    Xml.Node* root_node = xml_doc->get_root_element ();
    if (root_node == null) {
      //free the document manually before throwing because the garbage collector can't work on pointers
      delete xml_doc;
      warning ("Mpris2Watcher - the interface info xml is empty");
      return false;
    }

    //let's parse those nodes
    for (Xml.Node* iter = root_node->children; iter != null; iter = iter->next) {
      //spaces btw. tags are also nodes, discard them
      if (iter->type != ElementType.ELEMENT_NODE){
        continue;
      }
      Xml.Attr* attributes = iter->properties; //get the node's name    
      string interface_name = attributes->children->content;
      debug ( "this dbus object has interface %s ", interface_name );
      if ( interface_name == MPRIS_PREFIX.concat("Playlists")){
        result = true;
      }
    }
    delete xml_doc;
    return result;
  }  
}
