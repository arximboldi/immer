{
  description = "Immutable data structures";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
    gitignore = {
      url = "github:hercules-ci/gitignore.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    arximboldi-cereal-src = {
      url = "github:arximboldi/cereal";
      flake = false;
    };
    docs-nixpkgs = {
      url = "github:NixOS/nixpkgs/d0d905668c010b65795b57afdf7f0360aac6245b";
      flake = false;
    };
  };

  outputs =
    {
      self,
      nixpkgs,
      docs-nixpkgs,
      flake-utils,
      flake-compat,
      gitignore,
      arximboldi-cereal-src,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        lib = nixpkgs.lib;
        toolchains = [
          "gnu"
          "gnu-10"
          "gnu-11"
          "gnu-12"
          "gnu-13"
          "llvm"
          "llvm-11"
          "llvm-12"
          "llvm-13"
          "llvm-14"
        ];
      in
      {
        devShells = (
          with self.devShells.${system};
          {
            default = pkgs.callPackage ./shell.nix { };
          }
          // lib.attrsets.genAttrs toolchains (toolchain: default.override { inherit toolchain; })
        );

        checks = self.packages.${system};

        packages = (
          with self.packages.${system};
          {
            default = immer;

            immer = pkgs.callPackage nix/immer.nix { src = ./.; };

            tests = immer.override {
              withTests = true;
              withExamples = true;
              withPersist = true;
            };
            tests-debug = tests.override {
              withDebug = true;
              withASan = true;
              withLSan = true;
            };

            fuzzers = immer.override {
              withFuzzers = true;
              withASan = true;
            };
            fuzzers-debug = fuzzers.override {
              withDebug = true;
            };
          }
          // pkgs.lib.optionalAttrs pkgs.stdenv.isLinux {
            tests-valgrind = tests.override {
              withDebug = true;
              withValgrind = true;
            };

            benchmarks = immer.override { withBenchmarks = true; };
          }
        );
      }
    );
}
