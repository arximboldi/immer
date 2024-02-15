{
  stdenv,
  cmake,
  lib,
  src,
}:
stdenv.mkDerivation {
  name = "immer-git";
  version = "git";
  inherit src;
  nativeBuildInputs = [cmake];
  dontBuild = true;
  dontUseCmakeBuildDir = true;
  cmakeFlags = [
    "-Dimmer_BUILD_TESTS=OFF"
    "-Dimmer_BUILD_EXAMPLES=OFF"
  ];
  meta = {
    homepage = "https://github.com/arximboldi/immer";
    description = "Postmodern immutable data structures for C++";
    license = lib.licenses.boost;
  };
}
