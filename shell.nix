{ compiler ? "",
  nixpkgs ? (import <nixpkgs> {}).fetchFromGitHub {
    owner  = "NixOS";
    repo   = "nixpkgs";
    rev    = "d0d905668c010b65795b57afdf7f0360aac6245b";
    sha256 = "1kqxfmsik1s1jsmim20n5l4kq6wq8743h5h17igfxxbbwwqry88l";
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
      ps.sphinx
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
