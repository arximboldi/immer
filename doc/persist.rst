
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

.. _first-example:

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


Transformations with pools
--------------------------

Suppose, we want to apply certain transforming functions to the ``immer`` containers inside of a large document type.
The most straightforward way would be to simply create new containers with the new data, running the transforming
function over each element. However, this approach has some disadvantages:

- All new containers will be independent, no structural sharing will be preserved and the same data would be stored
  multiple times.
- The transformation would be applied more times than necessary when some of the data is shared. Example: one vector
  is built by appending elements to the other vector. Transforming shared elements multiple times could be
  unnecessary.

Let's look at a simple case using the document from the :ref:`first-example`. The desired transformation would be to
multiply each element of the ``immer::vector<int>`` by 10.

First, the document value would be created in the same way:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: intro/start-prepare-value
   :end-before:  intro/end-prepare-value

The next component we need is the pools of all the containers from the value:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-get_auto_pool
   :end-before:  end-get_auto_pool

The ``get_auto_pool`` function returns the output pools of all ``immer`` containers that would be serialized using
pools, as controlled by the policy. Here we use the default policy ``hana_struct_auto_policy`` which will use pools for
all ``immer`` containers inside of the document type which must be a ``hana::Struct``.

The other required component is the ``conversion_map``:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-conversion_map
   :end-before:  end-conversion_map

This is a ``hana::map`` that describes the desired transformations to be applied. The key of the map is an ``immer``
container and the value is the function to be applied to each element of the corresponding container type. In this case,
it will apply ``[](int val) { return val * 10; }`` to each ``int`` of the ``vector_one`` type, we have two of those in
the ``document``.

Having these two parts, we can create the new pools with the transformations:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-transformed_pools
   :end-before:  end-transformed_pools

At this point, we can start converting the ``immer`` containers and create the transformed document value with them,
``new_value``:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-convert-containers
   :end-before:  end-convert-containers

In order to confirm that the structural sharing has been preserved after applying the transformations, let's serialize
the ``new_value`` and inspect the JSON:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-save-new_value
   :end-before:  end-save-new_value

And indeed, we can see in the JSON that the node ``{"key": 2, "value": [10, 20]}`` is reused in both vectors.


Transformation into a different type
------------------------------------

The transforming function can even return a different type. In the following example ``vector<int>`` is transformed into ``vector<std::string>``.
The first two steps are the same as in the previous example:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: intro/start-prepare-value
   :end-before:  intro/end-prepare-value

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-get_auto_pool
   :end-before:  end-get_auto_pool

Only this time the transforming function will convert an integer into a string:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-conversion_map-string
   :end-before:  end-conversion_map-string

Then we convert the two vectors the same way as before:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-convert-vectors-of-strings
   :end-before:  end-convert-vectors-of-strings

And in order to confirm that the structural sharing has been preserved, we can introduce a new document type with
the two vectors being ``vector<std::string>``.

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-document_str
   :end-before:  end-document_str

And serialize it with pools:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-save-new_value-str
   :end-before:  end-save-new_value-str

In the resulting JSON we can confirm that the node ``{"key": 2, "value": ["_1_", "_2_"]}`` is reused for both vectors.


Transforming hash-based containers
----------------------------------

As it was shown, converting ``vectors`` is conceptually simple: the transforming function is applied to each element of
each node, producing a new node with the transformed elements. When it comes to the hash-based containers, that is `set
<containers.html#set>`_, `map <containers.html#map>`_ and `table <containers.html#table>`_, their structure is defined
by the used hash function, so defining the transformation may become a bit more verbose.

In the following example we'll start with a simple case transforming a map. For a map, only the hash of the key matters and we will not modify the key yet.
We will focus on transformations here and not on the structural sharing within the document, so we will use the ``immer`` container itself as the document.
Let's define the following policy to say that we want to use pools only for our container:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-direct_container_policy
   :end-before:  end-direct_container_policy

By default, ``immer`` uses ``std::hash`` for the hash-based containers. While sufficient for the runtime use, this hash can't be used for persistence, as
the `C++ reference <https://en.cppreference.com/w/cpp/utility/hash>`_ notes:

.. note::
   Hash functions are only required to produce the same result for the same input within a single execution of a program

We will use `xxHash <https://xxhash.com/>`_ as the hash for this example. Let's create a small map like this:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-prepare-int-map
   :end-before:  end-prepare-int-map

Our goal is to convert the value from ``int`` to ``std::string``. Let's create the ``conversion_map`` like this:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-prepare-conversion_map
   :end-before:  end-prepare-conversion_map

A few important details to note:

- For maps, the transforming function accepts a pair of key and value, ``std::pair<std::string, int>``.
- The transforming function must also support being called with an argument of type
  ``immer::persist::target_container_type_request``, we achieve it here by using ``hana::overload`` to tie 2 lambdas
  into one callable value. When called with that argument, it should return an empty container of the type we're
  transforming to. It has to be explicit like this since there is no good way to automatically determine the hash
  algorithm for the new container. Even though in this case the type of the key doesn't change (and so the hash stays
  the same), in other scenarios it might.

Once the ``conversion_map`` is defined, the actual conversion is done as before:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-transform-map
   :end-before:  end-transform-map

And we can see that the original map's values have been transformed into strings.


Transforming table's ID
------------------------

For this example, we'll transform the type of the table element's ID but we'll keep the hash of it the same.
It may happen, for instance, if the member that serves as the ID gets wrapped into a wrapper type.

Let's start by defining an item type for a table:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-old_item
   :end-before:  end-old_item

We can create a table value with some data and get the pools for it like this:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-prepare-table-value
   :end-before:  end-prepare-table-value

In this example, we want to change the type of the ``old_item's`` ID, which is ``std::string``, while keeping its hash the same.
Let's define a wrapper for ``std::string`` and a ``new_item`` type like this:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-new-table-types
   :end-before:  end-new-table-types

We're also changing the type for ``data`` from ``int`` to ``std::string`` but this doesn't affect the structure of the table.
We define the ``xx_hash_value`` function for the ``new_id_t`` type, that way the type becomes compatible with the ``immer::persist::xx_hash<new_id_t>`` hash.
Now we can define the target ``new_table_t`` type and the ``conversion_map`` describing how to convert ``old_item`` into a ``new_item``.

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-prepare-new_table_t-type
   :end-before:  end-prepare-new_table_t-type

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-prepare-new_table_t-conversion_map
   :end-before:  end-prepare-new_table_t-conversion_map

Finally, to convert the ``value`` with the defined ``conversion_map`` we prepare the converted pools with
``transform_output_pool`` and use ``convert_container`` to convert the ``value`` table.

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-new_table_t-transformation
   :end-before:  end-new_table_t-transformation

We can see that the ``new_value`` table contains the transformed data from the original ``value`` table.


Modifying the hash of the ID
----------------------------

If a map's key or a table item's ID or a set's element changes its hash as the result of the transformation, the transformed hash-based container can no longer
keep its shape, can't be efficiently transformed just by applying transformations to its nodes.

``immer::persist`` validates every container it creates from a pool. In case of this hash modification, a runtime exception will be thrown as it is not possible to detect
it during compile-time. Let's modify the previous example to also change the ID's data:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-prepare-new_table_t-broken-conversion_map
   :end-before:  end-prepare-new_table_t-broken-conversion_map

Now, if we attempt to convert the original table, a ``immer::persist::champ::hash_validation_failed_exception`` will be thrown:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-new_table_t-broken-transformation
   :end-before:  end-new_table_t-broken-transformation

Even though such transformation can't be performed efficiently, on a node level, we can still request these transformations to be applied and they will run for each
value of the original container, creating a new independent container that doesn't use structural sharing:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-prepare-new_table_t-new-hash-conversion_map
   :end-before:  end-prepare-new_table_t-new-hash-conversion_map

We can request for such container-level (as opposed to per-node level) transformation to be performed by wrapping the desired new container type ``new_table_t`` into
a ``immer::persist::incompatible_hash_wrapper`` as the result of the ``immer::persist::target_container_type_request`` call.

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-new_table_t-new-hash-transformation
   :end-before:  end-new_table_t-new-hash-transformation

We can see that the transformation has been applied, the keys have the ``_key`` suffix.


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
