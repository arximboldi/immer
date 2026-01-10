{
  flake ? import ./nix/flake-compat.nix { },
  pkgs ? import flake.inputs.nixpkgs { },
}:

let
  inherit (pkgs) lib;
  inherit
    (import (pkgs.fetchFromGitHub {
      owner = "hercules-ci";
      repo = "gitignore.nix";
      rev = "80463148cd97eebacf80ba68cf0043598f0d7438";
      sha256 = "1l34rmh4lf4w8a1r8vsvkmg32l1chl0p593fl12r28xx83vn150v";
    }) { })
    gitignoreSource
    ;

  nixFilter = name: type: !(lib.hasSuffix ".nix" name);
  srcFilter =
    src:
    lib.cleanSourceWith {
      filter = nixFilter;
      src = gitignoreSource src;
    };

in
pkgs.callPackage ./nix/immer.nix { sources = srcFilter ./.; }
