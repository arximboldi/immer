# Sorted containers — implementation plan

- **Status**: ready to execute
- **Companion**: [design document](design-sorted-containers.md) (interface &
  data-structure rationale; §9 records the resolved questions)
- **Tracking issue**: [#105](https://github.com/arximboldi/immer/issues/105)

## 0. Resolved decisions (binding for this plan)

From the Q&A in design doc §9:

| # | Decision | Resolution | Consequence |
|---|---|---|---|
| 1 | Naming | `sorted_map` / `sorted_set` | headers, namespaces, docs as designed |
| 2 | B / BL defaults | `2^5` both | benchmark-driven retune is a Milestone-4 task, not a blocker |
| 3 | Comparators | stateless, default-constructible (`Compare{}` at use sites) | no comparator storage anywhere; matches `Hash`/`Equal` |
| 4 | Set algebra | **members**: `merge`, `intersect`, `difference`; map also `merge(other, combine_fn)` | operators `\|`/`&`/`-` deferred to backlog (would need an ordered+unordered story) |
| 5 | `erase(iterator)` | not now — API symmetry with `map`/`set` first | backlog note: if benchmarks show a big win, add it *library-wide* |
| 6 | `equal_range` | omitted | doc note: `{lower_bound(k), advanced by count(k)}` |
| 7 | Rebalancing | implementer's choice → **bottom-up recursive** (better fill, simpler invariants) | code comment + backlog item to benchmark Rodeh-style top-down |
| 8 | Augmentation counters | 32-bit (`std::uint32_t`) | container capacity capped at 2³²−1 elements; debug assert + documented limit |

Two semantic call-outs to keep visible in review (both deliberate, both
documented): `insert` **replaces** (immer convention, unlike `std::map`), and
member `merge` **returns the union** (unlike `std::map::merge`, which splices
from its argument — a mutating concept that doesn't map to immer anyway).

## 1. Ground rules

- **Oracle-first.** The `std::map`-differential harness and the structural
  invariant checker land *before or with* each mutating operation, and every
  new operation must demonstrably fail its tests when sabotaged (flip a
  comparison, drop a rebalance) before it passes. Erase/borrow/merge is where
  B-tree bugs live; we don't get to discover that in a fuzzer three weeks in.
- **No build-system work needed.** `test/`, `extra/fuzzer/`, `benchmark/`
  CMakeLists all `GLOB_RECURSE` their sources — new files self-register.
  Same for `test/oss-fuzz`.
- **House style.** C++14; doxygen comments written with the code, matching
  `map.hpp`'s voice (including adapted "why does `find` return a pointer"
  admonition); `IMMER_NODISCARD`; warning pragmas as in `hamts/node.hpp`;
  `static_assert` nothrow-move of every public container.
- **One milestone = one PR**, each independently green: unit tests, ASAN/LSAN,
  valgrind, MSVC, coverage, docs build. Definition-of-done checklists below.

## 2. Milestone overview

| Milestone | Contents | Deliverable | New code (est.) |
|---|---|---|---|
| **M1** | `detail/bts` core; `sorted_map`/`sorted_set` + transients; lookup, insert, erase, update, iterators, bounds, equality; tests, fuzzers, smoke benchmark; docs | usable containers | ~2.2k lib, ~2.5k test |
| **M2** | `join`/`split` internals; `take`/`drop`/`split(k)`; `nth`/`rank`; `from_sorted`; overflow guards | slicing & rank | ~0.6k lib, ~0.8k test |
| **M3** | `merge`/`intersect`/`difference`, map `merge`+combiner; ordered `immer::diff`; `==` subtree skipping; iterator `distance` | algebra & diff | ~0.7k lib, ~0.9k test |
| **M4** | full benchmark matrix; B/BL + rebalancing + in-node-search experiments; tuning report | validated defaults | ~0.8k bench |
| **M5** | `sorted_table`; `immer::persist` pools; website docs, changelog | ecosystem parity | ~1.5k |

M2 and M3 are split so that the join/split machinery (M2) stabilizes under
fuzzing for a full cycle before the algebraic operations (M3) start composing
it recursively.

## 3. Milestone 1 — core containers

### 3.1 Files

```
immer/detail/bts/bits.hpp              (~60 loc)
immer/detail/bts/node.hpp              (~450 loc)
immer/detail/bts/btree.hpp             (~900 loc by end of M1)
immer/detail/bts/btree_iterator.hpp    (~220 loc)
immer/sorted_map.hpp                   (~450 loc, half doxygen)
immer/sorted_set.hpp                   (~350 loc)
immer/sorted_map_transient.hpp         (~250 loc)
immer/sorted_set_transient.hpp         (~200 loc)
test/sorted_map/{generic.ipp, default.cpp, B3.cpp, B6.cpp, gc.cpp}
test/sorted_set/{...same...}
test/sorted_map_transient/{...}        test/sorted_set_transient/{...}
test/detail/bts/node.cpp               (node lifecycle smoke tests)
test/alignment.cpp                     (add sorted types to existing test)
extra/fuzzer/{sorted-map.cpp, sorted-map-gc.cpp, sorted-set.cpp}
benchmark/sorted-set/unsigned/{insert,access,iter}.cpp   (smoke only in M1)
doc/containers.rst, doc/transients.rst, README.rst       (entries)
```

### 3.2 Build order (commit-sized steps, each with its test gate)

**Step 1 — `bits.hpp`.** `bits_t`, `count_t`, `size_t` (`std::size_t`),
`local_size_t` (`std::uint32_t`, decision #8), `branches<B>`,
`min_branches<B> = branches<B>/2`, `max_depth<B, BL>`
(= ⌈(64 − (BL−1)) / (B−1)⌉ + 1 ≈ 16 at defaults; bounds iterator stacks and
recursion). Reuses `immer::default_bits`.

**Step 2 — `node.hpp`.** Mirrors `hamts/node.hpp` structurally:

- `template <typename T, typename KeyFn, typename Compare, typename MemoryPolicy, bits_t B, bits_t BL> struct node`
  with `kind_t {leaf, inner}`; layouts through
  `combine_standard_layout_t<..., refs_t, ownee_t>`:
  - `leaf_t`: `count_t count` + trailing `T[≤ 2^BL]`;
  - `inner_t`: `count_t count` + trailing arrays: `K keys[≤ 2^B − 1]`,
    `node_t* children[≤ 2^B]`, `local_size_t sizes[≤ 2^B]` (cumulative).
    One allocation, computed offsets (`sizeof_inner_n` etc.).
- Fixed-capacity allocation through `heap_policy::optimized<max_sizeof_leaf>`
  / `optimized<max_sizeof_inner>` (two size classes, as rbts sizes its nodes).
- Node kind is **contextual** (uniform leaf depth; the tree handle stores the
  height, as rbts stores `shift`) — no runtime tag on hot paths;
  `IMMER_TAGGED_NODE` debug tag + `IMMER_ASSERT_TAGGED` like champ.
- Lifecycle: `make_leaf_n/make_inner_n`, `copy_leaf[_insert/_replace/_erase]`,
  `copy_inner[_replace/_insert_split/_merge]`, `inc`, `dec(height)`
  (recursive, height-threaded), `can_mutate(node, edit)` ≡ champ's
  `unique() || ownee.can_mutate(e)`, `delete_leaf/delete_inner`.
- Element copies via `detail::uninitialized_copy`/`uninitialized_move` with
  unwind cleanup (`detail/util.hpp`); children incref'd only after the copy
  that references them cannot fail anymore.
- Gate: `test/detail/bts/node.cpp` — make/copy/dec pairs balance refcounts
  (counting via `dada.hpp` allocators / `debug_size_heap`), throwing-copy
  element types leak nothing.

**Step 3 — `btree.hpp` skeleton + validator.**
`struct btree { node_t* root; size_t size; count_t height; }` (empty ⇒
statically-allocated empty leaf, champ's `empty()` pattern), rule-of-five +
`swap` copied from champ, `inc`/`dec`.

Debug machinery *before any mutation exists*:

- `check_tree()` (mirrors `check_champ`): uniform leaf depth; fill within
  `[min, max]` for non-roots; keys strictly ascending within nodes; every
  separator partitions its subtrees (loose-separator rule: presence not
  required); cumulative `sizes[]` consistent with recursive counts; `size`
  field consistent; refcounts ≥ 1.
- `get_debug_stats()` under `IMMER_DEBUG_STATS` (fill-rate histograms — this
  is also how we validate the §5 cost-model claims in M4).
- `get<Project, Default>(k)` lookup with `std::lower_bound` over separators /
  leaf keys, transparent-key templated like champ's `get`.
- Gate: empty-container behavior, lookups on hand-built tiny trees.

**Step 4 — persistent insert.** Bottom-up recursion (decision #7):

```c++
struct add_result { node_t* node; node_t* split; };  // split == nullptr usually
// leaf overflow  -> split at midpoint; separator = first key of right node (copied)
// inner overflow -> split children/keys/sizes at midpoint
// root split     -> new root, ++height
```

`add(T v)` (replace-on-equal, immer semantics) and the minimal public
façades `sorted_set::insert`, `sorted_map::insert/set` so tests run against
the real API. Gate: generic tests (ascending / descending / random / replace
patterns), **oracle test** (random op tape vs `std::map`, full-content
equality at checkpoints), `check_tree()` after every mutation, `dada.hpp`
exception injection.

**Step 5 — persistent erase.** `sub_result {node_t* node; bool underflow}`;
underflow resolved *in the copied parent* (borrow from richer neighbor, else
merge; separators only rotate, never invented); root collapse / −−height.
This is the highest-risk step of the whole project: erase-heavy oracle tapes,
erase-to-empty, erase-of-absent, interleaved insert/erase tapes, dada.

**Step 6 — update / update_if_exists.** Pure leaf value replacement (no
rebalancing) + `combine_value`-style projections shared with `map`'s idioms.

**Step 7 — iterator.** `btree_iterator` on `detail::iterator_facade`;
`std::array<(node*, count_t), max_depth>` stack + direct `cur_/end_` value
cursor in the current leaf (champ_iterator pattern), **bidirectional**
(`--` descends to rightmost; `--end()` supported), `end_t` sentinel ctor.
Gate: forward/backward full-iteration equivalence vs `std::map`, reverse
iterators, `IMMER_RANGES_CHECK(std::ranges::bidirectional_range<...>)`.

**Step 8 — ordered queries + full read façade.** `lower_bound`, `upper_bound`
(iterators positioned via descent stacks), `front`/`back`, `count`/`find`/
`at`/`operator[]` with transparent overloads gated on
`Compare::is_transparent` (SFINAE shape copied from `map`'s
`Hash::is_transparent` overloads); `operator==`/`!=` (size → identity → dual
leaf-stream scan; the shared-subtree *skip* optimization is deferred to M3,
correctness doesn't need it); `identity()`; persist-style semi-private
`impl()` + impl constructor.

**Step 9 — transients.** `add_mut/sub_mut/update_mut(edit, ...)` threading
`can_mutate` down the path: owned nodes edited in place (in-leaf `memmove`
insertion — gated on `std::is_nothrow_move_constructible<T>`, else copy
path), others path-copied with new nodes stamped `ownee = edit`. Façades
`sorted_map_transient`/`sorted_set_transient` inherit
`MemoryPolicy::transience_t::owner` (as `map_transient` does), expose
read API + iterators + `persistent()`. `&&`-qualified persistent methods via
the exact `move_t{}` / `*_move` dispatch from `map.hpp`. Gate:
`transient_tester.hpp`-style divergence tests (freeze mid-batch, keep
mutating, verify the frozen snapshot), oracle tapes mixing persistent &
transient ops, gc variant.

**Step 10 — chunked traversal.** `for_each_chunk`/`for_each_chunk_p` handing
out leaf arrays (in key order — worth a doc note: unlike `map`, chunk order
is meaningful), which lights up `immer::accumulate`/`all_of` from
`algorithm.hpp` for free. Gate: algorithm.cpp-style tests.

**Step 11 — fuzzers.** Following `flex-vector.cpp`'s shape (`fuzzer_input`,
variable array, op enum): ops = insert, insert-move, set, update, erase,
find/lower_bound spot-checks, iterate-and-compare (size-capped), assignment
between vars, `transient()`/`persistent()` round-trips; each var shadowed by
a `std::map` oracle; `check_tree()` under `IMMER_DEBUG` builds. Variants:
default policy, GC policy; seed corpus into `test/oss-fuzz/data`. Run
locally overnight before merge.

**Step 12 — docs + smoke benchmark.** Doxygen throughout; `containers.rst` +
`transients.rst` sections; README container list; one
`benchmark/sorted-set/unsigned` trio (insert/access/iter vs `std::map`,
`std::set`, boost `flat_set`, `immer::set`) so the PR carries first numbers —
the full matrix is M4.

### 3.3 Milestone 1 definition of done

- [ ] Generic suites green for: default policy, `B=3`, `B=6`, GC policy —
      for all four containers (2 persistent + 2 transient).
- [ ] Oracle tapes (≥10⁵ mixed ops) pass with `check_tree()` after every
      mutation in debug builds.
- [ ] `dada.hpp` fault-injection suites pass (persistent ops strong
      guarantee; transient fallback path covered).
- [ ] Fuzzers: overnight local run, zero findings; oss-fuzz harness merged.
- [ ] Valgrind + ASAN/LSAN/UBSAN clean; MSVC green; alignment test extended.
- [ ] `IMMER_NO_THREAD_SAFETY` and `IMMER_NO_FREE_LIST` configurations
      compile and pass (they alter heap/refcount paths).
- [ ] Every public symbol doxygen'd; docs build; README updated.
- [ ] Sanity numbers from the smoke benchmark quoted in the PR description
      (lookup/insert/iter vs `std::map` at n ∈ {10³, 10⁵, 10⁷}).

## 4. Milestone 2 — slicing, rank, bulk

1. **Internal `join`** (concatenate height-aligned trees with a separator;
   cascade splits; O(Δheight)) and **`split_at`** (by key and by rank —
   telescoping joins of the off-path subtrees). These are *internal* in M2.
2. Public **`take(n)` / `drop(n)` / `split(k)`** on both containers
   (`{keys < k, keys ≥ k}` per the design synopsis).
3. **`nth(i)` / `rank(k)`** descending the cumulative `sizes[]` arrays.
4. **`from_sorted(range)`** packed bulk build (100% fill, O(n)); the
   iterator-pair constructor gains the sortedness fast-path check with
   transient-insertion fallback.
5. **Capacity guard** (decision #8): `IMMER_ASSERT`-on-2³² overflow in the
   augmentation update path + documented limit.

Tests: partition properties (`take(i) + drop(i)` reassembles; `split(k)`
agrees with the oracle's `partition_point`), rank/nth vs sorted-vector
oracle, join/split invariant fuzzing (compose random take/drop/split chains,
`check_tree()` throughout, compare contents to oracle slices), dada over
slicing, fill-rate stats before/after bulk build.

DoD: as M1's checklist scoped to the new ops, plus fuzzer op-enum extended
with take/drop/split and re-soaked overnight.

## 5. Milestone 3 — algebra, diff, fast equality

1. **`merge` / `intersect` / `difference`** members (decision #4) on
   `sorted_set`; on `sorted_map`: `merge(other)` (right-biased — argument
   wins, matching `insert`-replaces intuition; documented) and
   `merge(other, Fn combine)` — recursive split+join (Adams/[9,13] shape)
   with the two fast paths: pointer-identical subtree short-circuit and
   small-vs-large degradation to insertion. Sequential only.
2. **Ordered `immer::diff`**: `btree::diff(other, differ)` so the existing
   free `immer::diff` in `algorithm.hpp` dispatches unchanged; callbacks in
   key order; identical-node skipping ⇒ effectively O(|diff|) between
   related versions. Documented contrast with the champ (unordered) diff.
3. **`operator==` subtree skipping** (dual-cursor scan skips a whole node
   when both cursors stand at the start of the same node object).
4. **Iterator `distance`** in O(log n) via the descent stacks + `sizes[]`.

Tests: algebraic properties on random and *derived* inputs (a merged with
a.insert(...)ᵏ must run in ~O(k·log n) — assert via op-count instrumentation
in debug stats, not wall clock), oracle via `std::set_union` /
`set_intersection` / `set_difference`, combiner semantics, diff callback
order + added/removed/changed completeness (reuse `test/algorithm.cpp`
patterns), equality of structurally-shared vs freshly-built equal trees.

DoD: fuzzer extended with binary ops between vars (this is where aliasing
bugs surface — two vars sharing structure merged into a third), overnight
soak, docs for the semantic notes in §0.

## 6. Milestone 4 — benchmarks and tuning

Benchmark matrix under `benchmark/sorted-map/` + `benchmark/sorted-set/`,
reusing the `benchmark/set` generator layout (`unsigned`, `string-short`,
`string-long`, `string-box`) and nonius config:

- Ops: point access (hit/miss), `lower_bound`, ordered iteration, range scan
  (window sums), persistent insert/erase, transient build, `from_sorted`,
  version merge (derived and unrelated), diff, memory-per-element (via
  `debug_size_heap`, `benchmark/set/memory` pattern).
- Baselines: `std::map`/`std::set`, sorted `std::vector` + binary search,
  `boost::container::flat_map` (already a bench dependency),
  `immer::map`/`set` (price of ordering), `immer::box<std::map>`
  (the workaround), and — optional, behind a CMake find — `absl::btree_map`.

Experiments this milestone resolves (all backlog-tagged in code comments):

1. **B/BL defaults** (decision #2): sweep {4,5,6}×{4,5,6} on the matrix.
2. **Bottom-up vs top-down preemptive** rebalancing (decision #7): branch
   experiment; keep whichever wins, record numbers in the design doc.
3. **In-node search**: `std::lower_bound` vs branchless/linear scan for
   arithmetic keys; memcpy fast paths for trivially-copyable elements.
4. Validate the §5 cost model (fill rates from debug stats, B/element,
   allocations/op) and update the design doc's numbers with measured ones.

Deliverable: numbers table + any default changes + a short "measured
characteristics" section replacing the design doc's estimates.

## 7. Milestone 5 — ecosystem parity

1. **`sorted_table<T, KeyFn, Compare>`** — the btree core is already
   `KeyFn`-parameterized (that's how map/set share it); this is a façade +
   tests exercise, mirroring `table.hpp`'s key-fn machinery.
2. **`immer::persist`** pools for bts nodes (C++17 land): leaf/inner pool
   types (counts, keys, values, child ids, sizes), champ-pool-equivalent
   save/load with sharing preservation, `transform` support; tests under
   `test/extra/persist`. Format documented in the persist docs.
3. Website docs (`containers.rst` long-form section with literalincludes
   from new `example/sorted_map/*.cpp`), changelog, README feature bullet,
   close #105 with a summary comment.

## 8. Cross-cutting engineering notes

- **Exception safety pattern**: all multi-node constructions go through
  RAII-ish local guards (build children first, adopt into parent copy last;
  on unwind, `dec` exactly what was built) — same discipline as rbts
  operations; `dada.hpp` is the enforcement mechanism, run over every op in
  the generic suites.
- **GC policy**: `dec` is a no-op, transience via `gc_transience_policy`
  heap-versioned edits; make the gc test variants exercise the patterns from
  the recent Boehm fixes (#324) — interior pointers, finalizer-free nodes.
- **MSVC watchlist**: `combine_standard_layout_t` unions (mirror champ's
  warning pragmas), `.template` disambiguation in the transparent-key
  overloads, over-aligned element types (alignment test), `/permissive-`.
- **Thread-safety configs**: default (atomic refcount + spinlock policy) and
  `IMMER_NO_THREAD_SAFETY` both in the test matrix from M1 on.
- **Height/kind threading**: no node tag on hot paths means every recursive
  helper takes a depth/height parameter; the debug tag + tagged asserts keep
  this honest.
- **Coverage**: `detail/bts` included in codecov; target ≥ the champ
  baseline before M1 merges.

## 9. Risk register

| Risk | Mitigation |
|---|---|
| Erase/borrow/merge bugs (the classic B-tree minefield) | oracle after every op + invariant checker + erase-heavy fuzzer corpus; erase gets its own step with the largest test budget |
| Transient aliasing (owned node reachable from a frozen snapshot) | champ's exact ownership rule, divergence tests that freeze mid-batch, fuzzer interleavings, LSAN |
| Exception-safety leaks in multi-node ops (split allocates two, parent throws) | guard pattern + dada on every op from day one |
| Refcount-increment storms disappoint vs the cost model | smoke benchmark in M1 (not M4) so surprises show up early; B knob; `unsafe_refcount_policy` comparison point |
| Iterator edge cases (`--end()`, empty tree, single leaf) | dedicated boundary battery + ranges conformance check |
| MSVC layout breakage | alignment tests + full CI matrix from the first PR |
| Scope creep in M1 | API surface frozen to design §2 synopsis; anything else goes to M2+ by construction |

## 10. Backlog (recorded, deliberately not scheduled)

- `erase(iterator)` / erase-by-range — evaluate library-wide if M4 shows
  re-search cost matters (decision #5).
- Set-algebra operators `|`, `&`, `-` across ordered *and* unordered
  containers (decision #4).
- Stateful comparator support (decision #3) if a real use case shows up.
- `equal_range` (decision #6) — revisit only for std-interop pressure.
- SIMD in-node search beyond M4's scalar experiments.
- `sorted_map`/`flex_vector` conversions (`keys()`/`values()` views).
- `immer::radix_map`, canonical prolly/MST variant, parallel bulk ops
  (design §8).

## 11. Suggested first session

Steps 1–3 of M1 (bits, node, tree skeleton + validator + lookup) plus the
oracle harness scaffold — roughly 700 lines of library code and 400 of
tests, no mutation algorithms yet, everything checkable. From there, each
subsequent step is a self-contained commit with its own gate.
