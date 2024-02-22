{
  description = "Immutable data structures";

  inputs = {
    nixpkgs.url = github:NixOS/nixpkgs/nixpkgs-unstable;
    flake-utils.url = "github:numtide/flake-utils";
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
    gitignore = {
      url = "github:hercules-ci/gitignore.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    pre-commit-hooks = {
      url = "github:cachix/pre-commit-hooks.nix";
      inputs = {
        flake-compat.follows = "flake-compat";
        flake-utils.follows = "flake-utils";
        gitignore.follows = "gitignore";
        nixpkgs.follows = "nixpkgs";
      };
    };
    arximboldi-cereal-src = {
      url = "github:arximboldi/cereal";
      flake = false;
    };
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    flake-compat,
    pre-commit-hooks,
    gitignore,
    arximboldi-cereal-src,
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};

      withLLVM = drv:
        if pkgs.stdenv.isLinux
        # Use LLVM for Linux to build fuzzers
        then drv.override {stdenv = pkgs.llvmPackages_latest.stdenv;}
        # macOS uses LLVM by default
        else drv;

      arximboldi-cereal = pkgs.callPackage ./nix/cereal.nix {inherit arximboldi-cereal-src;};

      archive-inputs = with pkgs; [
        spdlog
        arximboldi-cereal
        xxHash
        nlohmann_json
      ];
    in {
      checks =
        {
          pre-commit-check = pre-commit-hooks.lib.${system}.run {
            src = ./.;
            hooks = {
              alejandra.enable = true;
            };
          };

          inherit (self.packages.${system}) unit-tests fuzzers-debug;
        }
        // pkgs.lib.optionalAttrs pkgs.stdenv.isLinux {
          unit-tests-valgrind = self.packages.${system}.unit-tests.overrideAttrs (prev: {
            nativeBuildInputs = with pkgs; prev.nativeBuildInputs ++ [valgrind];
            name = "immer-unit-tests-valgrind";
            ninjaFlags = ["tests"];
            checkPhase = ''
              ctest -D ExperimentalMemCheck
              valgrind --quiet --error-exitcode=99 --leak-check=full --errors-for-leak-kinds=all \
                --suppressions=./test/extra/archive/valgrind.supp \
                ./test/extra/archive/archive-tests
            '';
          });
        };

      devShells.default = (withLLVM pkgs.mkShell) {
        NIX_HARDENING_ENABLE = "";
        inputsFrom = [
          (import ./shell.nix {
            inherit system nixpkgs;
          })
        ];

        packages = with pkgs;
          [
            # for the llvm-symbolizer binary, that allows to show stacks in ASAN and LeakSanitizer.
            llvmPackages_latest.bintools-unwrapped
            cmake-format
            alejandra
            just
            fzf
            starship
          ]
          ++ archive-inputs;

        shellHook =
          self.checks.${system}.pre-commit-check.shellHook
          + "\n"
          + ''
            alias j=just
            eval "$(starship init bash)"
          '';
      };

      packages = {
        immer = let
          inherit (gitignore.lib) gitignoreSource;
          nixFilter = name: type: !(pkgs.lib.hasSuffix ".nix" name);
          srcFilter = src:
            pkgs.lib.cleanSourceWith {
              filter = nixFilter;
              src = gitignoreSource src;
            };
        in
          pkgs.callPackage nix/immer.nix {src = srcFilter ./.;};

        default = self.packages.${system}.immer;

        fuzzers-debug = (withLLVM self.packages.${system}.immer).overrideAttrs (prev: {
          name = "immer-fuzzers";
          # Fuzzers should be built with minimal dependencies to use them easily with OSS-Fuzz
          buildInputs = with pkgs; [boehmgc];
          nativeBuildInputs = with pkgs; [cmake ninja];
          dontBuild = false;
          dontStrip = true;
          # fuzzers target is not built by default
          ninjaFlags = ["fuzzers"];
          cmakeBuildType = "Debug";
          cmakeFlags = [
            "-DENABLE_ASAN=ON"
            "-Dimmer_BUILD_TESTS=OFF"
            "-Dimmer_INSTALL_FUZZERS=ON"
          ];
        });

        unit-tests = (withLLVM self.packages.${system}.immer).overrideAttrs (prev: {
          name = "immer-unit-tests";
          buildInputs = with pkgs; [catch2_3 boehmgc boost fmt] ++ archive-inputs;
          nativeBuildInputs = with pkgs; [cmake ninja];
          dontBuild = false;
          doCheck = true;
          # Building fuzzers but not running them, just to ensure they compile
          ninjaFlags = ["fuzzers tests"];
          checkPhase = ''
            ninja test
          '';
          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Debug"
            "-Dimmer_BUILD_TESTS=ON"
            "-Dimmer_BUILD_ARCHIVE_TESTS=ON"
            "-Dimmer_BUILD_EXAMPLES=OFF"
            "-DCXX_STANDARD=20"
          ];
        });
      };
    });
}
