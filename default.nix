with import <nixpkgs> {};

stdenv.mkDerivation rec {
  name = "immer-git";
  version = "git";
  src = builtins.filterSource (path: type:
            baseNameOf path != ".git" &&
            baseNameOf path != "build" &&
            baseNameOf path != "_build" &&
            baseNameOf path != "reports" &&
            baseNameOf path != "tools")
          ./.;
  nativeBuildInputs = [ cmake ];
  dontBuild = true;
  meta = with stdenv.lib; {
    homepage    = "https://github.com/arximboldi/immer";
    description = "Postmodern immutable data structures for C++";
    license     = licenses.lgpl3Plus;
  };
}
