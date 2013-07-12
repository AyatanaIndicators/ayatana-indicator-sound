
[CCode (cheader_filename="libintl.h", type="char *")]
extern unowned string bind_textdomain_codeset (string domainname, string codeset);

static int main (string[] args) {
	bind_textdomain_codeset (Config.GETTEXT_PACKAGE, "UTF-8");
	Intl.setlocale (LocaleCategory.ALL, "");
	Intl.bindtextdomain (Config.GETTEXT_PACKAGE, Config.GNOMELOCALEDIR);

	var service = new IndicatorSound.Service ();
	return service.run ();
}
