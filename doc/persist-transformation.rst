Transformation
==============
.. _transformations-with-pools:

Suppose, we want to apply certain transforming functions to the
``immer`` containers inside a large document type.  The most
straightforward way would be to simply create new containers with the
new data, running the transforming function over each
element. However, this approach has some disadvantages:

- All new containers will be independent, no structural sharing will
  be preserved, leading to the same data being stored multiple times.

- The transformation would be applied more times than necessary when
  some of the data is shared. Example: one vector is built by
  appending elements to the other vector. Transforming shared elements
  multiple times could be unnecessary.


Basic example
~~~~~~~~~~~~~

Let's consider a simple case using the document from the
:ref:`first-example`. The desired transformation would be to multiply
each element of the ``immer::vector<int>`` by 10.

First, the document value would be created in the same way:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: intro/start-prepare-value
   :end-before:  intro/end-prepare-value

The next component we need is the :ref:`pools<pools>` of all the
containers from the value:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-get_output_pools
   :end-before:  end-get_output_pools

The ``get_output_pools`` function returns the output pools of all
``immer`` containers that would be serialized using pools, as
controlled by the policy. Here we use the default policy
``hana_struct_auto_policy`` which will use pools for all ``immer``
containers inside the document type which must be a ``hana::Struct``.

The other required component is the ``conversion_map``:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-conversion_map
   :end-before:  end-conversion_map

This is a ``hana::map`` that describes the desired transformations to
be applied. The key of the map is an ``immer`` container and the value
is the function to be applied to each element of the corresponding
container type. In this case, it will apply ``[](int val) { return
val * 10; }`` to each ``int`` of the ``vector_one`` type, we have two
of those in the ``document``.

Having these two parts, we can create new pools with the transformations:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-transformed_pools
   :end-before:  end-transformed_pools

At this point, we can start converting the ``immer`` containers and
create the transformed document value with them, ``new_value``:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-convert-containers
   :end-before:  end-convert-containers

In order to confirm that the structural sharing has been preserved
after applying the transformations, let's serialize the ``new_value``
and inspect the JSON:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-save-new_value
   :end-before:  end-save-new_value

And indeed, we can see in the JSON that the node ``[2, [10, 20]]`` is
reused in both vectors.


Converting types
~~~~~~~~~~~~~~~~

The transforming function can even return a different type. In the
following example, ``vector<int>`` is transformed into
``vector<std::string>``. The first two steps are the same as in the
previous example:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: intro/start-prepare-value
   :end-before:  intro/end-prepare-value

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-get_output_pools
   :end-before:  end-get_output_pools

Only this time the transforming function will convert an integer into
a string:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-conversion_map-string
   :end-before:  end-conversion_map-string

Then we convert the two vectors the same way as before:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-convert-vectors-of-strings
   :end-before:  end-convert-vectors-of-strings

And in order to confirm that the structural sharing has been
preserved, we can introduce a new document type with the two vectors
being ``vector<std::string>``.

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-document_str
   :end-before:  end-document_str

And serialize it with pools:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-save-new_value-str
   :end-before:  end-save-new_value-str

In the resulting JSON we can confirm that the node
``[2, ["_1_", "_2_"]]``
is reused for both vectors.

.. _transforming-hash-based-containers:

Hash-based containers
~~~~~~~~~

As it was shown, converting ``vectors`` is conceptually simple: the
transforming function is applied to each element of each node,
producing a new node with the transformed elements. When it comes to
the hash-based containers, that is `set <containers.html#set>`_, `map
<containers.html#map>`_ and `table <containers.html#table>`_, their
structure is defined by the used hash function, so defining the
transformation may become a bit more verbose.

In the following example, we'll start with a simple case of
transforming a map. For a map, only the hash of the key matters and we
will not modify the key yet. We will focus on transformations here and
not on the structural sharing within the document, so we will use the
``immer`` container itself as the document. Let's define the following
policy to indicate that we want to use pools only for our container:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-direct_container_policy
   :end-before:  end-direct_container_policy

By default, ``immer`` uses ``std::hash`` for the hash-based
containers. While this hash is sufficient for runtime use, it can't be
used for persistence, as noted in the `C++ reference
<https://en.cppreference.com/w/cpp/utility/hash>`_:

.. note::
   Hash functions are only required to produce the same result
   for the same input within a single execution of a program

We will use `xxHash <https://xxhash.com/>`_ as the hash for this
example. Let's create a small map like this:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-prepare-int-map
   :end-before:  end-prepare-int-map

Our goal is to convert the value from ``int`` to
``std::string``. Let's create the ``conversion_map`` like this:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-prepare-conversion_map
   :end-before:  end-prepare-conversion_map

A few important details to note:

- For maps, the transforming function accepts a pair of key and value,
  ``std::pair<std::string, int>``.

- The transforming function must also be able to handle an argument of
  type ``immer::persist::target_container_type_request``. This is
  achieved by using ``hana::overload`` to combine 2 lambdas into one
  callable value. When called with that argument, it should return an
  empty container of the type we're transforming to. This explicit
  approach is necessary because there is no reliable way to
  automatically determine the hash algorithm for the new
  container. Even though in this case the type of the key doesn't
  change (and so the hash remains the same), in other scenarios it
  might.

Once the ``conversion_map`` is defined, the actual conversion is done
as before:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-transform-map
   :end-before:  end-transform-map

And we can see that the original map's values have been transformed
into strings.


Transforming table's ID
------------------------

For this example, we'll transform the type of the ID of the table
element while keeping the hash of it the same.  This can occur, for
instance, if the member that serves as the ID gets wrapped in a
wrapper type.

To begin, let's define an item type for a table:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-old_item
   :end-before:  end-old_item

We can create a table value with some data and get the pools for it
like this:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-prepare-table-value
   :end-before:  end-prepare-table-value

In this example, we want to change the type of the ``old_item's`` ID,
which is ``std::string``, while keeping its hash the same.  Let's
define a wrapper for ``std::string`` and a ``new_item`` type like
this:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-new-table-types
   :end-before:  end-new-table-types

We're also changing the type for ``data`` from ``int`` to
``std::string`` but this change doesn't affect the structure of the
table. We define the ``xx_hash_value`` function for the ``new_id_t``
type to make it compatible with the
``immer::persist::xx_hash<new_id_t>`` hash. Then, we can define the
target ``new_table_t`` type and the ``conversion_map`` that describes
how to convert ``old_item`` into a ``new_item``.

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-prepare-new_table_t-type
   :end-before:  end-prepare-new_table_t-type

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-prepare-new_table_t-conversion_map
   :end-before:  end-prepare-new_table_t-conversion_map

Finally, to convert the ``value`` using the defined ``conversion_map``
we prepare the converted pools with ``transform_output_pool`` and use
``convert_container`` to convert the ``value`` table.

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-new_table_t-transformation
   :end-before:  end-new_table_t-transformation

We can see that the ``new_value`` table contains the transformed data
from the original ``value`` table.

.. _modifying-the-hash-of-the-id:

Modifying the hash of the ID
----------------------------

If the key of a map, the ID of a table item or an element of a set
changes its hash due to a transformation, the transformed hash-based
container can no longer keep its shape and it can't be efficiently
transformed by simply applying transformations to its nodes.

``immer::persist`` validates every container it creates from a
pool. If such a hash modification occurs, a runtime exception will be
thrown because it is not possible to detect this issue during
compile-time. Let's modify the previous example to also change the
data of the ID:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-prepare-new_table_t-broken-conversion_map
   :end-before:  end-prepare-new_table_t-broken-conversion_map

Now, if we attempt to convert the original table, a
``immer::persist::champ::hash_validation_failed_exception`` will be
thrown:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-new_table_t-broken-transformation
   :end-before:  end-new_table_t-broken-transformation

Even though such transformation can't be performed efficiently, on a
node level, we can still request these transformations to be
applied. This will run for each value of the original container,
creating a new independent container that doesn't use structural
sharing:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-prepare-new_table_t-new-hash-conversion_map
   :end-before:  end-prepare-new_table_t-new-hash-conversion_map

We can request for such container-level (as opposed to per-node level)
transformation to be performed by wrapping the desired new container
type ``new_table_t`` in a
``immer::persist::incompatible_hash_wrapper`` as the result of the
``immer::persist::target_container_type_request`` call.

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-new_table_t-new-hash-transformation
   :end-before:  end-new_table_t-new-hash-transformation

We can see that the transformation has been applied, the keys have the ``_key`` suffix.

.. note::
   While different transformed containers will not have structural
   sharing, transforming the same container multiple times will reuse
   previously transformed data.  In other words, transformation will
   be cached on the container level but not on the nodes level.

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-returned-transformed-container-is-the-same
   :end-before:  end-returned-transformed-container-is-the-same


.. _transforming-nested-containers:

Transforming nested containers
------------------------------

Let's consider a scenario where a transforming function works on an
item within an ``immer`` container and also needs to transform another
``immer`` container. We define the types as follows:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-define-nested-types
   :end-before:  end-define-nested-types

The important property here is that we have a ``vector<nested_t>``
where ``nested_t`` contains ``vector<int>``, so we can say a
``vector`` is nested inside another ``vector``. We can prepare a value
with some structural sharing and then serialize it:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-prepare-nested-value
   :end-before:  end-prepare-nested-value

The resulting JSON looks like:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-nested-value-json
   :end-before:  end-nested-value-json

Looking at the JSON we can confirm that the node ``[2, [1, 2]]`` is reused.
Let's define a ``conversion_map`` like this:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-nested-conversion_map
   :end-before:  end-nested-conversion_map

The transforming function for ``vector_one`` is simple as it
transforms an ``int`` into a ``std::string``. However, the function
for the ``vector<nested_t>`` is more involved. When we attempt to
transform one item of that vector, ``nested_t``, we realize that
inside that function we have a ``vector<int>`` to deal with. This
brings us back to the problems described in the beginning of the
:ref:`transformations-with-pools` section. To solve this issue,
``immer::persist`` provides an optional second argument to the
transforming function, a function called ``convert_container``. This
function can be called with two arguments: the desired container type
and the ``immer`` container to convert. This allows us to access the
``conversion_map`` we're defining. This transformation will be
performed using pools and will preserve structural sharing as
expected.

Having defined the ``conversion_map``, we apply it in the usual way
and get the ``new_value``:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-apply-nested-transformations
   :end-before:  end-apply-nested-transformations

We can verify that the ``new_value`` has the expected content:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-verify-nested-value
   :end-before:  end-verify-nested-value

And we can serialize it again to confirm that the structural sharing
of the nested vectors has been preserved:

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: start-verify-structural-sharing-of-nested
   :end-before:  end-verify-structural-sharing-of-nested

We can see that the ``[2, ["_1_", "_2_"]]`` node is still being reused
in the two vectors.
