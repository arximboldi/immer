{ compiler ? "",
  nixpkgs ? (import <nixpkgs> {}).fetchFromGitHub {
    owner = "NixOS";
    repo = "nixpkgs";
    rev = "7db2916648ba8184bf619fc895e8d3fe2de84db3";
    sha256 = "0b9hkri53hiynawv0hzfnczcs00bm0zsbvw7hy4s8yrxafr5ak1g";
  }}:

with import nixpkgs {};

let
  compiler_pkg       = if compiler != ""
                       then pkgs.${compiler}
                       else stdenv.cc;
  docs               = import ./nix/docs.nix { inherit nixpkgs; };
  benchmarks         = import ./nix/benchmarks.nix { inherit nixpkgs; };
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
    (python.withPackages (ps: [
      docs.sphinx
      docs.breathe
      docs.recommonmark
    ]))
  ]
  ++ stdenv.lib.optionals compiler_pkg.isClang [
    llvm libcxx libcxxabi
  ];
  propagatedBuildInputs =
    stdenv.lib.optional propagate_compiler compiler_pkg;
  nativeBuildInputs =
    stdenv.lib.optional native_compiler compiler_pkg;
}
