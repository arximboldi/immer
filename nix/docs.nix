{
  stdenv,
  fetchFromGitHub,
  lib,
}:

let
  # For the documentation tools we use an older Nixpkgs since the
  # newer versions seem to be not working great...
  pkgs = import (fetchFromGitHub {
    owner = "NixOS";
    repo = "nixpkgs";
    rev = "d0d905668c010b65795b57afdf7f0360aac6245b";
    sha256 = "1kqxfmsik1s1jsmim20n5l4kq6wq8743h5h17igfxxbbwwqry88l";
  }) { inherit (stdenv.hostPlatform) system; };

in
rec {
  breathe =
    with pkgs.python27Packages;
    buildPythonPackage rec {
      version = "git-arximboldi-${commit}";
      pname = "breathe";
      name = "${pname}-${version}";
      commit = "5074aecb5ad37bb70f50216eaa01d03a375801ec";
      src = pkgs.fetchFromGitHub {
        owner = "arximboldi";
        repo = "breathe";
        rev = commit;
        sha256 = "10kkh3wb0ggyxx1a7x50aklhhw0cq269g3jddf2gb3pv9gpbj7sa";
      };
      propagatedBuildInputs = [
        docutils
        sphinx
      ];
      meta = with lib; {
        homepage = "https://github.com/michaeljones/breathe";
        license = licenses.bsd3;
        description = "Sphinx Doxygen renderer";
        inherit (sphinx.meta) platforms;
      };
    };

  recommonmark = pkgs.python27Packages.recommonmark;

  doxygen = pkgs.doxygen;

  python = pkgs.python.withPackages (ps: [
    ps.sphinx
    breathe
    recommonmark
  ]);
}
