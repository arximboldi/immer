{
  toolchain ? "",
  rev ? "08ef0f28e3a41424b92ba1d203de64257a9fca6a",
  sha256 ? "1mql1gp86bk6pfsrp0lcww6hw5civi6f8542d4nh356506jdxmcy",
  nixpkgs ?
    builtins.fetchTarball {
      name = "nixpkgs-${rev}";
      url = "https://github.com/nixos/nixpkgs/archive/${rev}.tar.gz";
      sha256 = sha256;
    },
  ...
} @ args: let
  # For the documentation tools we use an older Nixpkgs since the
  # newer versions seem to be not working great...
  pkgs = import nixpkgs (
    if args ? system
    then {inherit (args) system;}
    else {}
  );
  catch2_3 =
    if pkgs ? catch2_3
    then pkgs.catch2_3
    else
      pkgs.callPackage ./nix/catch2_3.nix {
        stdenv = tc.stdenv;
      };
  oldNixpkgsSrc = pkgs.fetchFromGitHub {
    owner = "NixOS";
    repo = "nixpkgs";
    rev = "d0d905668c010b65795b57afdf7f0360aac6245b";
    sha256 = "1kqxfmsik1s1jsmim20n5l4kq6wq8743h5h17igfxxbbwwqry88l";
  };
  oldNixpkgs = import oldNixpkgsSrc (
    if args ? system
    then {inherit (args) system;}
    else {}
  );
  docs = oldNixpkgs.callPackage ./nix/docs.nix {};
  benchmarks = pkgs.callPackage ./nix/benchmarks.nix {};
  tc =
    if toolchain == ""
    then {
      stdenv = pkgs.stdenv;
      cc = pkgs.stdenv.cc;
    }
    else if toolchain == "gnu-6"
    then {
      stdenv = pkgs.gcc6Stdenv;
      cc = pkgs.gcc6;
    }
    else if toolchain == "gnu-7"
    then {
      stdenv = pkgs.gcc7Stdenv;
      cc = pkgs.gcc7;
    }
    else if toolchain == "gnu-8"
    then {
      stdenv = pkgs.gcc8Stdenv;
      cc = pkgs.gcc8;
    }
    else if toolchain == "gnu-9"
    then {
      stdenv = pkgs.gcc9Stdenv;
      cc = pkgs.gcc9;
    }
    else if toolchain == "gnu-10"
    then {
      stdenv = pkgs.gcc10Stdenv;
      cc = pkgs.gcc10;
    }
    else if toolchain == "gnu-11"
    then {
      stdenv = pkgs.gcc11Stdenv;
      cc = pkgs.gcc11;
    }
    else if toolchain == "llvm-39"
    then {
      stdenv = pkgs.llvmPackages_39.libcxxStdenv;
      cc = pkgs.llvmPackages_39.libcxxClang;
    }
    else if toolchain == "llvm-4"
    then {
      stdenv = pkgs.llvmPackages_4.libcxxStdenv;
      cc = pkgs.llvmPackages_4.libcxxClang;
    }
    else if toolchain == "llvm-5"
    then {
      stdenv = pkgs.llvmPackages_5.libcxxStdenv;
      cc = pkgs.llvmPackages_5.libcxxClang;
    }
    else if toolchain == "llvm-6"
    then {
      stdenv = pkgs.llvmPackages_6.libcxxStdenv;
      cc = pkgs.llvmPackages_6.libcxxClang;
    }
    else if toolchain == "llvm-7"
    then {
      stdenv = pkgs.llvmPackages_7.libcxxStdenv;
      cc = pkgs.llvmPackages_7.libcxxClang;
    }
    else if toolchain == "llvm-8"
    then {
      stdenv = pkgs.llvmPackages_8.libcxxStdenv;
      cc = pkgs.llvmPackages_8.libcxxClang;
    }
    else if toolchain == "llvm-9"
    then {
      stdenv = pkgs.llvmPackages_9.stdenv;
      cc = pkgs.llvmPackages_9.clang;
    }
    else if toolchain == "llvm-10"
    then {
      stdenv = pkgs.llvmPackages_10.stdenv;
      cc = pkgs.llvmPackages_10.clang;
    }
    else if toolchain == "llvm-11"
    then {
      stdenv = pkgs.llvmPackages_11.stdenv;
      cc = pkgs.llvmPackages_11.clang;
    }
    else if toolchain == "llvm-12"
    then {
      stdenv = pkgs.llvmPackages_12.stdenv;
      cc = pkgs.llvmPackages_12.clang;
    }
    else if toolchain == "llvm-13"
    then {
      stdenv = pkgs.llvmPackages_13.stdenv;
      cc = pkgs.llvmPackages_13.clang;
    }
    else abort "unknown toolchain";
in
  tc.stdenv.mkDerivation rec {
    name = "immer-env";
    buildInputs = with pkgs;
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
        fmt
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
    hardeningDisable = ["fortify"];
  }
