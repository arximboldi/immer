Introduction
===============

The ``immer::persist`` library persists persistent data structures,
allowing the preservation of structural sharing of ``immer`` containers
when serializing, deserializing or transforming the data.


.. warning:: This library is still experimental and its API may
   change in the future. The headers can be found in
   ``immer/extra/persist/...`` and the ``extra`` subpath will be
   removed once its interface stabilizes.

Dependencies
------------

In addition to the `dependencies <introduction.html#dependencies>`_ of
``immer``, this library makes use of **C++17**, `Boost.Hana
<https://boostorg.github.io/hana/>`_, `fmt <https://fmt.dev/>`_ and
`cereal <https://uscilab.github.io/cereal/>`_.

Why?
---------

Preserving structural sharing on disk
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

*Structural sharing* allows ``immer`` containers to be efficient. At
runtime, two distinct containers can be operated on independently but
internally they share nodes and use memory efficiently in that
way.

However when such containers are serialized in a trivial form, for
example, as JSON lists, this sharing is lost: they become truly
independent---the same data is stored multiple times on disk, and
later, when read back into memory, the program has lost the structural
sharing.

This library operates on the internal structure of ``immer``
containers: allowing it to be serialized, deserialized and
transformed. This enables more efficient storage, particularly when
many nodes are reused, and, even more importantly, preserving
structural sharing after deserializing the containers.


Transforming data with structural sharing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Consider this scenario where you have multiple
``immer::vector<std::string>``, where the various instances are
derived from one another. Some of these vectors would be completely
identical, while others would have just a few elements different. This
scenario is not uncommon, for example, when `implementing the undo
history of an application by preserving the previous
states <https://sinusoid.es/lager/modularity.html#genericity>`_.

The goal is to apply a transformation function to these vectors with
something like ``std::transform``.

A direct approach would be to take each vector and create a new vector
by applying the transformation function for each element. However,
after this process, all the structural sharing of the original
containers would be lost---the result would be multiple independent
vectors without any structural sharing, and the transformation may
have been applied unnecessarily multiple times to identical elements
that were previously shared.

This library enables the application of the transformation function
directly on the nodes, preserving structural sharing. Additionally,
regardless of how many times a node is reused, the transformation
needs to be performed only once.

.. _pools:
How?
------

To solve this problem, this library introduces the notion of a *pool*.

A **pool** represents a *set* of ``immer`` containers of a specific
type. For example, we may have a pool that contains all
``immer::vector<int>`` of our document. You can think of it as a small
database of ``immer`` containers. When serializing the pool, the
internal structure of all those ``immer`` containers is written as
a whole, preserving the structural sharing between those containers.

Note that for the most part, the user of the library is not concerned
with pools, as they are generated automatically from your
data structures.  However, you may become aware of them in the JSON
output or when transforming recursive data structures.
