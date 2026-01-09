{
  pkgs,
  toolchain ? "",
}:

#
# `toolchain` is a string of the form "gnu-X" or "llvm-Y", empty for default.
#
#   gnu  => gccStdenv
#   llvm => llvmPackages.libcxxStdenv
#

if toolchain == "" then
  pkgs.stdenv
else if toolchain == "gnu" then
  pkgs.gccStdenv
else if toolchain == "llvm" then
  pkgs.llvmPackages_latest.libcxxStdenv
else
  let
    parts = builtins.split "-" toolchain;
    compiler = builtins.elemAt parts 0;
    version = builtins.elemAt parts 2;
  in
  if compiler == "gnu" then
    pkgs."gcc${version}Stdenv"
  else if compiler == "llvm" then
    pkgs."llvmPackages_${version}".libcxxStdenv
  else
    abort "unknown toolchain"
