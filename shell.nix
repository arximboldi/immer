{
  toolchain ? "",
  flake ? import ./nix/flake-compat.nix { },
  pkgs ? import flake.inputs.nixpkgs { },
  ...
}@args:
let
  # For the documentation tools we use an older Nixpkgs since the
  # newer versions seem to be not working great...
  oldNixpkgs = import (pkgs.fetchFromGitHub {
    owner = "NixOS";
    repo = "nixpkgs";
    rev = "d0d905668c010b65795b57afdf7f0360aac6245b";
    sha256 = "1kqxfmsik1s1jsmim20n5l4kq6wq8743h5h17igfxxbbwwqry88l";
  }) { system = pkgs.system; };

  docs = oldNixpkgs.callPackage ./nix/docs.nix { };
  benchmarks = pkgs.callPackage ./nix/benchmarks.nix { };
  arximboldi-cereal = pkgs.callPackage ./nix/cereal.nix { };

  # toolchain is a string of the form "gnu-X" or "llvm-Y", empty for default
  tc =
    if toolchain == "" then
      {
        stdenv = pkgs.stdenv;
        cc = pkgs.stdenv.cc;
      }
    else if toolchain == "gnu" then
      {
        stdenv = pkgs.gccStdenv;
        cc = pkgs.gcc_latest;
      }
    else if toolchain == "llvm" then
      {
        stdenv = pkgs.llvmPackages_latest.libcxxStdenv;
        cc = pkgs.llvmPackages_latest.clang;
      }
    else
      let
        parts = builtins.split "-" toolchain;
        compiler = builtins.elemAt parts 0;
        version = builtins.elemAt parts 2;
      in
      if compiler == "gnu" then
        {
          stdenv = pkgs."gcc${version}Stdenv";
          cc = pkgs."gcc${version}";
        }
      else if compiler == "llvm" then
        {
          stdenv = pkgs."llvmPackages_${version}".libcxxStdenv;
          cc = pkgs."llvmPackages_${version}".clang;
        }
      else
        abort "unknown toolchain";

  # use Catch2 v3
  catch2_3 = pkgs.callPackage ./nix/catch2_3.nix {
    stdenv = tc.stdenv;
  };

in
tc.stdenv.mkDerivation rec {
  name = "immer-env";
  buildInputs =
    with pkgs;
    [
      tc.cc
      git
      catch2_3
      cmake
      pkg-config
      ninja
      lldb
      boost
      boehmgc
      (fmt.override { stdenv = tc.stdenv; })
    ]
    ++
      # for immer::persist
      [
        arximboldi-cereal
        xxHash
        nlohmann_json
      ]
    ++ lib.optionals stdenv.isLinux [
      gdb
      ccache
      valgrind
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
