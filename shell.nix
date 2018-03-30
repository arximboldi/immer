{ compiler ? "",
  nixpkgs ? (import <nixpkgs> {}).fetchFromGitHub {
    owner  = "NixOS";
    repo   = "nixpkgs";
    rev    = "adaa9d93488c7fc1fee020899cb08ce2f899c94b";
    sha256 = "0rkfxgmdammpq4xbsx7994bljqhzjjblpky2pqmkgn6x9s6adwrr";
  }}:

with import nixpkgs {};

let
  docs        = import ./nix/docs.nix { inherit nixpkgs; };
  benchmarks  = import ./nix/benchmarks.nix { inherit nixpkgs; };
  compilerPkg = if compiler != ""
                then pkgs.${compiler}
                else stdenv.cc;
  theStdenv   = if compilerPkg.isClang
                then clangStdenv
                else stdenv;
in

theStdenv.mkDerivation rec {
  name = "immer-env";
  buildInputs = [
    compilerPkg
    git
    cmake
    pkgconfig
    doxygen
    ninja
    gdb
    lldb
    ccache
    boost
    boehmgc
    benchmarks.c_rrb
    benchmarks.steady
    benchmarks.chunkedseq
    benchmarks.immutable_cpp
    benchmarks.hash_trie
    (python.withPackages (ps: [
      ps.sphinx
      docs.breathe
      docs.recommonmark
    ]))
  ];
  shellHook = ''
    export CC=${compilerPkg}/bin/cc
    export CXX=${compilerPkg}/bin/c++
  '';
}
