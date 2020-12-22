[CCode (cprefix="", lower_case_cprefix="", cheader_filename="liblomiri-url-dispatcher/lomiri-url-dispatcher.h")]

namespace LomiriURLDispatch
{
  public delegate void LomiriURLDispatchCallback ();

  [CCode (cname = "lomiri_url_dispatch_send")]
  public static void send (string url, LomiriURLDispatchCallback? func = null);
}
