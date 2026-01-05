{
  pkgs,
  lib,
  stdenv,
  cmake,
  ninja,
  xxHash,
  nlohmann_json,
  boehmgc,
  boost,
  fmt,
  catch2_3,
  src ? ../.,
  withTests ? false,
  withExamples ? false,
  withBenchmarks ? false,
  withDocs ? false,
  withPersist ? false,
}:

let
  # maybe these should be moved outside, or perhaps be written as overlays
  benchmarks = pkgs.callPackage ./benchmarks.nix { };
  docs = pkgs.callPackage ./docs.nix { };
  cereal = pkgs.callPackage ./cereal.nix { };

in
stdenv.mkDerivation {
  name = "immer";
  version = "git";
  inherit src;

  buildInputs =
    lib.optionals withTests [
      boehmgc
      boost
      fmt
      catch2_3
    ]
    ++ lib.optionals withPersist [
      fmt
      cereal
      xxHash
      nlohmann_json
    ]
    ++ lib.optionals withBenchmarks [
      benchmarks.c_rrb
      benchmarks.steady
      benchmarks.chunkedseq
      benchmarks.immutable_cpp
      benchmarks.hash_trie
    ]
    ++ lib.optionals withDocs [
      docs.doxygen
      docs.python
    ];

  nativeBuildInputs = [
    cmake
    ninja
  ];

  # dontBuild = true;
  # dontUseCmakeBuildDir = false;
  # doCheck = true;

  cmakeFlags = [
    (lib.cmakeBool "immer_BUILD_TESTS" withTests)
    (lib.cmakeBool "immer_BUILD_PERSIST_TESTS" (withPersist && withTests))
    (lib.cmakeBool "immer_BUILD_EXAMPLES" withExamples)
    (lib.cmakeBool "immer_BUILD_BENCHMARKS" withBenchmarks)
    (lib.cmakeBool "immer_BUILD_DOCS" withDocs)
  ];

  checkPhase = ''ninja check'';

  meta = {
    homepage = "https://github.com/arximboldi/immer";
    description = "Postmodern immutable data structures for C++";
    license = lib.licenses.boost;
  };
}
