{
  flake ? import ./nix/flake-compat.nix { },
  pkgs ? import flake.inputs.nixpkgs { },
  toolchain ? "",
  ...
}@args:
let
  lib = pkgs.lib;

  toolchain-stdenv = pkgs.callPackage ./nix/choose-stdenv.nix {
    inherit toolchain;
  };
  stdenv = toolchain-stdenv;

  # use Catch2 v3
  catch2_3 = pkgs.catch2_3.override {
    inherit stdenv;
  };

  immer = pkgs.callPackage ./nix/immer.nix {
    inherit catch2_3 stdenv;
    fmt = pkgs.fmt.override { inherit stdenv; };
    withTests = true;
    withExamples = true;
    withPersist = true;
    withBenchmarks = stdenv.isLinux;
    withDocs = stdenv.isLinux;
    withValgrind = stdenv.isLinux;
  };

in
pkgs.mkShell.override { stdenv = toolchain-stdenv; } {
  inputsFrom = [ immer ];
  packages = (
    with pkgs;
    [ ccache ]
    ++ lib.optionals toolchain-stdenv.cc.isClang [ lldb ]
    ++ lib.optionals toolchain-stdenv.cc.isGNU [ gdb ]
  );
  hardeningDisable = [ "fortify" ];
}
