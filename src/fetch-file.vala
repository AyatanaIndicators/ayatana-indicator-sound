public class FetchFile : Object
{
  /* public variables */
  public string uri {get; construct;}
	public string intended_property {get; construct;}

  /* private variables */
  private DataInputStream stream;
  private File? file;
  private ByteArray data;

  /* public signals */
  public signal void failed ();
  public signal void completed (ByteArray data, string property);

  public FetchFile (string uri, string prop)
  {
    Object (uri: uri, intended_property: prop);
  }

  construct
  {
    this.file = File.new_for_uri(this.uri);
    this.data = new ByteArray ();
  }

  public async void fetch_data ()
  {
    //grab our data from our uri
    try {
      this.stream = new DataInputStream(this.file.read(null));
      this.stream.set_byte_order (DataStreamByteOrder.LITTLE_ENDIAN);
    } catch (GLib.Error e) {
      this.failed ();
    }
    this.read_something_async ();
  }

  private async void read_something_async ()
  {
    ssize_t size = 1024;
    uint8[] buffer = new uint8[size];

    ssize_t bufsize = 1;
    do {
      try {
        bufsize = yield this.stream.read_async (buffer, size, GLib.Priority.DEFAULT, null);
        if (bufsize < 1) { break;}

        if (bufsize != size)
          {
            uint8[] cpybuf = new uint8[bufsize];
            Memory.copy (cpybuf, buffer, bufsize);
            this.data.append (cpybuf);
          }
        else
          {
            this.data.append (buffer);
          }
      } catch (Error e) {
        this.failed ();
      }
    } while (bufsize > 0);
    this.completed (this.data, this.intended_property);
  }
}
