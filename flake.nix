{
  description = "Immutable data structures";

  inputs = {
    nixpkgs.url = github:NixOS/nixpkgs/nixpkgs-unstable;
    flake-utils.url = "github:numtide/flake-utils";
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    flake-compat,
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
      clang-format = pkgs.runCommand "clang-format" {} ''
        mkdir -p $out/bin
        ln -s ${pkgs.llvmPackages_16.clang-unwrapped}/bin/clang-format $out/bin/
      '';
      our_llvm = pkgs.llvmPackages_14;

      immer-derivation = {
        stdenv,
        cmake,
        lib,
        catch2,
      }:
        stdenv.mkDerivation rec {
          pname = "immer";
          version = "v0.0";
          src = ./.;
          nativeBuildInputs = [cmake];
          buildInputs = [catch2];
          dontUseCmakeBuildDir = true;
          meta = {
            homepage = "https://sinusoid.al/immer";
            description = "Immutable data structures";
            license = lib.licenses.boost;
          };
        };

      immer = pkgs.callPackage immer-derivation {stdenv = our_llvm.stdenv;};
    in rec {
      devShell = pkgs.mkShell.override {stdenv = our_llvm.stdenv;} {
        packages = with pkgs; [
          clang-format
          cmake-format
          cmake
          ninja
          catch2_3

          # for the llvm-symbolizer binary, that allows to show stacks in ASAN and LeakSanitizer.
          our_llvm.bintools-unwrapped
        ];
      };

      defaultPackage = immer;
    });
}
