Serialization
=========

Serializing your data structures using ``immer::persist`` allows you to
preserve the *structural sharing* across sessions of your application.

This has multiple practical use cases, like storing the undo history
or the clipboard of a complex application, or applying advanced
logging techniques.

The library serializes multiple containers together via the notion of
a :ref:`pool<pools>`.  These pools are produced automatically and
represent in the JSON the internal structure (trees) that implement
the Immer containers.

Example
-------
.. _first-example:

For this example, we'll use a ``document`` type that contains two
``immer`` vectors.

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: intro/start-types
   :end-before:  intro/end-types

Let's say we have two vectors ``v1`` and ``v2``, where ``v2`` is
derived from ``v1`` so that it shares data with it:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: intro/start-prepare-value
   :end-before:  intro/end-prepare-value

We can serialize the document using ``cereal`` with this:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: intro/start-serialize-with-cereal
   :end-before:  intro/end-serialize-with-cereal

Generating a JSON like this one:

.. code-block:: c++

   {"value0": {"ints": [1, 2, 3], "ints2": [1, 2, 3, 4, 5, 6]}}

As you can see, ``ints`` and ``ints2`` contain the full linearization
of each vector.  The structural sharing between these two data
structures is not represented in its serialized form.

Using pools
-----------

First, let's make the ``document`` struct compatible with
``boost::hana``. This way, the ``persist`` library can automatically
determine what :ref:`pool<pools>` types are needed, and to name the
pools.

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: intro/start-adapt-document-for-hana
   :end-before:  intro/end-adapt-document-for-hana

Then using ``immer::persist`` we can serialize it with:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: intro/start-serialize-with-persist
   :end-before:  intro/end-serialize-with-persist

Which generates some JSON like this:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: include:intro/start-persist-json
   :end-before:  include:intro/end-persist-json

As you can see, the value is serialized with every ``immer`` container
replaced by an identifier.  This identifier is a key into a
:ref:`pool<pools>`, which is serialized just after.

.. note::
   Currently, ``immer-persist`` makes a distinction between
   pools used for saving containers (*output* pools) and for loading
   containers (*input* pools), similar to ``cereal`` with its
   ``InputArchive`` and ``OutputArchive`` distinction.

Currently, ``immer-persist`` focuses on JSON as the serialization
format and uses the ``cereal`` library internally. In principle, other
formats and serialization libraries could be supported in the future.
sharing across sessions.

You can see in the output that the nodes of the trees that make up the
``immer`` containers are directly represented in the JSON and, because
we are representing all the containers as a whole, those nodes that are
referenced in multiple trees can be stored only once. That same
structure is preserved when reading the pool back from disk and
reconstructing the vectors (and other containers) from it, thus allowing
us to preserve the structural sharing across sessions.

Custom policies
----------

We can use policy to control the naming of the pools for each container.

For this example, let's define a new document type ``doc_2``. It will
also contain another type ``extra_data`` with a ``vector`` of
``strings`` in it. To demonstrate the responsibilities of the policy,
the ``doc_2`` type will not be a ``boost::hana::Struct`` and will not
allow for compile-time reflection.

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: include:start-doc_2-type
   :end-before:  include:end-doc_2-type

We define the ``doc_2_policy`` as following:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: include:start-doc_2_policy
   :end-before:  include:end-doc_2_policy

The ``get_pool_types`` function returns the types of containers that
should be serialized with pools, in this case it's both ``vector`` of
``ints`` and ``strings``. The ``save`` and ``load`` functions control
the name of the document node, in this case it is ``doc2_value``. And
the ``get_pool_name`` overloaded functions supply the name of the pool
for each corresponding ``immer`` container. To create and serialize a
value of ``doc_2``, you can use the following approach:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: include:start-doc_2-cereal_save_with_pools
   :end-before:  include:end-doc_2-cereal_save_with_pools

The serialized JSON looks like this:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: include:start-doc_2-json
   :end-before:  include:end-doc_2-json

And it can also be loaded from JSON like this:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: include:start-doc_2-load
   :end-before:  include:end-doc_2-load

This example also demonstrates a scenario in which the main document
type ``doc_2`` contains another type ``extra_data`` with a
``vector``. As you can see in the resulting JSON, nested types are
also serialized with pools: ``"extra": {"comments": 1}``. Only the ID
of the ``comments`` ``vector`` is serialized instead of its content.
