// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "benchmark/benchmark.h"
#include <fermat/art/art.h>
#include <fermat/art/internal/node.h>

namespace {

using fermat::Art;
using fermat::art_internal::LeafNode;
using fermat::art_internal::node_4;
using fermat::art_internal::node_16;
using fermat::art_internal::node_48;
using fermat::art_internal::node_256;

uint32_t ZipfSample(std::mt19937& rng, uint32_t n) {
    std::uniform_real_distribution<double> u(std::numeric_limits<double>::min(),
        1.0);
    const double maxv = static_cast<double>(n);
    const double exp = -1.0 / 0.6;
    for (;;) {
        double k = std::pow(u(rng), exp);
        if (k >= 1.0 && k <= maxv) {
            return static_cast<uint32_t>(k) - 1u;
        }
    }
}

std::string ToBase64(const std::string& string_) {
    static const std::string kChars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const char* bytes_to_encode = string_.c_str();
    int in_len = static_cast<int>(string_.length());
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = static_cast<unsigned char>(*(bytes_to_encode++));
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] =
                ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] =
                ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; i < 4; i++) {
                ret += kChars[char_array_4[i]];
            }
            i = 0;
        }
    }
    if (i) {
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] =
            ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] =
            ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (j = 0; j < i + 1; j++) {
            ret += kChars[char_array_4[j]];
        }
        while ((i++ < 3)) {
            ret += '=';
        }
    }
    return ret;
}

char RandPartialKey(std::mt19937& rng) {
    return static_cast<char>(std::uniform_int_distribution<int>(0, 255)(rng) - 128);
}

// --- insert ---

void BM_ArtInsertSparse(benchmark::State& state) {
    Art<int*> m;
    int v = 1;
    std::mt19937_64 rng(0);
    for (auto _ : state) {
        auto k = std::to_string(rng());
        m.set(k.c_str(), &v);
    }
}
BENCHMARK(BM_ArtInsertSparse);

void BM_RedBlackInsertSparse(benchmark::State& state) {
    std::map<std::string, int> m;
    int v = 1;
    std::mt19937_64 rng(0);
    for (auto _ : state) {
        m[std::to_string(rng())] = v;
    }
}
BENCHMARK(BM_RedBlackInsertSparse);

void BM_HashmapInsertSparse(benchmark::State& state) {
    std::unordered_map<std::string, int> m;
    int v = 1;
    std::mt19937_64 rng(0);
    for (auto _ : state) {
        m[std::to_string(rng())] = v;
    }
}
BENCHMARK(BM_HashmapInsertSparse);

// --- delete ---

void BM_ArtDeleteSparse(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    int v = 1;
    std::vector<std::string> keys;
    keys.reserve(n);
    std::mt19937_64 g(0);
    for (int i = 0; i < n; ++i) {
        keys.push_back(std::to_string(g()));
    }
    for (auto _ : state) {
        state.PauseTiming();
        Art<int*> m;
        for (const auto& k : keys) {
            m.set(k.c_str(), &v);
        }
        state.ResumeTiming();
        for (const auto& k : keys) {
            m.del(k.c_str());
        }
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_ArtDeleteSparse)->Arg(10000);

void BM_RedBlackDeleteSparse(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    int v = 1;
    std::vector<std::string> keys;
    keys.reserve(n);
    std::mt19937_64 g(0);
    for (int i = 0; i < n; ++i) {
        keys.push_back(std::to_string(g()));
    }
    for (auto _ : state) {
        state.PauseTiming();
        std::map<std::string, int> m;
        for (const auto& k : keys) {
            m[k] = v;
        }
        state.ResumeTiming();
        for (const auto& k : keys) {
            m.erase(k);
        }
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_RedBlackDeleteSparse)->Arg(10000);

void BM_HashmapDeleteSparse(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    int v = 1;
    std::vector<std::string> keys;
    keys.reserve(n);
    std::mt19937_64 g(0);
    for (int i = 0; i < n; ++i) {
        keys.push_back(std::to_string(g()));
    }
    for (auto _ : state) {
        state.PauseTiming();
        std::unordered_map<std::string, int> m;
        for (const auto& k : keys) {
            m[k] = v;
        }
        state.ResumeTiming();
        for (const auto& k : keys) {
            m.erase(k);
        }
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_HashmapDeleteSparse)->Arg(10000);

// --- memory ---

void BM_ArtMemoryUniform(benchmark::State& state) {
    const uint32_t n = static_cast<uint32_t>(state.range(0));
    std::hash<uint32_t> h;
    for (auto _ : state) {
        Art<int*> m;
        for (uint32_t i = 0; i < n; i++) {
            m.set(std::to_string(h(i)).c_str(), nullptr);
        }
        benchmark::DoNotOptimize(m);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_ArtMemoryUniform)->Arg(100000);

void BM_ArtMemoryZipf(benchmark::State& state) {
    const uint32_t n = static_cast<uint32_t>(state.range(0));
    std::hash<uint32_t> h;
    std::mt19937 rng(0);
    for (auto _ : state) {
        Art<int*> m;
        for (uint32_t i = 0; i < n; i++) {
            m.set(std::to_string(h(ZipfSample(rng, n))).c_str(), nullptr);
        }
        benchmark::DoNotOptimize(m);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_ArtMemoryZipf)->Arg(100000);

// --- mixed ---

void BM_ArtMixedSparse(benchmark::State& state) {
    Art<int*> m;
    std::mt19937 rng(0);
    std::hash<uint32_t> h;
    int v = 1;
    for (auto _ : state) {
        std::string k = std::to_string(h(ZipfSample(rng, 10000000)));
        if (m.get(k.c_str()) == nullptr) {
            m.set(k.c_str(), &v);
        } else {
            m.del(k.c_str());
        }
    }
}
BENCHMARK(BM_ArtMixedSparse);

void BM_RedBlackMixedSparse(benchmark::State& state) {
    std::map<std::string, int*> m;
    std::mt19937 rng(0);
    std::hash<uint32_t> h;
    int v = 1;
    for (auto _ : state) {
        std::string k = std::to_string(h(ZipfSample(rng, 10000000)));
        if (m[k] == nullptr) {
            m[k] = &v;
        } else {
            m.erase(k);
        }
    }
}
BENCHMARK(BM_RedBlackMixedSparse);

void BM_HashmapMixedSparse(benchmark::State& state) {
    std::unordered_map<std::string, int*> m;
    std::mt19937 rng(0);
    std::hash<uint32_t> h;
    int v = 1;
    for (auto _ : state) {
        std::string k = std::to_string(h(ZipfSample(rng, 10000000)));
        if (m[k] == nullptr) {
            m[k] = &v;
        } else {
            m.erase(k);
        }
    }
}
BENCHMARK(BM_HashmapMixedSparse);

void BM_ArtMixedDense(benchmark::State& state) {
    std::ifstream file("dataset.txt");
    if (!file) {
        state.SkipWithError("dataset.txt not found");
        return;
    }
    std::unordered_map<uint32_t, std::string> dataset;
    uint32_t n = 0;
    std::string line;
    while (std::getline(file, line)) {
        dataset[n++] = line;
    }
    if (n == 0) {
        state.SkipWithError("dataset.txt is empty");
        return;
    }
    int v = 1;
    std::mt19937 rng(0);
    Art<int*> m;
    for (auto _ : state) {
        const std::string& k = dataset[ZipfSample(rng, n)];
        if (m.get(k.c_str()) == nullptr) {
            m.set(k.c_str(), &v);
        } else {
            m.del(k.c_str());
        }
    }
}
BENCHMARK(BM_ArtMixedDense);

// --- node helpers ---

template <class NodeT>
void BM_NodeConstructor(benchmark::State& state) {
    for (auto _ : state) {
        NodeT n;
        benchmark::DoNotOptimize(n);
    }
}

void BM_Node4FindChild(benchmark::State& state) {
    node_4<int*> n;
    LeafNode<int*> c0(nullptr);
    LeafNode<int*> c1(nullptr);
    LeafNode<int*> c2(nullptr);
    LeafNode<int*> c3(nullptr);
    n.set_child(-128, &c0);
    n.set_child(-43, &c1);
    n.set_child(42, &c2);
    n.set_child(127, &c3);
    char partial_keys[] = { -128, -43, 42, 127 };
    std::mt19937 rng(0);
    std::uniform_int_distribution<int> dist(0, 3);
    for (auto _ : state) {
        auto* child = n.find_child(partial_keys[dist(rng)]);
        benchmark::DoNotOptimize(child);
    }
}

void BM_Node4SetChild(benchmark::State& state) {
    auto* n = new node_4<int*>();
    LeafNode<int*> child(nullptr);
    char partial_keys[] = { -128, -43, 42, 127 };
    std::mt19937 rng(0);
    std::uniform_int_distribution<int> dist(0, 3);
    for (auto _ : state) {
        if (n->is_full()) {
            delete n;
            n = new node_4<int*>();
        }
        n->set_child(partial_keys[dist(rng)], &child);
    }
    delete n;
}

void BM_Node4Grow(benchmark::State& state) {
    for (auto _ : state) {
        auto* n = new node_4<int*>();
        auto* new_n = n->grow();
        benchmark::DoNotOptimize(new_n);
        delete new_n;
        // grow() deletes *this
    }
}

void BM_Node4NextPartialKey(benchmark::State& state) {
    node_4<int*> n;
    LeafNode<int*> c0(nullptr);
    LeafNode<int*> c1(nullptr);
    LeafNode<int*> c2(nullptr);
    LeafNode<int*> c3(nullptr);
    n.set_child(-128, &c0);
    n.set_child(-43, &c1);
    n.set_child(42, &c2);
    n.set_child(127, &c3);
    std::mt19937 rng(0);
    for (auto _ : state) {
        try {
            auto k = n.next_partial_key(RandPartialKey(rng));
            benchmark::DoNotOptimize(k);
        } catch (const std::out_of_range&) {
        }
    }
}

void BM_Node4PrevPartialKey(benchmark::State& state) {
    node_4<int*> n;
    LeafNode<int*> c0(nullptr);
    LeafNode<int*> c1(nullptr);
    LeafNode<int*> c2(nullptr);
    LeafNode<int*> c3(nullptr);
    n.set_child(-128, &c0);
    n.set_child(-43, &c1);
    n.set_child(42, &c2);
    n.set_child(127, &c3);
    std::mt19937 rng(0);
    for (auto _ : state) {
        try {
            auto k = n.prev_partial_key(RandPartialKey(rng));
            benchmark::DoNotOptimize(k);
        } catch (const std::out_of_range&) {
        }
    }
}

using Node4 = node_4<int*>;
using Node16 = node_16<int*>;
using Node48 = node_48<int*>;
using Node256 = node_256<int*>;

BENCHMARK_TEMPLATE(BM_NodeConstructor, Node4);
BENCHMARK(BM_Node4FindChild);
BENCHMARK(BM_Node4SetChild);
BENCHMARK(BM_Node4Grow);
BENCHMARK(BM_Node4NextPartialKey);
BENCHMARK(BM_Node4PrevPartialKey);

void BM_Node16FindChild(benchmark::State& state) {
    node_16<int*> n;
    LeafNode<int*> child(nullptr);
    for (int i = 0; i < 16; ++i) {
        n.set_child(static_cast<char>((i * 17) - 128), &child);
    }
    std::mt19937 rng(0);
    std::uniform_int_distribution<int> dist(0, 15);
    for (auto _ : state) {
        auto* child_out = n.find_child(static_cast<char>((dist(rng) * 17) - 128));
        benchmark::DoNotOptimize(child_out);
    }
}

void BM_Node16SetChild(benchmark::State& state) {
    auto* n = new node_16<int*>();
    LeafNode<int*> child(nullptr);
    std::mt19937 rng(0);
    std::uniform_int_distribution<int> dist(0, 15);
    for (auto _ : state) {
        if (n->is_full()) {
            delete n;
            n = new node_16<int*>();
        }
        n->set_child(static_cast<char>((dist(rng) * 17) - 128), &child);
    }
    delete n;
}

void BM_Node16Grow(benchmark::State& state) {
    for (auto _ : state) {
        auto* n = new node_16<int*>();
        auto* new_n = n->grow();
        benchmark::DoNotOptimize(new_n);
        delete new_n;
        // grow() deletes *this
    }
}

void BM_Node16NextPartialKey(benchmark::State& state) {
    node_16<int*> n;
    LeafNode<int*> child(nullptr);
    for (int i = 0; i < 16; ++i) {
        n.set_child(static_cast<char>((i * 17) - 128), &child);
    }
    std::mt19937 rng(0);
    for (auto _ : state) {
        try {
            auto k = n.next_partial_key(RandPartialKey(rng));
            benchmark::DoNotOptimize(k);
        } catch (const std::out_of_range&) {
        }
    }
}

void BM_Node16PrevPartialKey(benchmark::State& state) {
    node_16<int*> n;
    LeafNode<int*> child(nullptr);
    for (int i = 0; i < 16; ++i) {
        n.set_child(static_cast<char>((i * 17) - 128), &child);
    }
    std::mt19937 rng(0);
    for (auto _ : state) {
        try {
            auto k = n.prev_partial_key(RandPartialKey(rng));
            benchmark::DoNotOptimize(k);
        } catch (const std::out_of_range&) {
        }
    }
}

BENCHMARK_TEMPLATE(BM_NodeConstructor, Node16);
BENCHMARK(BM_Node16FindChild);
BENCHMARK(BM_Node16SetChild);
BENCHMARK(BM_Node16Grow);
BENCHMARK(BM_Node16NextPartialKey);
BENCHMARK(BM_Node16PrevPartialKey);

void BM_Node48FindChild(benchmark::State& state) {
    node_48<int*> n;
    LeafNode<int*> child(nullptr);
    for (int i = 0; i < 48; ++i) {
        n.set_child(static_cast<char>(std::floor(5.4468 * i) - 128), &child);
    }
    std::mt19937 rng(0);
    std::uniform_int_distribution<int> dist(0, 47);
    for (auto _ : state) {
        auto* c = n.find_child(
            static_cast<char>(std::floor(5.4468 * dist(rng)) - 128));
        benchmark::DoNotOptimize(c);
    }
}

void BM_Node48SetChild(benchmark::State& state) {
    auto* n = new node_48<int*>();
    LeafNode<int*> child(nullptr);
    std::mt19937 rng(0);
    for (auto _ : state) {
        if (n->is_full()) {
            delete n;
            n = new node_48<int*>();
        }
        n->set_child(RandPartialKey(rng), &child);
    }
    delete n;
}

void BM_Node48Grow(benchmark::State& state) {
    for (auto _ : state) {
        auto* n = new node_48<int*>();
        auto* new_n = n->grow();
        benchmark::DoNotOptimize(new_n);
        delete new_n;
        // grow() deletes *this
    }
}

void BM_Node48NextPartialKey(benchmark::State& state) {
    node_48<int*> n;
    LeafNode<int*> child(nullptr);
    for (int i = 0; i < 48; ++i) {
        n.set_child(static_cast<char>(std::floor(5.4468 * i) - 128), &child);
    }
    std::mt19937 rng(0);
    for (auto _ : state) {
        try {
            auto k = n.next_partial_key(RandPartialKey(rng));
            benchmark::DoNotOptimize(k);
        } catch (const std::out_of_range&) {
        }
    }
}

void BM_Node48PrevPartialKey(benchmark::State& state) {
    node_48<int*> n;
    LeafNode<int*> child(nullptr);
    for (int i = 0; i < 48; ++i) {
        n.set_child(static_cast<char>(std::floor(5.4468 * i) - 128), &child);
    }
    std::mt19937 rng(0);
    for (auto _ : state) {
        try {
            auto k = n.prev_partial_key(RandPartialKey(rng));
            benchmark::DoNotOptimize(k);
        } catch (const std::out_of_range&) {
        }
    }
}

BENCHMARK_TEMPLATE(BM_NodeConstructor, Node48);
BENCHMARK(BM_Node48FindChild);
BENCHMARK(BM_Node48SetChild);
BENCHMARK(BM_Node48Grow);
BENCHMARK(BM_Node48NextPartialKey);
BENCHMARK(BM_Node48PrevPartialKey);

void BM_Node256FindChild(benchmark::State& state) {
    node_256<int*> n;
    LeafNode<int*> child(nullptr);
    for (int i = 0; i < 256; ++i) {
        n.set_child(static_cast<char>(i - 128), &child);
    }
    std::mt19937 rng(0);
    for (auto _ : state) {
        auto* c = n.find_child(RandPartialKey(rng));
        benchmark::DoNotOptimize(c);
    }
}

void BM_Node256SetChild(benchmark::State& state) {
    auto* n = new node_256<int*>();
    LeafNode<int*> child(nullptr);
    std::mt19937 rng(0);
    for (auto _ : state) {
        if (n->is_full()) {
            delete n;
            n = new node_256<int*>();
        }
        n->set_child(RandPartialKey(rng), &child);
    }
    delete n;
}

void BM_Node256NextPartialKey(benchmark::State& state) {
    node_256<int*> n;
    LeafNode<int*> child(nullptr);
    for (int i = 0; i < 256; ++i) {
        n.set_child(static_cast<char>(i - 128), &child);
    }
    std::mt19937 rng(0);
    for (auto _ : state) {
        try {
            auto k = n.next_partial_key(RandPartialKey(rng));
            benchmark::DoNotOptimize(k);
        } catch (const std::out_of_range&) {
        }
    }
}

void BM_Node256PrevPartialKey(benchmark::State& state) {
    node_256<int*> n;
    LeafNode<int*> child(nullptr);
    for (int i = 0; i < 256; ++i) {
        n.set_child(static_cast<char>(i - 128), &child);
    }
    std::mt19937 rng(0);
    for (auto _ : state) {
        try {
            auto k = n.prev_partial_key(RandPartialKey(rng));
            benchmark::DoNotOptimize(k);
        } catch (const std::out_of_range&) {
        }
    }
}

BENCHMARK_TEMPLATE(BM_NodeConstructor, Node256);
BENCHMARK(BM_Node256FindChild);
BENCHMARK(BM_Node256SetChild);
BENCHMARK(BM_Node256NextPartialKey);
BENCHMARK(BM_Node256PrevPartialKey);

// --- query iteration ---

void BM_FullScanZipf(benchmark::State& state) {
    Art<int*> m;
    std::hash<uint32_t> h;
    int v = 1;
    std::mt19937 rng(0);
    for (int i = 0; i < 100000; ++i) {
        m.set(std::to_string(h(ZipfSample(rng, 1000000))).c_str(), &v);
    }
    for (auto _ : state) {
        for (auto it = m.begin(), it_end = m.end(); it != it_end; ++it) {
            benchmark::DoNotOptimize(*it);
        }
    }
}
BENCHMARK(BM_FullScanZipf);

void BM_FullScanUniform(benchmark::State& state) {
    Art<int*> m;
    std::hash<uint32_t> h;
    int v = 1;
    std::mt19937_64 rng(0);
    for (int i = 0; i < 100000; ++i) {
        m.set(std::to_string(h(static_cast<uint32_t>(rng()))).c_str(), &v);
    }
    for (auto _ : state) {
        for (auto it = m.begin(), it_end = m.end(); it != it_end; ++it) {
            benchmark::DoNotOptimize(*it);
        }
    }
}
BENCHMARK(BM_FullScanUniform);

// --- query sparse uniform ---

void BM_ArtQuerySparseUniform(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    Art<int*> m;
    std::hash<uint32_t> h;
    int v = 1;
    std::mt19937_64 rng1(0);
    std::vector<std::string> keys;
    keys.reserve(n);
    for (int i = 0; i < n; ++i) {
        keys.push_back(std::to_string(h(static_cast<uint32_t>(rng1()))));
        m.set(keys.back().c_str(), &v);
    }
    std::mt19937_64 rng2(0);
    for (auto _ : state) {
        auto* p = m.get(std::to_string(h(static_cast<uint32_t>(rng2()))).c_str());
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(BM_ArtQuerySparseUniform)->Arg(10000);

void BM_RedBlackQuerySparseUniform(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::map<std::string, int> m;
    std::hash<uint32_t> h;
    int v = 1;
    std::mt19937_64 rng1(0);
    for (int i = 0; i < n; ++i) {
        m[std::to_string(h(static_cast<uint32_t>(rng1())))] = v;
    }
    std::mt19937_64 rng2(0);
    for (auto _ : state) {
        v = m[std::to_string(h(static_cast<uint32_t>(rng2())))];
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_RedBlackQuerySparseUniform)->Arg(10000);

void BM_HashmapQuerySparseUniform(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::unordered_map<std::string, int> m;
    std::hash<uint32_t> h;
    int v = 1;
    std::mt19937_64 rng1(0);
    for (int i = 0; i < n; ++i) {
        m[std::to_string(h(static_cast<uint32_t>(rng1())))] = v;
    }
    std::mt19937_64 rng2(0);
    for (auto _ : state) {
        v = m[std::to_string(h(static_cast<uint32_t>(rng2())))];
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_HashmapQuerySparseUniform)->Arg(10000);

void BM_ArtQuerySparseUniform64(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    Art<int*> m;
    std::hash<uint32_t> h;
    int v = 1;
    std::mt19937_64 rng1(0);
    for (int i = 0; i < n; ++i) {
        m.set(ToBase64(std::to_string(h(static_cast<uint32_t>(rng1())))).c_str(),
            &v);
    }
    std::mt19937_64 rng2(0);
    for (auto _ : state) {
        auto* p = m.get(
            ToBase64(std::to_string(h(static_cast<uint32_t>(rng2())))).c_str());
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(BM_ArtQuerySparseUniform64)->Arg(10000);

void BM_RedBlackQuerySparseUniform64(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::map<std::string, int> m;
    std::hash<uint32_t> h;
    int v = 1;
    std::mt19937_64 rng1(0);
    for (int i = 0; i < n; ++i) {
        m[ToBase64(std::to_string(h(static_cast<uint32_t>(rng1()))))] = v;
    }
    std::mt19937_64 rng2(0);
    for (auto _ : state) {
        v = m[ToBase64(std::to_string(h(static_cast<uint32_t>(rng2()))))];
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_RedBlackQuerySparseUniform64)->Arg(10000);

void BM_HashmapQuerySparseUniform64(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::unordered_map<std::string, int> m;
    std::hash<uint32_t> h;
    int v = 1;
    std::mt19937_64 rng1(0);
    for (int i = 0; i < n; ++i) {
        m[ToBase64(std::to_string(h(static_cast<uint32_t>(rng1()))))] = v;
    }
    std::mt19937_64 rng2(0);
    for (auto _ : state) {
        v = m[ToBase64(std::to_string(h(static_cast<uint32_t>(rng2()))))];
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_HashmapQuerySparseUniform64)->Arg(10000);

void BM_ArtQuerySparseZipf(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    Art<int*> m;
    std::hash<uint32_t> h;
    int v = 1;
    std::mt19937 rng1(0);
    for (int i = 0; i < n; ++i) {
        m.set(std::to_string(h(ZipfSample(rng1, 1000000))).c_str(), &v);
    }
    std::mt19937 rng2(0);
    for (auto _ : state) {
        auto* p = m.get(std::to_string(h(ZipfSample(rng2, 1000000))).c_str());
        benchmark::DoNotOptimize(p);
    }
}
BENCHMARK(BM_ArtQuerySparseZipf)->Arg(10000);

void BM_RedBlackQuerySparseZipf(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::map<std::string, int> m;
    std::hash<uint32_t> h;
    int v = 1;
    std::mt19937 rng1(0);
    for (int i = 0; i < n; ++i) {
        m[std::to_string(h(ZipfSample(rng1, 1000000)))] = v;
    }
    std::mt19937 rng2(0);
    for (auto _ : state) {
        v = m[std::to_string(h(ZipfSample(rng2, 1000000)))];
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_RedBlackQuerySparseZipf)->Arg(10000);

void BM_HashmapQuerySparseZipf(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    std::unordered_map<std::string, int> m;
    std::hash<uint32_t> h;
    int v = 1;
    std::mt19937 rng1(0);
    for (int i = 0; i < n; ++i) {
        m[std::to_string(h(ZipfSample(rng1, 1000000)))] = v;
    }
    std::mt19937 rng2(0);
    for (auto _ : state) {
        v = m[std::to_string(h(ZipfSample(rng2, 1000000)))];
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_HashmapQuerySparseZipf)->Arg(10000);

}  // namespace
