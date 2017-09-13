with import <nixpkgs> {};

{ compiler ? "" }:

let
  compiler_pkg       = if compiler != ""
                       then pkgs.${compiler}
                       else stdenv.cc;
  docs               = import ./nix/docs.nix;
  benchmarks         = import ./nix/benchmarks.nix;
  propagate_compiler = compiler_pkg.isClang != stdenv.cc.isClang;
  native_compiler    = compiler_pkg.isClang == stdenv.cc.isClang;
in

stdenv.mkDerivation rec {
  name = "immer-env";
  buildInputs = [
    git
    cmake
    pkgconfig
    doxygen
    ninja
    ccache
    boost
    boehmgc
    benchmarks.c_rrb
    benchmarks.steady
    benchmarks.chunkedseq
    benchmarks.immutable_cpp
    docs.sphinx_arximboldi
    docs.breathe_arximboldi
    docs.recommonmark
  ]
  ++ stdenv.lib.optionals compiler_pkg.isClang [
    llvm libcxx libcxxabi
  ];
  propagatedBuildInputs =
    stdenv.lib.optional propagate_compiler compiler_pkg;
  nativeBuildInputs =
    stdenv.lib.optional native_compiler compiler_pkg;
}
