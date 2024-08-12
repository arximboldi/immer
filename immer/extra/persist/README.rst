
Persist
===============

This library allows to preserve structural sharing of immer containers while serializing and deserializing them.


Motivation: serialization
----------

Structural sharing allows immer containers to be efficient. In runtime, two distinct containers can be operated on independently but internally they share nodes and
use memory efficiently in that way. But when such containers are serialized in a simple direct way, for example, as lists, this sharing is lost: they become truly
independent, same data is stored multiple times on disk and later, when it is read from disk, in memory.

This library operates on the internal structure of immer containers: allowing it to be serialized and deserialized (and also transformed). That allows for more efficient
storage (especially, in case when a lot of nodes are reused) and, even more importantly, for preserving structural sharing after deserializing the containers.


Motivation: transformation
----------

Imagine this scenario: an application has a document type that uses an immer container internally in multiple places, for example, a vector of strings. Some of these vectors
would be completely identical, some would have just a few elements different (stored in an undo history, for example). And we want to run a transformation function
over these vectors.

A direct approach would be to take each vector and create a new vector applying the transformation function for each element. But after this, all the structural sharing
of the original containers would be lost: we will have multiple independent vectors without any structural sharing.

This library allows to apply the transformation function directly on the nodes which allows to preserve structural sharing. Additionally, it doesn't matter how many times
a node is reused, the transformation needs to be performed only once.


First example
-------------

For this example, we'll use a `document` type that contains two immer vectors.

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: intro/start-types
   :end-before:  intro/end-types

Let's say we have two vectors ``v1`` and ``v2``, where ``v2`` is derived from ``v1`` so that it shares data with it:

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

As you can see, ``ints`` and ``ints2`` contain the full linearization of each vector.
The structural sharing between these two data structures is not represented in its
serialized form. However, with ``immer-persist`` we can serialize it with:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: intro/start-serialize-with-persist
   :end-before:  intro/end-serialize-with-persist

Which generates some JSON like this:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: include:intro/start-persist-json
   :end-before:  include:intro/end-persist-json

As you can see, the value is serialized with every ``immer`` container replaced by an identifier.
This identifier is a key into a pool, which is serialized just after.

A pool represents a *set* of ``immer`` containers of a given type. For example, we may have a pool that contains all
``immer::vector<int>`` of our document. You can think of it as a little database of ``immer`` containers. When
serializing the pool, the internal structure of all those ``immer`` containers is written, preserving the structural
sharing between those containers. The nodes of the trees that implement the ``immer`` containers are represented
directly in the JSON and, because we are representing all the containers as a whole, those nodes that are referenced in
multiple trees can be stored only once. That same structure is preserved when reading the pool back from disk and
reconstructing the vectors (and other containers) from it, thus allowing us to preserve the structural sharing across
sessions.

.. note::
   Currently, ``immer-persist`` makes a distiction between pools used for saving containers (*output* pools) and for loading containers (*input* pools),
   similar to ``cereal`` with its ``InputArchive`` and ``OutputArchive`` distiction.

Currently, ``immer-persist`` focuses on JSON as the serialization format and uses the ``cereal`` library internally. In principle, other formats
and serialization libraries could be supported in the future.


Custom policy
----------

We can use policy to control the names of the pools for each container.

For this example, let's define a new document type ``doc_2``. It will also contain another type ``extra_data`` with a ``vector`` of ``strings`` in it.
To demonstrate the responsibilities of the policy, the ``doc_2`` type will not be a ``boost::hana::Struct`` and will not allow for a compile-time reflection.

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: include:start-doc_2-type
   :end-before:  include:end-doc_2-type

We define the ``doc_2_policy`` as following:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: include:start-doc_2_policy
   :end-before:  include:end-doc_2_policy

The ``get_pool_types`` function returns the types of containers that should be serialized with pools, in this case it's both ``vector`` of ``ints`` and ``strings``.
The ``save`` and ``load`` functions control the name of the document node, in this case it is ``doc2_value``.
And the ``get_pool_name`` overloaded functions supply the name of the pool for each corresponding ``immer`` container.
We can create and serialize a value of ``doc_2`` like this:

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

This example also demonstrates a case where the main document type ``doc_2`` contains another type ``extra_data`` with a ``vector``.
As you can see in the resulting JSON, nested types are also serialized with pools: ``"extra": {"comments": 1}``. Only the ID of the ``comments`` ``vector``
is serialized instead of its content.


Policy
------

.. doxygengroup:: Persist-policy
   :project: immer
   :content-only:


API Overview
------------

.. doxygengroup:: persist-api
   :project: immer
   :content-only:
