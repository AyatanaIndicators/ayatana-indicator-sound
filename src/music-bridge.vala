using Indicate;
using DbusmenuGlib;

public class BridgeServer : GLib.Object{

    private Listener listener;

//    private static const int LISTENING_MODE = 0;
//    private static const int MASTER_MODE = 0;
//    private int current_mode = LISTENING_MODE;

    public BridgeServer(){
        listener = Listener.ref_default();        
        listener.indicator_added.connect(on_indicator_added);
        listener.indicator_removed.connect(on_indicator_removed);
        listener.indicator_modified.connect(on_indicator_modified);
        listener.server_added.connect(on_server_added);
        listener.server_removed.connect(on_server_removed);
        listener.server_count_changed.connect(on_server_count_changed);
    }
		
		public void test_me(){
			debug("I'm being tested'");
		}

    public void on_indicator_added(Indicate.ListenerServer object, Indicate.ListenerIndicator p0){
        debug("BridgerServer -> on_indicator_added");
    }

    public void on_indicator_removed(Indicate.ListenerServer object, Indicate.ListenerIndicator p0){
        debug("BridgeServer -> on_indicator_removed");
    }

    public void on_indicator_modified(Indicate.ListenerServer object, Indicate.ListenerIndicator p0, string s){
        debug("BridgeServer -> indicator_modified with vale %s", s );
    }

    public void on_server_added(Indicate.ListenerServer object, string type){
        debug("BridgeServer -> on_server_added with value %s", type);
	    if (type == null) return;
        if (type.contains("music") == false){
            debug("server is of no interest,  it is not an music server");
            return;
        }
        else{
            debug("client of type %s has registered with us", type);
		        if (type.contains("rhythmbox") == true){
							new RhythmboxController();	
						}
        }
    }

    public void on_server_removed(Indicate.ListenerServer object, string s){
        debug("BridgeServer -> on_server_removed with value %s", s);
    }
    
    public void on_server_count_changed(Indicate.ListenerServer object, uint i){
        debug("BridgeServer -> on_server_count_changed with value %u", i);
    }

} 


//public void main (string[] args) {

//    // Creating a GLib main loop with a default context
//    var loop = new MainLoop(null, false);

//    BridgeServer server = new BridgeServer();

//    // Start GLib mainloop
//    loop.run();
//}



