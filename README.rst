
.. image:: https://travis-ci.org/arximboldi/immer.svg?branch=master
   :target: https://travis-ci.org/arximboldi/immer
   :alt: Travis Badge

.. image:: https://codecov.io/gh/arximboldi/immer/branch/master/graph/badge.svg
   :target: https://codecov.io/gh/arximboldi/immer
   :alt: CodeCov Badge

.. raw:: html

    <a href="https://github.com/arximboldi/immer"><img style="position: absolute; top: 0; right: 0; border: 0;" src="https://camo.githubusercontent.com/38ef81f8aca64bb9a64448d0d70f1308ef5341ab/68747470733a2f2f73332e616d617a6f6e6177732e636f6d2f6769746875622f726962626f6e732f666f726b6d655f72696768745f6461726b626c75655f3132313632312e706e67" alt="Fork me on GitHub" data-canonical-src="https://s3.amazonaws.com/github/ribbons/forkme_right_darkblue_121621.png"></a>

.. raw:: html

   <img style="display:block; width:100%;" src="https://cdn.rawgit.com/arximboldi/immer/doc/doc/_static/logo.svg" alt="Logotype"/>

**immer** is a library of persistent_ and immutable_ data structures
written in C++.

.. _persistent: https://en.wikipedia.org/wiki/Persistent_data_structure
.. _immutable:  https://en.wikipedia.org/wiki/Immutable_object

- **Persistent** means that when you modify the data structure, the
  old value is preserved.

- **Immutable** means that all *manipulation* methods are ``const``.

  An object is never modified in place but *a new value* is returned
  instead.  Since the old value is still there and it will never
  change, the new value can transparently keep references to common
  parts of it.  This property is called *structural sharing*.

----

**Read the docs**
  https://sinusoid.es/immer
**Check the code**
  https://github.com/arximboldi/immer
**Talks**
  `[Video (7min)] <https://www.youtube.com/watch?v=9nupb1SNo3Q>`_
  `[Slides] <https://sinusoid.es/talks/meetingcpp16>`_
  *The tragedy of the value based architecture* (MeetingC++ 2016)


.. attention::
   This library is *work in progress* and its API is not complete
   enough yet.

   A **stable release** is scheduled for early Q2 2017.  This release
   will use a more liberal license, which is yet to be determined.

.. admonition:: Sponsorship
   :class: tip

   This library is so far a non-remunerated full time job and months
   of research and development are invested in it.  To ensure it's
   long term sustainability, we will be launching a sponsoring program
   in early 2017.

   **Should your organization be interested in supporting it,
   contact:** raskolnikov@gnu.org.

.. include:index/end

Example
-------

.. github does not support the ``literalinclude`` directive.  This
   example is copy pasted form ``example/vector/intro.cpp``

.. code-block:: c++

   #include <immer/vector.hpp>
   int main()
   {
       const auto v0 = immer::vector<int>{};
       const auto v1 = v0.push_back(13);
       assert(v0.size() == 0 && v1.size() == 1 && v1[0] == 13);

       const auto v2 = v1.set(0, 42);
       assert(v1[0] == 13 && v2[0] == 42);
   }

Why?
----

In the last few years, there has been a growing interest in immutable
data structures, motivated by the horizontal scaling of our processing
power and the ubiquity of highly interactive systems.  Languages like
Clojure_ and Scala_ provide them by default, and implementations
for JavaScript like Mori_ and Immutable.js_ are widely used,
specially in combination with modern UI frameworks like React_.

**Interactivity**
    Thanks to *persistence* and *structural sharing*, new values can
    be efficiently compared with old ones.  This enables simpler ways of
    *reasoning about change* that sit at the core of modern
    interactive systems programming paradigms like `reactive
    programming`_.

**Concurrency**
    Passing immutable data structures by value does not need to copy
    any data. In the absence of mutation, data can be safely read
    from multiple concurrent processes, and enable concurrency
    patterns like `share by communicating`_ efficiently.

**Parallelism**
   Some recent immutable data structures have interesting properties
   like :math:`O(log(n))` concatenation, which enable new kinds of
   `parallelization algorithms`_.

.. _clojure: http://clojure.org/reference/data_structures
.. _scala: http://docs.scala-lang.org/overviews/collections/overview.html

.. _mori: https://swannodette.github.io/mori/
.. _immutable.js: https://github.com/facebook/immutable-js
.. _react: https://facebook.github.io/react/

.. _reactive programming: https://en.wikipedia.org/wiki/Reactive_programming
.. _share by communicating: https://blog.golang.org/share-memory-by-communicating
.. _parallelization algorithms: http://docs.scala-lang.org/overviews/parallel-collections/overview.html

Goals
-----

Idiomatic
    This library doesn't pretend that it is written in Haskell.  It
    leverages features from recent standards to provide an API that is
    both efficient and natural for a C++ developer.

Performant
    You use C++ because you need this.  *Immer* implements state of
    the art data structures with efficient cache utilization and have
    been proven production ready in other languages.  It also includes
    our own improvements over that are only possible because of the
    C++'s ability to abstract over memory layout.  We monitor the
    performance impact of every change by collecting `benchmark
    results`_ directly from CI.

.. _benchmark results: https://public.sinusoid.es/misc/immer/reports/

Customizable
    We leverage templates and `policy-based design`_ to build
    data-structures that can be adapted to work efficiently for
    various purposes and architectures, for example, by choosing among
    various :doc:`memory management strategies<memory>`.  This turns
    *immer* into a good foundation to provide immutable data
    structures to higher level languages with a C runtime, like
    Python_ or Guile_.

.. _python: https://www.python.org/
.. _guile: https://www.gnu.org/software/guile/
.. _policy-based design: https://en.wikipedia.org/wiki/Policy-based_design

Dependencies
------------

This library is written in **C++14** and a compliant compiler is
necessary.  It is `continuously tested`_ with Clang 3.8 and GCC 6, but
it might work with other compilers and versions.

No external library is necessary and there are no other requirements.

.. _continuously tested: https://travis-ci.org/arximboldi/immer

.. note:: Some optional modules do have other dependencies, but this
          is noted in their respective documentation pages.

Usage
-----

This is a **header only** library.  Just make sure that the project
root is in your include path.

Development
-----------

One may generate a development project using `CMake`_::

    mkdir build && cd build
    cmake ..

To automatically fetch and build all depedencies required to build and
run the *tests* and *benchmarks* run::

    make deps

From then on, one may build and run all tests by doing::

    make check

In order to build and run all benchmarks when running ``make check``,
run ``cmake`` again with the option ``-DCHECK_BENCHMARKS=1``.  The
results of running the benchmarks will be saved to a folder
``reports/`` in the project root.

.. _cmake: https://cmake.org/

License
-------

This software is licensed under the `GPLv3 license`_.

.. admonition:: License header

    Copyright (C) 2016 Juan Pedro Bolivar Puente

    This file is part of immer.

    immer is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    immer is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with immer.  If not, see <http://www.gnu.org/licenses/>.

.. _gplv3 license: https://www.gnu.org/licenses/gpl-3.0.en.html

.. image:: https://www.gnu.org/graphics/gplv3-127x51.png
   :alt: GPL3 logo
   :target: https://www.gnu.org/licenses/gpl-3.0.en.html
