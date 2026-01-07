{
  description = "Immutable data structures";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        lib = nixpkgs.lib;
        toolchains = [
          "gnu"
          "gnu-15"
          "gnu-14"
          "gnu-13"
          "llvm"
          "llvm-20"
          "llvm-19"
          "llvm-18"
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

            immer = pkgs.callPackage nix/immer.nix { sources = ./.; };

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
