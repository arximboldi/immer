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
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    flake-compat,
    pre-commit-hooks,
    gitignore,
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      checks = {
        pre-commit-check = pre-commit-hooks.lib.${system}.run {
          src = ./.;
          hooks = {
            alejandra.enable = true;
          };
        };

        unit-tests = self.packages.${system}.immer.overrideAttrs (prev: {
          buildInputs = with pkgs; [catch2 boehmgc boost fmt];
          nativeBuildInputs = with pkgs; [cmake ninja];
          dontBuild = false;
          doCheck = true;
          checkPhase = ''
            ninja tests
            ninja test
          '';
          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Debug"
            "-Dimmer_BUILD_TESTS=ON"
            "-Dimmer_BUILD_EXAMPLES=OFF"
          ];
        });
      };

      devShells.default = pkgs.mkShell {
        NIX_HARDENING_ENABLE = "";
        inputsFrom = [
          (import ./shell.nix {
            inherit system nixpkgs;
          })
        ];

        shellHook =
          self.checks.${system}.pre-commit-check.shellHook;
      };

      packages = {
        immer = with pkgs; let
          inherit (gitignore.lib) gitignoreSource;
          nixFilter = name: type: !(lib.hasSuffix ".nix" name);
          srcFilter = src:
            lib.cleanSourceWith {
              filter = nixFilter;
              src = gitignoreSource src;
            };
        in
          stdenv.mkDerivation {
            name = "immer-git";
            version = "git";
            src = srcFilter ./.;
            nativeBuildInputs = [cmake];
            dontBuild = true;
            dontUseCmakeBuildDir = true;
            cmakeFlags = [
              "-Dimmer_BUILD_TESTS=OFF"
              "-Dimmer_BUILD_EXAMPLES=OFF"
            ];
            meta = {
              homepage = "https://github.com/arximboldi/immer";
              description = "Postmodern immutable data structures for C++";
              license = lib.licenses.boost;
            };
          };

        default = self.packages.${system}.immer;
      };
    });
}
