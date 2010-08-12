public class MprisBridge : GLib.Object
{
	private MprisController mpris1_controller;
	private Mpris2Controller mpris2_controller;	
	private enum mode{
		MPRIS_1,
		MPRIS_2
	}
	private mode mode_in_use;
	
	public MprisBridge(PlayerController ctrl)
	{
		this.mpris2_controller = new Mpris2Controller(ctrl);
		if(this.mpris2_controller.was_successfull() == true){
			this.mode_in_use = mode.MPRIS_2;
			this.mpris1_controller = null;
			this.mpris2_controller.initial_update();
		}
		else{
			this.mpris2_controller = null;
			this.mode_in_use = mode.MPRIS_1;
			this.mpris1_controller = new MprisController(ctrl);
		}
	}
	
	// The handling of both mpris controllers can be abstracted further 
	// once the mpris2 is implemented. This will allow for one instance
	// variable to point at the active controller. For now handle both ...  
	public bool connected()
	{
		if(this.mode_in_use == mode.MPRIS_1){
			return this.mpris1_controller.connected();			
		}
		else if(this.mode_in_use == mode.MPRIS_2){
			return this.mpris2_controller.connected();			
		}
		return false;
	}
	
	public void transport_update(TransportMenuitem.action update)
	{
		if(this.mode_in_use == mode.MPRIS_1){
			this.mpris1_controller.transport_event(update);
		}
		else if(this.mode_in_use == mode.MPRIS_2){
			this.mpris2_controller.transport_event(update);
		}
	}

	public void set_track_position(double pos)
	{
		if(this.mode_in_use == mode.MPRIS_1){
			this.mpris1_controller.set_position(pos);
		}
		else if(this.mode_in_use == mode.MPRIS_2){
			this.mpris2_controller.set_position(pos);
		}
	}	
}