with import <nixpkgs> {};

rec {
  sphinxcontrib_websupport = with python27Packages; buildPythonPackage rec {
    name = "sphinxcontrib-websupport-${version}";
    version = "1.0.1";
    src = pkgs.fetchurl {
      url = "mirror://pypi/s/sphinxcontrib-websupport/${name}.tar.gz";
      sha256 = "1f9f0wjpi9nhikbyaz6d19s7qvzdf1nq2g5dsh640fma4q9rd1bs";
    };
    meta = with stdenv.lib; {
      description = "Python API to easily integrate Sphinx documentation into your Web application";
      homepage = https://pypi.python.org/pypi/sphinxcontrib-websupport;
      license = licenses.bsd2;
    };
  };

  sphinx_arximboldi = with python27Packages; buildPythonPackage rec {
    name = "${pname}-${version}";
    pname = "Sphinx";
    version = "git-arximbolid-${commit}";
    commit = "b03fb3889adbd865070b27dabd84479888af7099";
    src = fetchFromGitHub {
      owner = "arximboldi";
      repo = "sphinx";
      rev = commit;
      sha256 = "1lgch4xxbfv4k6ibgkd4jfdpvzqk1pjjj4wwgs6cmr5089g4d3za";
    };
    LC_ALL = "en_US.UTF-8";
    buildInputs = [
      pytest
      simplejson
      mock
      pkgs.glibcLocales
      html5lib
    ];
    doCheck = false;
    propagatedBuildInputs = [
      docutils
      jinja2
      pygments
      alabaster
      Babel
      snowballstemmer
      sqlalchemy
      whoosh
      imagesize
      requests
      typing
      six
      sphinxcontrib_websupport
    ];
    # https://github.com/NixOS/nixpkgs/issues/22501
    # Do not run `python sphinx-build arguments` but `sphinx-build arguments`.
    postPatch = ''
      substituteInPlace sphinx/make_mode.py --replace "sys.executable, " ""
    '';
    meta = with stdenv.lib; {
      description = "A tool that makes it easy to create intelligent and beautiful documentation for Python projects";
      homepage = http://sphinx.pocoo.org/;
      license = licenses.bsd3;
      platforms = platforms.all;
    };
  };

  breathe_arximboldi = with python27Packages; buildPythonPackage rec {
    version = "git-arximboldi-${commit}";
    pname = "breathe";
    name = "${pname}-${version}";
    commit = "5074aecb5ad37bb70f50216eaa01d03a375801ec";
    src = fetchFromGitHub {
      owner = "arximboldi";
      repo = "breathe";
      rev = commit;
      sha256 = "10kkh3wb0ggyxx1a7x50aklhhw0cq269g3jddf2gb3pv9gpbj7sa";
    };
    propagatedBuildInputs = [ docutils sphinx_arximboldi ];
    meta = with stdenv.lib; {
      homepage = https://github.com/michaeljones/breathe;
      license = licenses.bsd3;
      description = "Sphinx Doxygen renderer";
      inherit (sphinx_arximboldi.meta) platforms;
    };
  };

  recommonmark = python27Packages.recommonmark;
}
