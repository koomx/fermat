# Fermat containers

In-process C++ containers. Read [`fermat/skills.h`](../fermat/skills.h) first.
This page is the human selection guide.

Bit/integer **codecs** (PFor, ZFP, …) are **out of scope**. Use another repo.
Roaring in Fermat is an integer **set** (union/intersect/contains), not a file codec.

## Default picks

1. **General KV, large table:** `fermat::flat_hash_map` / `flat_hash_set`
   (Swiss table). Prefer `find`, not `operator[]`, on the lookup path.
2. **Tiny map** (a handful of keys, HTTP query/header): `fermat::vector_map`.
   Contiguous, transparent lookup if `Compare` has `is_transparent`.
3. **Ordered range** over comparable keys: `fermat::btree_map` / `btree_set`.
   Do not use ART as a generic ordered `std::map`.
4. **String prefix / dictionary / routing:** `fermat::htrie_map` / `htrie_set`.
5. **Short C-string index, point lookup:** `fermat::Art<T>` (`get`/`set`/`del`
   on `const char*`).
6. **Integer sets:** `fermat::roaring::Roaring` / `Roaring64` / `Roaring64Map`.
   C API is under `fermat::roaring` / `fermat::roaring::api` (not global
   `roaring_*` names in public C++ wrappers). Do not rely on
   `-fvisibility=hidden` for ABI isolation; C symbols must stay prefixed if
   another CRoaring is linked (work in progress if still unprefixed).
7. **Pointer stability:** `node_hash_*`. **Insertion order:** `linked_hash_*`.
8. **FIFO:** `chunked_queue` / `ring_buffer`. **Intrusive list:** `list/intrusive_list.h`.

## Do not

- Add another hashmap “just in case” (no robin-hood).
- Put bit codecs next to maps.
- Treat ART as STL map (byte keys only; `grow()` on inner nodes `delete this`).
- Scan the whole tree: headers under `fermat/<module>/`, tests under `tests/`.
- Edit `kmcmake/` unless the task is the framework.

## Tests and examples

- Unit tests: GTest, `kmcmake_cc_test`, link `fermat::fermat_static`.
- Roaring demos: `examples/roaring/*.cc`, not tests.
- ART Google Benchmark: `benchmark/art/all_benchmark.cc`.
- Roaring unified bench: `benchmark/roaring/benchmark.cpp` (own main).

## Headers (includes from repo root)

```
#include <fermat/hashmaps/flat_hash_map.h>
#include <fermat/hashmaps/flat_hash_set.h>
#include <fermat/hashmaps/node_hash_map.h>
#include <fermat/hashmaps/linked_hash_map.h>
#include <fermat/maps/vector_map.h>
#include <fermat/maps/btree_map.h>
#include <fermat/trie/hat/htrie_map.h>
#include <fermat/art/art.h>
#include <fermat/roaring/roaring.hpp>
#include <fermat/roaring/roaring64.hpp>
#include <fermat/roaring/roaring64map.hpp>
#include <fermat/queue/chunked_queue.h>
#include <fermat/list/intrusive_list.h>
#include <fermat/memory/container_memory.h>
```
