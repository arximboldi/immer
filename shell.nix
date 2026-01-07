{
  toolchain ? "",
  flake ? import ./nix/flake-compat.nix { },
  pkgs ? import flake.inputs.nixpkgs { },
  ...
}@args:
let
  lib = pkgs.lib;

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

  stdenv = tc.stdenv;
  cc = tc.cc;

  # use Catch2 v3
  catch2_3 = pkgs.catch2_3.override {
    inherit stdenv;
  };

  immer = pkgs.callPackage ./nix/immer.nix {
    inherit catch2_3 stdenv;
    fmt = (pkgs.fmt.override { stdenv = tc.stdenv; });
    withTests = true;
    withExamples = true;
    withPersist = true;
    withBenchmarks = stdenv.isLinux;
    withDocs = stdenv.isLinux;
  };

in
pkgs.mkShell.override { inherit stdenv; } {
  inputsFrom = [ immer ];
  packages = (
    with pkgs; [ ccache ] ++ lib.optionals cc.isClang [ lldb ] ++ lib.optionals cc.isGNU [ gdb ]
  );
  hardeningDisable = [ "fortify" ];
}
