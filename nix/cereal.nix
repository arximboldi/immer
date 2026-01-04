{
  stdenv,
  cmake,
  lib,
  fetchFromGitHub,
}:
stdenv.mkDerivation rec {
  name = "cereal-${version}";
  version = "git-${commit}";
  commit = "4bfaf5fee1cbc69db4614169092368a29c7607c4";
  src = fetchFromGitHub {
    owner = "arximboldi";
    repo = "cereal";
    rev = commit;
    hash = "sha256-G8V5g0POddpukPmiWAX/MhnIhi+EEVE/P+MQGCGH/J0=";
  };
  nativeBuildInputs = [ cmake ];
  cmakeFlags = [ "-DJUST_INSTALL_CEREAL=true" ];
  meta = {
    homepage = "http://uscilab.github.io/cereal";
    description = "A C++11 library for serialization";
    license = lib.licenses.bsd3;
  };
}
