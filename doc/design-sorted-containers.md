# Sorted containers for immer — design document

- **Status**: draft proposal
- **Tracking issue**: [#105](https://github.com/arximboldi/immer/issues/105)
- **Scope**: `immer::sorted_map`, `immer::sorted_set` (and transients), plus the
  underlying persistent tree in `immer/detail/bts/`

## Summary

This document proposes adding *sorted* (comparison-based) associative
containers to immer.  The recommended underlying structure is a
**path-copied, reference-counted (a,b)-tree — an in-memory B-tree with
values stored only at the leaves** — with per-node fixed-capacity
storage, subtree-size augmentation, and transient support via the
existing edit-token machinery.  The document surveys the alternatives
(balanced binary trees, chunked binary trees à la PaC-trees, radix
tries, history-independent structures such as treaps and Merkle search
trees, finger trees, skip lists) and explains why a B-tree best matches
the library's established design principles: chunked nodes for cache
efficiency and low allocation traffic, policy-based memory management,
and transience through ownership tokens, as described in the ICFP'17
paper (*Persistence for the Masses*, [1]).

The proposal is intentionally conservative in its algorithmic core —
B-trees with path copying are the best-understood design in this space
(btrfs, LMDB, CouchDB, Rust's `im` crate) — while reserving room for
the more interesting recent literature (join-based set algebra,
compressed purely-functional trees, uniquely-represented trees) in
clearly delimited later phases.

## 1. Motivation

immer currently offers hashed associative containers (`map`, `set`,
`table`, backed by CHAMP) and sequences (`vector`, `flex_vector`,
`array`, backed by RRB trees and plain arrays).  There is no container
that maintains elements *sorted by a comparator*, which blocks a family
of use cases:

- **Keyed timelines.** The motivating request in #105 is a music
  editor storing timestamped note events, needing iteration in time
  order and neighbor queries.  This generalizes to time series,
  animation curves, undo metadata, order books, interval indices.
- **Range queries.** "All entries with `lo <= k < hi`" — the defining
  operation of sorted containers, unavailable on hash structures.
- **Ordered iteration and determinism.** CHAMP iterates in hash order:
  stable, but meaningless to humans and unstable across key-type or
  hash-function changes.  Sorted iteration gives reproducible,
  human-meaningful output for serialization, diffing, UI display.
- **Neighbor lookup.** `lower_bound`/`upper_bound`, first/last
  element, "closest event before t".
- **Ordered merges.** Merging two sorted structures with structural
  sharing (e.g. combining edits of a document model) can be
  asymptotically better than element-wise insertion.

Current workarounds are all unsatisfying:

- `immer::box<std::map<K, T>>` — O(n) copy on every update, no
  structural sharing whatsoever.
- Sorted `immer::flex_vector` + `std::lower_bound` — usable, but
  binary search over an RRB tree costs O(log² n) cache-unfriendly
  probes, insertion needs `take`/`push_back`/`concat` gymnastics, and
  the key discipline is manual.  (For fewer than ~100 elements, as in
  the original issue, this is fine — and notably, the container
  proposed here *degenerates to exactly this representation* for small
  sizes: a single sorted-array leaf.)
- `immer::map` + sort at the point of use — O(n log n) per read.

### Goals

Inherited from the library's charter and the ICFP'17 paper [1]:

1. **Idiomatic value semantics** — `const` methods returning new
   values; API style consistent with `immer::map`.
2. **Performance** competitive with `std::map` for point operations
   and much better for iteration, snapshots, and bulk/merge
   operations; performance-conscious defaults (chunking, few
   allocations).
3. **Structural sharing** with O(log n) path copying.
4. **Transients** for efficient batch mutation, using edit tokens; and
   `&&`-qualified in-place updates on unique owners
   (`use_transient_rvalues`).
5. **Policy-based memory management** — works with reference counting
   (thread-safe or not), Boehm GC, custom heaps, free lists.
6. **C++14**, header-only, no dependencies, MSVC support.
7. **Verifiability** — property-testable against a `std::map` oracle,
   fuzzable, exception-safety-testable with the existing `dada.hpp`
   fault-injection machinery.

### Non-goals

- `multimap`/`multiset` semantics (can be modeled as
  `sorted_map<K, vector<T>>`; revisit on demand).
- Concurrent in-place mutation (out of scope for the whole library).
- On-disk persistence (though the design should be serializable via
  `immer::persist` pools later).

## 2. Proposed API

### 2.1 Naming

**`immer::sorted_map` / `immer::sorted_set`** (headers
`immer/sorted_map.hpp`, `immer/sorted_set.hpp`, plus `_transient`
variants).  Rationale:

- `ordered_map` is ambiguous — in several ecosystems it means
  *insertion*-ordered (Python's `OrderedDict`, various C++ libs).
- `tree_map` (Scala) leaks the implementation.
- `sorted-map`/`sorted-set` is the Clojure naming, which immer's
  `map`/`set` already mirror; Rust's `im` uses `OrdMap`/`OrdSet`.

The internal namespace follows the `hamts`/`rbts` pattern:
`immer::detail::bts` (B-trees).

### 2.2 Class synopses

```c++
template <typename K,
          typename T,
          typename Compare       = std::less<K>,
          typename MemoryPolicy  = default_memory_policy,
          detail::bts::bits_t B  = default_bits,   // inner fanout: 2^B children max
          detail::bts::bits_t BL = default_bits>   // leaf capacity: 2^BL values max
class sorted_map
{
public:
    using key_type        = K;
    using mapped_type     = T;
    using value_type      = std::pair<K, T>;
    using size_type       = detail::bts::size_t;
    using difference_type = std::ptrdiff_t;
    using key_compare     = Compare;
    using reference       = const value_type&;
    using const_reference = const value_type&;

    using iterator        = detail::bts::btree_iterator<...>; // bidirectional, const
    using const_iterator  = iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    using transient_type  = sorted_map_transient<K, T, Compare, MemoryPolicy, B, BL>;
    using memory_policy_type = MemoryPolicy;

    sorted_map();                                       // O(1), no allocation
    sorted_map(std::initializer_list<value_type>);
    template <typename Iter, typename Sent>
    sorted_map(Iter first, Sent last);

    // --- queries ------------------------------------------------------
    iterator begin() const;                             // smallest key
    iterator end() const;
    reverse_iterator rbegin() const;
    reverse_iterator rend() const;

    size_type size() const;                             // O(1)
    bool empty() const;                                 // O(1)

    size_type count(const K&) const;                    // O(log n)
    const T* find(const K&) const;                      // O(log n), nullptr if absent
    const T& at(const K&) const;                        // throws std::out_of_range
    const T& operator[](const K&) const;                // default value if absent
    // + transparent-comparator overloads of the above, enabled when
    //   `Compare::is_transparent` is valid (mirrors Hash::is_transparent
    //   in immer::map); e.g. find(std::string_view) on sorted_map<std::string, T>

    // --- order queries (the new vocabulary) ---------------------------
    const value_type& front() const;                    // smallest entry, O(log n)
    const value_type& back() const;                     // largest entry,  O(log n)
    iterator lower_bound(const K&) const;               // first entry >= k
    iterator upper_bound(const K&) const;               // first entry >  k
    // + transparent overloads

    iterator nth(size_type i) const;                    // i-th smallest, O(log n)
    size_type rank(const K&) const;                     // #entries with key < k

    // --- functional updates -------------------------------------------
    sorted_map insert(value_type) const&;               // replaces on collision,
    sorted_map set(key_type, mapped_type) const&;       //   like immer::map
    template <typename Fn>
    sorted_map update(key_type, Fn&&) const&;
    template <typename Fn>
    sorted_map update_if_exists(key_type, Fn&&) const&;
    sorted_map erase(const K&) const&;
    // ... plus `&&`-qualified overloads of all of the above that mutate
    //     in place when uniquely owned (use_transient_rvalues), exactly
    //     as in map.hpp's *_move dispatch.

    // --- slicing (order-specific, all O(log n), sharing structure) ----
    sorted_map take(size_type n) const;                 // n smallest entries
    sorted_map drop(size_type n) const;                 // all but n smallest
    std::pair<sorted_map, sorted_map> split(const K&) const;
                                                        // {keys < k, keys >= k}

    // --- other ---------------------------------------------------------
    bool operator==(const sorted_map&) const;           // O(1) identity fast path
    bool operator!=(const sorted_map&) const;
    transient_type transient() const&;
    transient_type transient() &&;
    void* identity() const;

    // semi-private, for persist / algorithms
    const impl_t& impl() const;
};
```

`sorted_set<T, Compare, MemoryPolicy, B, BL>` is analogous, minus the
map-only members (`at`, `operator[]`, `update*`, `set`), with
`find(const T&) -> const T*` and the same order/slicing vocabulary.

The transients mirror `map_transient`/`set_transient`: mutating
`insert`/`set`/`update`/`erase`, read-only queries and iteration, and
`persistent()`.

Later (phase 4): `sorted_table<T, KeyFn, Compare, ...>`, the ordered
sibling of `immer::table`, reusing the same tree with a key-extraction
function.

### 2.3 Semantics and conventions

- **`insert` replaces.** As in `immer::map` (and unlike `std::map`),
  inserting an existing key replaces the association.  Divergence from
  the standard is already established library policy; documentation
  calls it out.
- **`find` returns a pointer**, not an iterator, keeping the
  documented immer rationale.  `lower_bound`/`upper_bound` *do* return
  iterators — ranges are the raison d'être of a sorted container, and
  an iterator materializes its descent stack so that subsequent
  traversal is O(1) amortized per step.
- **Key equivalence** is `!comp(a,b) && !comp(b,a)`.  `operator==` on
  containers compares sizes, then element sequences with `==`
  (identity fast path first; shared subtrees are skipped during the
  scan, see §4.6).
- **Comparator requirements.** `Compare` must be a strict weak
  ordering, *stateless and default-constructible*, mirroring how
  `Hash`/`Equal` are used in `map` (instantiated as `Compare{}` at use
  sites; they are never stored in nodes).  Support for stateful
  comparators is listed as an open question (§9).
- **Heterogeneous lookup** via `Compare::is_transparent`, so
  `std::less<>` works, mirroring the `Hash::is_transparent` overload
  sets in `map`/`set`.
- **Iterators** are `const`, bidirectional, and do not own the nodes
  they point into: the container (or transient) must outlive them —
  same contract as `champ_iterator`.  They are larger than `std::map`
  iterators (a bounded descent stack, ~100–200 bytes); documented.
- **Exception safety.** Persistent operations give the strong
  guarantee: new nodes are built off to the side (with
  `detail::uninitialized_copy`-style cleanup on unwind, as in
  rbts/hamts) and the root is published last.  Transient in-place
  paths require `std::is_nothrow_move_constructible<T>` (otherwise
  they fall back to the copying path), so a throwing element copy can
  never corrupt a node in place.
- **Thread safety.** As the rest of the library: immutable values are
  freely shareable; the default memory policy uses thread-safe
  refcounts; transients are single-owner.
- **`IMMER_NO_EXCEPTIONS`** honored via `IMMER_THROW` in `at`.

### 2.4 Complexities

With `b = 2^B` (inner fanout), `m = 2^BL` (leaf capacity); log bases
matter only in constants.

| Operation | Complexity | Notes |
|---|---|---|
| `find` / `count` / `at` / `[]` | O(log n) | ~⌈log_b n⌉ node visits, contiguous key search each |
| `insert` / `set` / `update` / `erase` (persistent) | O(log n) | copies one root-to-leaf path (≈ b·log_b n element copies) |
| same, transient with owned path | O(log n) search + O(m) memmove | 0 allocations in the common case |
| `begin` / `end` / iterator `++`/`--` | O(log n) / amortized O(1) | pointer bumps within leaf arrays |
| full iteration | O(n) | contiguous within leaves; `for_each_chunk` supported |
| `front` / `back` | O(log n) | |
| `lower_bound` / `upper_bound` | O(log n) | |
| `nth` / `rank` | O(log n) | via subtree-size augmentation |
| `take` / `drop` / `split` | O(log n) | result shares all untouched subtrees |
| concat of disjoint ranges (internal `join`) | O(log n) | classic (a,b)-tree concatenation |
| union / intersection / difference (phase 3) | O(p·log(q/p + 1)), p = min size | optimal; skips shared subtrees by identity |
| `==` | O(n) worst, O(1) identical, skips shared nodes | |
| `diff(a, b)` | effectively O(|diff|) between related versions | ordered callbacks |
| construction from sorted range | O(n) | packed nodes, 100% fill |

## 3. Choosing the data structure

### 3.1 Evaluation criteria

Derived from the project's goals and the techniques that made
`vector`/`map` competitive ([1], §2–4):

1. **Cache behavior** of search and iteration (pointer hops × cache
   lines per hop; sequential locality).
2. **Allocation and refcount traffic** per update.  This matters more
   in immer than in GC'd languages: the default policy performs
   *atomic* refcount operations, and each heap object costs
   allocator time plus a count.  Chunking amortizes both — the central
   lesson of the RRB/CHAMP work.
3. **Sharing granularity** — bytes newly allocated per update bound
   how cheaply many versions coexist.
4. **Transience fit** — can nodes be mutated in place when uniquely
   owned/edited?  (Needs spare capacity inside nodes and an
   ownership check; rules out exact-fit allocations.)
5. **Algorithmic breadth** — split/join for slicing, set algebra,
   ordered merge; augmentation for rank queries.
6. **Genericity** — arbitrary `Compare` over arbitrary `K`; no
   constraints on key representation.
7. **Guarantees** — worst-case (not amortized-only, not
   probabilistic-only) O(log n); no pathological adversarial inputs.
8. **Implementation fit** — reuse of `combine_standard_layout`,
   heaps/free lists, refcount and transience policies, iterator
   facades, testing patterns.
9. **Simplicity & verifiability** — a structure we can fuzz against
   `std::map` and reason about confidently.

### 3.2 Candidates

#### (a) Balanced binary search trees (red-black, AVL, weight-balanced)

The classic functional answer: Clojure's `sorted-map` (persistent
red-black), Scala's `TreeMap` (red-black), Haskell's `Data.Map`
(weight-balanced trees after Adams [6]), `data.avl` (AVL with rank
queries).  Join-based algorithms (Blelloch–Ferizovic–Sun [9], [11];
the PAM library [10]) give bulk set operations with optimal work over
*any* of these balancing schemes.

- ✅ Finest possible sharing granularity (one element per node);
  simplest persistent-update story; deep literature; deletion is the
  only classically fiddly part (for red-black trees famously so —
  Germane & Might [7]) but AVL/WBT deletions are tame.
- ❌ One heap object and one refcount *per element*: for a
  `sorted_map<int64_t, int64_t>` that is ≥48–56 bytes per 16-byte
  payload, an allocation per insert, ~1.44·log₂ n pointer
  dereferences (each a probable cache miss) per lookup, and a
  refcount touch per visited node during updates.  This is precisely
  the cost model immer's chunked designs exist to avoid; the ICFP'17
  benchmarks against pointer-per-element structures make the case
  empirically [1].

Verdict: **rejected as the primary structure**, but its *algorithms*
(join-based bulk operations, Adams-style divide and conquer) transfer
to any tree with `split`/`join`, including B-trees, and we adopt them
in phase 3.

#### (b) In-memory B-trees / (a,b)-trees with path copying — **recommended**

The mutable-world staple (`std::map`-killer `absl::btree_map`), and
equally established in immutable settings: Ohad Rodeh's
shadowed/cloned B-trees [15] are the foundation of btrfs; LMDB and
CouchDB are copy-on-write B+ trees; Rust's `im` crate implements
`OrdMap` as a persistent B-tree with 64-element chunks; Clojure-world
"hitchhiker trees" extend the same skeleton with write buffers.
Concatenation, split, and join on 2-3/(a,b)-trees are textbook
material (AHU [16], Huddleston–Mehlhorn [17]) and have modern
parallel/bulk formulations (Akhremtsev–Sanders [13], and just this
year, joinable parallel B-trees in the fork-join model [14]).

- ✅ Chunked nodes: one allocation covers ~2^BL elements; search
  touches ⌈log_b n⌉ nodes each holding keys *contiguously*; iteration
  is array traversal; per-element memory overhead is a fraction of a
  pointer; free-list-friendly fixed node sizes; fits
  `prefer_fewer_bigger_objects`.
- ✅ Transience: fixed-capacity nodes leave slack for in-place
  insertion; the ownership rule (`unique || owned by edit`) is
  exactly champ's `can_mutate`.
- ✅ Split/join/augmentation well understood → slicing, rank queries,
  set algebra.
- ✅ Deterministic worst-case O(log n); dead-simple invariant to
  fuzz-check (all leaves at equal depth, fill within [a,b]).
- ⚠️ Coarser sharing: a persistent point update copies a full path,
  ≈ b·log_b n *element slots* rather than log₂ n *elements*.  In
  bytes this is typically 2–4× a binary tree's path copy (see §5) —
  fast, contiguous memcpys — but it is the real trade-off, and it is
  tunable via B/BL.
- ⚠️ Refcount traffic per update is *not* lower than binary trees
  (copying an inner node increfs all its children — same phenomenon
  as champ), only *batched* into fewer, cheaper, cache-local loops.

Verdict: **chosen**.  Detailed design in §4.

#### (c) Chunked-leaf binary trees — PaC-trees / CPAM

The 2022 PLDI paper of Dhulipala–Blelloch–Gu–Sun [12] ("PaC-trees",
implemented in the CPAM library) addresses exactly the weakness of
(a): it keeps a *binary* weight-balanced top (so all the join-based
PAM algorithms and augmentation carry over verbatim) but stores
elements in compressed chunks ("blocks") at the leaves,
64–256 elements each.  It reports ~2.3–5× space reduction and large
speedups over PAM.

- ✅ Strong, recent prior art; simplest possible rebalancing code
  (Adams-style `join` only); parallel-ready.
- ⚠️ Lookup still walks a binary top: ~log₂(n/m) one-pointer hops
  (≈14 for 10⁶ elements with 64-wide leaves) vs ~3–4 fat-node visits
  for a B-tree; for a *sequential, lookup-heavy* general-purpose
  container the B-tree's constant factors win.
- ⚠️ Two node disciplines (binary inner + chunk leaves) is roughly
  the same implementation surface as a B-tree anyway.

Verdict: **runner-up**.  If during implementation the B-tree's
rebalancing complexity turns out to dominate, a PaC-tree is the
fallback with nearly identical leaf machinery.  Its algorithmic story
(join-based everything) is adopted regardless.

#### (d) Radix structures over byte-comparable keys — ART / PART

Adaptive Radix Trees [18] with path copying (PART [19], used by
Tegra) are persistent, ordered, and extremely fast for integer and
short-string keys, with O(key-length) operations independent of n.
Tries are also *canonical* (see (e)) and champ-adjacent in
implementation technique.

- ❌ Order is the lexicographic order of a **binary-comparable key
  encoding** — a fundamental mismatch with a generic `Compare`
  parameter.  Floats need bit-flipping encodings, signed integers
  bias, locale-aware string collation is out, `std::greater`,
  case-insensitive orders, or comparators over user types are
  inexpressible.
- ❌ Adaptive node kinds (Node4/16/48/256) multiply the persistence,
  transience, and layout code paths.

Verdict: **rejected for this proposal** — it answers a different
question ("fast ordered index for encodable keys").  A future
`immer::radix_map` could complement `sorted_map` the way `table`
complements `map`; noted in §8.

#### (e) History-independent (canonical) structures — treaps, zip trees, B-treaps, Merkle search trees, prolly trees

A treap keyed by `hash(key)` priorities — or its modern replacements,
zip trees [20] and zip-zip trees [21] — has a *unique shape per key
set*: equal containers become pointer-comparable after hash-consing,
and convergent across replicas regardless of operation order.
Chunked canonical variants exist: Golovin's B-treaps [22] (uniquely
represented B-trees, "very complex" by the author's own follow-up,
which proposed the simpler B-skip-list [23]); and, from the systems
world, content-defined chunking trees — **Merkle search trees** [24]
(the structure behind Bluesky's AT Protocol repositories) and **prolly
trees** (Noms/Dolt [25]).  These buy efficient replica diff/sync and
canonical hashing on top of ordered-map semantics.

- ✅ Genuinely modern and aligned with immer's value-oriented,
  diff-friendly worldview; the natural basis for a future
  content-addressed / synchronizable container (pairs beautifully
  with `immer::persist` and Merkle hashing).
- ❌ Bounds are expected-case only, with heavy-tailed node sizes
  (geometric chunk boundaries) or adversarial worst cases without
  keyed hashing; point updates rewrite expected-O(1) but
  variance-prone regions; equality-of-shape only pays off with a
  hash-consing or content-addressing layer immer does not (yet) have.
  Within a single process, immer's `identity()` + structural `diff`
  already deliver most of the practical benefit for *related*
  versions.

Verdict: **rejected for the default container; earmarked** as the
most interesting follow-up direction (§8) — e.g. a
`prolly_map` for sync-oriented applications.

#### (f) Briefly: finger trees, skip lists, sorted arrays, PMAs

- **Finger trees** [8]: elegant, but their amortized bounds lean on
  lazy evaluation (a poor fit for strict, refcounted C++ — a lesson
  the library has already internalized), constants are high, and
  allocation is per-2-3-node.  `flex_vector` already covers the
  sequence use cases.
- **Skip lists**: persistent variants exist but path copying must
  rebuild towers crossing the update point, randomization weakens
  guarantees, and their real strength (lock-free concurrent
  mutation) is irrelevant to immutable values.
- **Sorted `immer::array`/`flex_vector`**: O(n) updates; the right
  answer below ~100 elements — which the B-tree subsumes (a
  small container *is* a single sorted-array leaf).
- **Packed-memory arrays**: amortized rebuilds and in-place design
  don't survive path copying.

### 3.3 Decision matrix

| | binary BST (RB/AVL/WBT) | **(a,b)-tree, values at leaves** | PaC-tree | ART/PART | treap / zip tree | MST / prolly | finger tree |
|---|---|---|---|---|---|---|---|
| lookup cache behavior | poor (log₂ n hops) | **best (log_b n fat nodes)** | mid (binary top) | best for encodable keys | poor | good | poor |
| allocs per persistent update | log₂ n | **log_b n** | ~log₂(n/m) + chunk | O(key len) | log₂ n | expected O(1) chunks, high variance | O(log n) |
| bytes copied per update | **lowest** | b·log_b n slots (tunable) | chunk + spine | node-kind dependent | lowest | chunk(s) | mid |
| sharing granularity | **element** | chunk | chunk | node | element | chunk | node |
| split/join/set algebra | ✅ mature [9,10] | ✅ classic [13,14,16,17] | ✅ native [12] | ranges only | ✅ trivial join | merge-oriented | ✅ |
| rank/nth | via size augmentation | **cheap (size arrays)** | native (WBT) | needs augmentation | augmentation | no | via measure |
| generic `Compare` | ✅ | ✅ | ✅ | ❌ encoding-bound | ✅ (needs hash too) | ❌ needs hash/encoding | ✅ |
| worst-case bounds | ✅ | ✅ | ✅ | ✅ (in key len) | probabilistic | probabilistic | amortized/lazy |
| transience fit | weak (no slack) | **strong (fixed-capacity nodes)** | leaves only | mid | weak | mid | weak |
| fit with immer node infra | low | **high (rbts/champ patterns)** | high | mid | low | mid | low |
| canonical form | ❌ | ❌ | ❌ | ✅ | ✅ | ✅ | ❌ |

### 3.4 Continuity with the ICFP'17 techniques

Each load-bearing technique from the paper [1] and the existing
implementations transfers directly:

| Technique in immer today | In the proposed B-tree |
|---|---|
| Chunked nodes of 2^B slots (RRB, CHAMP) | inner fanout 2^B, leaf capacity 2^BL |
| Embedded, type-erased node layout via `combine_standard_layout_t` (refs + ownee + data in one allocation) | identical; two node kinds instead of champ's inner/collision |
| Fixed-capacity nodes sized for the free list (`heap_policy::optimized<max_sizeof>`, rbts) | identical; enables in-place transient growth |
| Memory policies: heap × refcount × lock × transience | the same `MemoryPolicy` parameter, unchanged |
| Transience via edit tokens; `can_mutate = unique ∥ ownee.can_mutate(e)` (champ `node.hpp`) | identical rule on both node kinds |
| `&&`-qualified update methods with `use_transient_rvalues` (`map.hpp` `*_move` dispatch) | same dispatch, same `move_t` idiom |
| GC support (`gc_transience_policy`, no refcounts) | unchanged; same trivial-destruction caveats under `gc_heap` |
| Relaxed invariants to make concatenation O(log n) (RRB) | the (a,b) fill interval is the ordered analogue: slack that makes `join`/`split` cheap |
| Iterators as explicit bounded stacks (`champ_iterator`) | same, bidirectional |
| `for_each_chunk` exposure for fold/vectorization | leaves are the chunks |

A size-annotated B+ tree is, structurally, a close cousin of the
relaxed RRB node (children + a size table) with separator keys added
— much of the mental model and testing discipline carries over.

## 4. Detailed design

### 4.1 Tree shape and node layout

An **(a,b)-tree with values only at the leaves** ("B+ tree without
sibling links" — links are incompatible with path copying, since they
would transitively invalidate every leaf on any update):

- **Leaf node**: `count`, plus an inline array of up to `2^BL` values
  (`std::pair<K, T>` for maps, `T` for sets).  All elements live in
  leaves, all leaves at the same depth.
- **Inner node**: `count` (number of children `c`), inline arrays of
  `c−1` **separator keys**, `c` child pointers, and `c` **cumulative
  subtree sizes** (the augmentation powering `nth`/`rank`/`take`/
  `drop`; see §4.4).
- **Invariant**: `a = 2^(B−1) ≤ c ≤ b = 2^B` for inner nodes (root:
  `2 ≤ c ≤ b`), `2^(BL−1) ≤ count ≤ 2^BL` for non-root leaves.
- **Separators are "loose"**: a separator must only *partition* its
  subtrees (`keys(child[i]) < sep[i] ≤ keys(child[i+1])`); it need not
  equal any live key.  Erasure therefore never has to fix separators,
  only the leaf and the counts along the path.

Layout and memory management reuse the champ/rbts machinery verbatim:

- One heap allocation per node, laid out with
  `combine_standard_layout_t<data_t, refs_t, ownee_t>`; works with
  over-aligned element types (covered by `test/alignment.cpp`
  patterns).
- Nodes are allocated at **fixed capacity** (`max_sizeof_leaf`,
  `max_sizeof_inner`) through `heap_policy::optimized<Size>`, as rbts
  does — free-list friendly, and the slack is what makes in-place
  transient insertion possible.
- No runtime kind tag needed on hot paths: all leaves sit at the same
  depth, so kind is known from the descent level (the container
  stores the tree height, like rbts stores `shift`).  Debug builds
  keep an `IMMER_TAGGED_NODE`-style tag and assertions.
- The **empty container** points at a statically allocated empty leaf
  (champ's `empty()` pattern), preserving the "default construction
  does not allocate" guarantee.
- **Height is bounded** by ⌈log_a(2^64)⌉ ≈ 16 at the default B; the
  iterator's stack and the recursion depth are small compile-time
  constants (champ's `max_depth` idiom).

For big keys or values, the standing library advice applies unchanged
and will be documented: wrap them in `immer::box` (with a transparent
comparator looking through the box) to keep nodes lean.

### 4.2 Core algorithms

- **Search**: descend by binary search over the separator array
  (`std::lower_bound`); binary search in the leaf.  Keys being
  contiguous makes this ~1–3 cache lines per level.  (A later,
  API-invisible optimization: branchless/linear scan for small
  arithmetic keys, as absl::btree does.)
- **Insert / update / erase (persistent)**: recursive descent that
  path-copies exactly the nodes on the root-to-leaf path
  (bottom-up rebuilding: the recursion returns the replacement child
  plus an optional split sibling / underflow signal).  Overflowing
  leaves split; underflowing nodes borrow from or merge with a
  neighbor **within the copied parent**; the root grows/shrinks the
  height by one when needed.  Each node on the path is copied at most
  once; nodes off the path are shared and incref'd.
  - The alternative — Rodeh-style *top-down preemptive*
    split/merge [15], which enables a loop instead of recursion and
    slightly simplifies transients at the cost of lower average fill
    — is noted as an implementation-time experiment; the choice is
    invisible in the API.
- **Transient operations** thread an `edit_t` and mutate any node for
  which `can_mutate(node, e)` holds (owned or unique), copying only
  the others — champ's exact discipline.  The common case (repeated
  inserts on a freshly-thawed transient) allocates only on splits:
  ~one allocation per 2^(BL−1) inserts, everything else being an
  in-leaf `memmove` plus size-counter bumps along an owned path.
- **`&&`-qualified persistent methods** dispatch to the `_mut` path
  with a null edit (mutate-if-unique) under `use_transient_rvalues`,
  copying `map.hpp`'s `*_move(move_t{}, ...)` pattern verbatim.
- **Bulk construction**: `from_sorted_range` builds packed (100%
  fill) leaves left-to-right in O(n); the iterator-pair constructor
  detects unsorted input and falls back to transient insertion
  (O(n log n)).
- **Iteration**: `btree_iterator` on `detail::iterator_facade`, a
  bounded stack of `(node*, offset)` plus a direct value cursor;
  bidirectional; `for_each_chunk` hands out whole leaf arrays.

### 4.3 Slicing and set algebra (split/join)

`join` (concatenate two trees with disjoint key ranges, O(Δheight +
1) rebalance) and `split` (cut at a key into two valid trees,
O(log n) with telescoping joins) are the classic (a,b)-tree
operations [16,17].  On top of them, following the join-based
framework [9,13]:

- `take`/`drop`/`split(k)` — O(log n), sharing every untouched
  subtree.  This makes "window over a time range" — the #105 use case
  — a cheap persistent operation, not a copy.
- `union` / `intersection` / `difference`, and for maps `merge(other,
  combine_fn)` — recursive split-and-join with two crucial fast
  paths: **pointer-identical subtrees** short-circuit entirely
  (merging two versions derived from a common ancestor costs
  ~O(diff)), and height-imbalanced cases degrade to insertion.  Work
  bound O(p·log(q/p+1)), the information-theoretic optimum.  These
  are sequential in phase 3; the same decomposition is
  embarrassingly parallel if the library ever wants it [13,14].

Naming of the set-algebra entry points (member `merge` vs free
`immer::set_union` — `union` being a keyword — vs operators `|`, `&`,
`-`) is deliberately left open (§9).

### 4.4 Rank augmentation

Inner nodes store **cumulative subtree sizes** (`c` machine words
next to the child array).  This costs ≈ `8·n/2^BL` bytes ≈ 2–3% of a
small-pair payload (§5) and buys:

- `nth(i)` / `rank(k)` in O(log n) — the `data.avl` feature set;
- `take(n)`/`drop(n)` by rank;
- O(log n) `std::distance` between iterators (later, via the stored
  stacks);
- balanced work division for any future parallel bulk operation.

Given the low cost and that adding it later would perturb the node
layout, it is included from phase 1.  (Storing them as cumulative
rather than per-child makes rank descent a binary search too.)

### 4.5 Memory policies, GC, platforms

Nothing new is required of `memory_policy`:

- **Refcounted policies** (default, unsafe, thread-unsafe): nodes
  embed `refs_t`; path copies incref children of copied nodes
  (batched, cache-local loops over the child array).
- **`gc_heap` / no refcounts**: `get_transience_policy` already
  selects `gc_transience_policy`; same element-destruction caveats as
  the existing containers.
- Two free-list size classes (leaf/inner) via
  `heap_policy::optimized<...>`, as rbts does with its node sizes.
- C++14, no dependencies, exceptions optional — all as today.

### 4.6 Equality and diff

- `operator==`: size check → identity check → dual leaf-stream scan
  that, whenever both cursors stand at the *same node*, skips the
  whole node (subtree skipping is positional, not structural, since
  B-trees are not canonical; the identity skip still collapses the
  common case of compare-after-small-edit).
- `immer::diff(a, b, differ)` (`algorithm.hpp`) gains a sorted
  overload with a stronger contract than the champ version:
  callbacks fire **in key order**, and the merge-walk skips
  pointer-identical subtrees, giving effectively O(|diff|) between
  related versions and O(n+m) between unrelated ones.  Ordered diff
  is a headline feature for the reactive/`lager` use cases (e.g.
  reconciling UI lists).

## 5. Cost model (back-of-envelope)

For `sorted_map<std::int64_t, std::int64_t>` (16-byte pairs), default
B = BL = 5 (32-way), 64-bit platform, steady-state random-insert fill
≈ 70%:

**Memory.**  Leaf ≈ 24 B header + 32×16 B slots ≈ 536 B holding ~22
elements → ~24 B/element; inner overhead adds ~1.5 B/element (keys +
children + sizes ≈ 776 B per ~490 elements); **total ≈ 25–26
B/element**, dropping to ~18 B/element bulk-loaded.  Reference points:
`std::map` ≈ 60–70 B/element (node header + padding + allocator
metadata); a refcounted persistent red-black tree ≈ 70–80 B/element;
raw data 16 B/element.

**Lookup, n = 10⁶.**  Height = 3 inner levels + leaf ≈ 4 node visits,
~2–3 cache lines each ≈ ~10 misses; vs ~20 dependent-load misses for
a binary tree.  n ≤ 32 is a single sorted array; n ≤ ~700 has height
2.

**Persistent insert, n = 10⁶.**  Copies 1 leaf + 3 inner nodes ≈ 2.8
KB of contiguous copies, **4 allocations**, ~100 child-refcount
increments; vs a persistent red-black tree's ~20 allocations / ~1 KB
/ ~40 increments.  Fewer, bigger, sequential — the profile the
allocator, free list and prefetcher like, and the knob (B, BL) trades
it against write amplification if a workload disagrees.

**Transient insert.**  Amortized: one leaf split per ~16 inserts;
otherwise an in-leaf memmove (~180 B average) and counter updates —
competitive with `absl::btree_map`, far ahead of `std::map`.

These numbers are estimates to be validated by the benchmark suite
(§6.3) before freezing the defaults; the write-amplification /
lookup-locality balance is exactly the B=16 vs 32 vs 64 question, and
the answer may differ for leaves (BL) and inner nodes (B).

## 6. Integration plan

### 6.1 Source layout

```
immer/
  sorted_map.hpp                 immer/sorted_map_transient.hpp
  sorted_set.hpp                 immer/sorted_set_transient.hpp
  detail/bts/
    bits.hpp                     # bits_t, size_t, counts, masks
    node.hpp                     # node kinds, layout, refs/ownee, make/copy/delete
    btree.hpp                    # the persistent tree ops (cf. hamts/champ.hpp)
    btree_iterator.hpp
```

The public classes are thin façades over `detail::bts::btree<T,
KeyFn, Compare, MemoryPolicy, B, BL>` exactly the way `map`/`set`/
`table` wrap `champ` (projections/`KeyFn` so map, set and a future
`sorted_table` share one implementation).

### 6.2 Testing

Mirror the established structure:

- `test/sorted_map/{generic.ipp, default.cpp, B3.cpp, gc.cpp, ...}`
  and likewise for `sorted_set` and both transients — instantiating
  the generic suite across memory policies and B values, as
  `test/map/*` does.
- **Oracle property tests** against `std::map` over randomized
  operation sequences (including `lower_bound`/slicing/rank), plus
  invariant checkers (uniform leaf depth, fill bounds, separator
  ordering, size-array consistency) run after every mutation in debug
  test builds.
- **Exception safety** with `test/dada.hpp` fault injection, as the
  vector/map suites do.
- **Fuzzers** in `extra/fuzzer/` (`sorted-map.cpp`, `sorted-map-gc.cpp`,
  ...) driving interleaved persistent/transient ops against the
  oracle, wired into `test/oss-fuzz` like the existing ones.
- Valgrind/ASAN/LSAN via the existing CI matrix; MSVC included.

### 6.3 Benchmarks

Extend `benchmark/` following `benchmark/set`'s generator pattern
(`unsigned`, `string-short`, `string-long`, `string-box`) with a
`sorted-map`/`sorted-set` family: point lookup, ordered iteration,
range scan, `lower_bound`, persistent insert/erase, transient build,
bulk `from_sorted`, and version-merge/diff.  Baselines: `std::map`/
`std::set`, sorted `std::vector` + binary search, `absl::btree_map`
(mutable state of the art), `immer::map` (price of ordering), and the
`immer::box<std::map>` workaround (motivation).

### 6.4 Documentation

Doxygen comments in the headers matching `map.hpp`'s style (including
the "why does `find` return a pointer" admonition, adapted);
`doc/containers.rst` entries; a short design note in
`doc/design.rst`; changelog.  This document lives as
`doc/design-sorted-containers.md` while the work is in flight.

### 6.5 `immer::persist`

Phase-4 work: pool types for bts nodes so sorted containers
serialize/deserialize with structural-sharing preservation like the
champ- and rbts-based containers.  The node format is
straightforward (counts, keys, child ids, sizes) but adds format
surface, so it is explicitly deferred.

## 7. Phasing

1. **Core** — `detail/bts` tree; `sorted_map`/`sorted_set` +
   transients; search, insert/set/update/erase, iterators,
   `lower_bound`/`upper_bound`, `front`/`back`, equality; tests,
   fuzzers, benchmarks; docs.  (Node layout includes size
   augmentation from day one.)
2. **Order extras** — `nth`/`rank`, `take`/`drop`/`split`,
   `from_sorted` bulk construction, `for_each_chunk` integration.
3. **Algebra** — `join`-based union/intersection/difference,
   `merge` with combiner, ordered `immer::diff`, iterator
   `distance`.
4. **Ecosystem** — `sorted_table`; `immer::persist` support; SIMD
   in-node search; (exploratory) parallel bulk ops; (exploratory)
   canonical/content-defined variant (§3.2e) for sync use cases.

## 8. Future directions deliberately out of scope

- `immer::radix_map` (persistent ART) for encodable keys — different
  trade-off point, complements rather than replaces `sorted_map`.
- Canonical trees (prolly/MST) + Merkle hashing for replication and
  content-addressed storage, likely as an `extra/` on top of
  `persist`.
- Write-optimized (buffered / hitchhiker-style) variants — relevant
  only if a disk-backed story emerges.
- Parallel bulk operations (the join-based decomposition is ready for
  it, but immer has no parallelism runtime dependency and should not
  grow one for this).

## 9. Open questions

1. **Naming**: `sorted_map`/`sorted_set` (proposed) vs `ordered_*`;
   bikeshed window closes at phase-1 merge.
   ANSWER: I was initially thinking of `ordered_` for symmetry with `unordered_` in the standard library, but you convinced me of `sorted_`

2. **Defaults for B/BL**: 2^5 to match the library's house style, but
   benchmarks may argue for asymmetric defaults (e.g. B=4, BL=5–6);
   also whether `sizeof(T)` should influence a recommended BL the way
   `map`'s docs steer big values toward `box`.
   ANSWER: Let's go for 2^5 for now and benchmark later.


3. **Stateful comparators**: require stateless (consistent with
   `Hash`/`Equal` today, proposed) or store the comparator in the
   container handle?  Storing it is cheap (EBO) but must be threaded
   into every algorithm and transients; nodes never store it either
   way.
   ANSWER: let's go for stateless for consistency with the rest of the library.

4. **Set-algebra spelling**: members (`merge`, `intersect`, ...) vs
   free functions (`immer::set_union` — `union` is a keyword) vs
   operators (`|`, `&`, `-`).
   ANSWER: members is good, merge / intersect is good. we could add
   operators later (we would have to think also about unordered
   collections).

5. **`erase(iterator)` / erase-by-range**: worth exposing once
   iterators carry a full descent stack (can avoid the re-search)?
   ANSWER: let's prioritize symmetrice. make a note for later if it could be a sigificant performance improvement, in which case we should implement everywhere.

6. **`equal_range`**: redundant for unique keys
   (`{lower_bound, lower_bound+0/1}`); include for std-compat or omit?
   Omit if redundant, write note.

7. **Top-down preemptive vs bottom-up rebalancing** (§4.2):
   implementation-time benchmark; affects average fill (~5–10%) and
   code shape, not the API.
   ANSWER: make whatever choice is best and write not to benchmark later.

8. **32-bit size counters** in the augmentation arrays (halves their
   footprint, caps subtrees at 2³² — the container `size_t` is
   already configurable per `detail::bts::size_t`)?
   ANSWER: sounds good.

## 10. References

[1] J. P. Bolívar Puente. *Persistence for the Masses: RRB-Vectors in
a Systems Language.* Proc. ACM Program. Lang. 1, ICFP, 2017.
https://public.sinusoid.es/misc/immer/immer-icfp17.pdf

[2] P. Bagwell, T. Rompf. *RRB-Trees: Efficient Immutable Vectors.*
EPFL Tech Report, 2011.

[3] N. Stucki, T. Rompf, V. Ureche, P. Bagwell. *RRB Vector: A
Practical General Purpose Immutable Sequence.* ICFP 2015.

[4] M. Steindorfer, J. Vinju. *Optimizing Hash-Array Mapped Tries for
Fast and Lean Immutable JVM Collections.* OOPSLA 2015. (CHAMP)

[5] C. Okasaki. *Purely Functional Data Structures.* Cambridge
University Press, 1998.

[6] S. Adams. *Efficient Sets — A Balancing Act.* JFP 3(4), 1993.
(Weight-balanced trees; basis of Haskell's `Data.Map`.)

[7] K. Germane, M. Might. *Deletion: The Curse of the Red-Black
Tree.* JFP 24(4), 2014.

[8] R. Hinze, R. Paterson. *Finger Trees: A Simple General-Purpose
Data Structure.* JFP 16(2), 2006.

[9] G. Blelloch, D. Ferizovic, Y. Sun. *Just Join for Parallel
Ordered Sets.* SPAA 2016.

[10] Y. Sun, D. Ferizovic, G. Blelloch. *PAM: Parallel Augmented
Maps.* PPoPP 2018.

[11] G. Blelloch, D. Ferizovic, Y. Sun. *Joinable Parallel Balanced
Binary Trees.* ACM TOPC 9(2), 2022.

[12] L. Dhulipala, G. Blelloch, Y. Gu, Y. Sun. *PaC-trees: Supporting
Parallel and Compressed Purely-Functional Collections.* PLDI 2022.
(CPAM: https://github.com/ParAlg/CPAM)

[13] Y. Akhremtsev, P. Sanders. *Fast Parallel Operations on Search
Trees.* HiPC 2016; arXiv:1510.05433. ((a,b)-tree join/split/bulk.)

[14] M. T. Goodrich et al. *Parallel Joinable B-Trees in the
Fork-Join I/O Model.* ISAAC 2025.

[15] O. Rodeh. *B-trees, Shadowing, and Clones.* ACM Transactions on
Storage 3(4), 2008. (Foundation of btrfs.)

[16] A. Aho, J. Hopcroft, J. Ullman. *The Design and Analysis of
Computer Algorithms.* Addison-Wesley, 1974. (2-3 tree split/concat.)

[17] S. Huddleston, K. Mehlhorn. *A New Data Structure for
Representing Sorted Lists.* Acta Informatica 17, 1982. ((a,b)-trees.)

[18] V. Leis, A. Kemper, T. Neumann. *The Adaptive Radix Tree: ARTful
Indexing for Main-Memory Databases.* ICDE 2013.

[19] A. Dave, J. Gonzalez, M. J. Franklin, I. Stoica. *Persistent
Adaptive Radix Trees: Efficient Fine-Grained Updates to Immutable
Data.* Tech report; https://ankurdave.com/dl/part-tr.pdf (used in
Tegra, NSDI 2021).

[20] R. Tarjan, C. Levy, S. Timmel. *Zip Trees.* WADS 2019 / ACM
TALG 2021.

[21] O. Gila, M. Goodrich, R. Tarjan. *Zip-Zip Trees: Making Zip
Trees More Balanced, Biased, Compact, or Persistent.* WADS 2023.

[22] D. Golovin. *B-Treaps: A Uniquely Represented Alternative to
B-Trees.* ICALP 2009.

[23] D. Golovin. *The B-Skip-List: A Simpler Uniquely Represented
Alternative to B-Trees.* arXiv:1005.0662, 2010. (See also *B-Treaps
Revised*, arXiv:2303.04722, 2023.)

[24] A. Auvolat, F. Taïani. *Merkle Search Trees: Efficient
State-Based CRDTs in Open Networks.* SRDS 2019. (Basis of Bluesky's
AT Protocol repository structure.)

[25] Prolly Trees — content-defined chunking search trees.
Noms/Dolt engineering documentation,
https://docs.dolthub.com/architecture/storage-engine/prolly-tree

Practice surveyed: Rust `im` crate (`OrdMap`/`OrdSet`, 64-chunk
B-trees); Clojure `sorted-map` (persistent red-black) and
`data.avl`; Scala `TreeMap`; Haskell `containers` (weight-balanced);
`absl::btree_map` (mutable B-tree engineering); btrfs / LMDB /
CouchDB (copy-on-write B(+)-trees); PAM/CPAM (C++ persistent
augmented maps); hitchhiker trees (write-buffered functional
B-trees).
