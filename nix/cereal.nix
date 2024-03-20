{
  arximboldi-cereal-src,
  stdenv,
  cmake,
  lib,
}:
stdenv.mkDerivation rec {
  name = "cereal-${version}";
  version = "git-${commit}";
  commit = arximboldi-cereal-src.rev;
  src = arximboldi-cereal-src;
  nativeBuildInputs = [cmake];
  cmakeFlags = ["-DJUST_INSTALL_CEREAL=true"];
  meta = {
    homepage = "http://uscilab.github.io/cereal";
    description = "A C++11 library for serialization";
    license = lib.licenses.bsd3;
  };
}
