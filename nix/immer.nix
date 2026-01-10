{
  pkgs,
  lib,
  stdenv,
  clangStdenv,
  cmake,
  ninja,
  pkg-config,
  xxHash,
  nlohmann_json,
  boehmgc,
  boost,
  fmt,
  catch2_3,
  valgrind,
  sources ? ../.,
  withTests ? false,
  withExamples ? false,
  withBenchmarks ? false,
  withDocs ? false,
  withPersist ? false,
  withFuzzers ? false,
  withDebug ? false,
  withASan ? false,
  withUBSan ? false,
  withLSan ? false,
  withValgrind ? false,
}:

let
  # maybe these should be moved outside, or perhaps be written as overlays
  benchmarks = pkgs.callPackage ./benchmarks.nix { };
  docs = pkgs.callPackage ./docs.nix { };
  cereal = pkgs.callPackage ./cereal.nix { };

  # building fuzzers require clang
  withSanitizer = withASan || withUBSan || withLSan;
  preferClang = withFuzzers || withSanitizer;
  theStdenv = if preferClang && !stdenv.cc.isClang then clangStdenv else stdenv;
in
theStdenv.mkDerivation {
  name = builtins.concatStringsSep "-" (
    [ "immer" ]
    ++ lib.optionals withDebug [ "debug" ]
    ++ lib.optionals withPersist [ "persist" ]
    ++ lib.optionals withTests [ "tests" ]
    ++ lib.optionals withBenchmarks [ "benchmarks" ]
    ++ lib.optionals withFuzzers [ "fuzzers" ]
    ++ lib.optionals withASan [ "asan" ]
    ++ lib.optionals withUBSan [ "ubsan" ]
    ++ lib.optionals withLSan [ "lsan" ]
    ++ lib.optionals withValgrind [ "valgrind" ]
  );
  version = "git";

  src = sources;

  hardeningDisable = lib.optionals withSanitizer [ "fortify" ];

  buildInputs =
    lib.optionals (withExamples || withFuzzers || withTests || withBenchmarks) [
      boehmgc
    ]
    ++ lib.optionals (withTests || withBenchmarks) [
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
      valgrind
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
    pkg-config
  ];

  # on macos "build" conflicts with the BUILD file
  cmakeBuildDir = "project";

  doCheck = withTests;

  ctestFlags = lib.optionals withValgrind [
    "-D"
    "ExperimentalMemCheck"
  ];

  cmakeBuildType =
    if withDebug then
      "Debug"
    else if withSanitizer then
      "RelWithDebInfo"
    else
      "Release";

  cmakeFlags = [
    (lib.cmakeBool "ENABLE_ASAN" withASan)
    (lib.cmakeBool "ENABLE_UBSAN" withUBSan)
    (lib.cmakeBool "ENABLE_LSAN" withLSan)
    (lib.cmakeBool "immer_BUILD_TESTS" (withTests || withBenchmarks))
    (lib.cmakeBool "immer_BUILD_PERSIST_TESTS" (withPersist && withTests))
    (lib.cmakeBool "immer_BUILD_EXAMPLES" withExamples)
    (lib.cmakeBool "immer_BUILD_DOCS" withDocs)
    (lib.cmakeBool "immer_INSTALL_FUZZERS" withFuzzers)
    (lib.cmakeBool "CHECK_BENCHMARKS" withBenchmarks)
    (lib.cmakeBool "DISABLE_FREE_LIST" (withSanitizer || withValgrind))
  ];

  ninjaFlags =
    lib.optionals withFuzzers [ "fuzzers" ]
    ++ lib.optionals withTests [ "tests" ]
    ++ lib.optionals withExamples [ "examples" ]
    ++ lib.optionals withBenchmarks [ "benchmarks" ];

  meta = {
    homepage = "https://github.com/arximboldi/immer";
    description = "Postmodern immutable data structures for C++";
    license = lib.licenses.boost;
  };
}
