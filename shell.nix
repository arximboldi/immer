with import <nixpkgs> {};

{ compiler ? "gcc6" }:

let
  docs = import ./nix/docs.nix;
  benchmarks = import ./nix/benchmarks.nix;
  compiler_pkg = pkgs.${compiler};
  native_compiler = compiler_pkg.isClang == stdenv.cc.isClang;
in

stdenv.mkDerivation rec {
  name = "immer-env";
  buildInputs = [
    cmake
    pkgconfig
    doxygen
    ninja
    boost
    boehmgc
    benchmarks.c_rrb
    benchmarks.steady
    benchmarks.chunkedseq
    benchmarks.immutable_cpp
    docs.sphinx_arximboldi
    docs.breathe_arximboldi
    docs.recommonmark
  ];
  propagatedBuildInputs = stdenv.lib.optional (!native_compiler) compiler_pkg;
  nativeBuildInputs = stdenv.lib.optional native_compiler compiler_pkg;
}
