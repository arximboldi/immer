{ compiler ? "",
  nixpkgs ? (import <nixpkgs> {}).fetchFromGitHub {
    owner  = "NixOS";
    repo   = "nixpkgs";
    rev    = "630e25ba5cd30adb9efb8b6a64a295af780e62b4";
    sha256 = "1998kj0zn1qcxrqms95p1sw29084lyb15jbczamv5jm01dccqs07";
  }}:

with import nixpkgs {};

let
  # For the documentation tools we use an older Nixpkgs since the
  # newer versions seem to be not working great...
  oldNixpkgsSrc = fetchFromGitHub {
                    owner  = "NixOS";
                    repo   = "nixpkgs";
                    rev    = "d0d905668c010b65795b57afdf7f0360aac6245b";
                    sha256 = "1kqxfmsik1s1jsmim20n5l4kq6wq8743h5h17igfxxbbwwqry88l";
                  };
  oldNixpkgs    = import oldNixpkgsSrc {};
  docs          = import ./nix/docs.nix { nixpkgs = oldNixpkgsSrc; };
  benchmarks    = import ./nix/benchmarks.nix { inherit nixpkgs; };
  compilerPkg   = if compiler != ""
                  then pkgs.${compiler}
                  else stdenv.cc;
  theStdenv     = if compilerPkg.isClang
                  then llvmPackages_8.stdenv
                  else stdenv;
in

theStdenv.mkDerivation rec {
  name = "immer-env";
  buildInputs = [
    compilerPkg
    git
    cmake
    pkgconfig
    ninja
    gdb
    lldb
    ccache
    boost
    eigen
    boehmgc
    benchmarks.c_rrb
    benchmarks.steady
    benchmarks.chunkedseq
    benchmarks.immutable_cpp
    benchmarks.hash_trie
    oldNixpkgs.doxygen
    (oldNixpkgs.python.withPackages (ps: [
      ps.sphinx
      docs.breathe
      docs.recommonmark
    ]))
  ];
  hardeningDisable = [ "fortify" ];
}
