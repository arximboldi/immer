
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

**Without immer-persist**

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: intro/start-no-persist
   :end-before:  intro/end-no-persist

**With immer-persist**

.. literalinclude:: ../test/extra/persist/test_for_docs.cpp
   :language: c++
   :start-after: intro/start-with-persist
   :end-before:  intro/end-with-persist


API Overview
------------

.. doxygengroup:: persist-api
   :project: immer
   :content-only:
