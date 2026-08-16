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

#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <array>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include <turbo/debugging/internal/demangle.h>
#include <turbo/macros/config.h>
#include <turbo/meta/type_traits.h>
#include <turbo/strings/str_cat.h>
#include <turbo/types/span.h>
#include <turbo/utility/utility.h>

#if KUMO_HAVE_ADDRESS_SANITIZER
#include <sanitizer/asan_interface.h>
#endif

namespace fermat::base_internal {

    // A type wrapper that instructs `Layout` to use the specific alignment for the
    // array. `Layout<..., Aligned<T, N>, ...>` has exactly the same API
    // and behavior as `Layout<..., T, ...>` except that the first element of the
    // array of `T` is aligned to `N` (the rest of the elements follow without
    // padding).
    //
    // Requires: `N >= alignof(T)` and `N` is a power of 2.
    template <class T, size_t N>
    struct Aligned;

    namespace internal_layout {

        template <class T>
        struct NotAligned { };

        template <class T, size_t N>
        struct NotAligned<const Aligned<T, N>> {
            static_assert(sizeof(T) == 0, "Aligned<T, N> cannot be const-qualified");
        };

        template <size_t>
        using IntToSize = size_t;

        template <class T>
        struct Type : NotAligned<T> {
            using type = T;
        };

        template <class T, size_t N>
        struct Type<Aligned<T, N>> {
            using type = T;
        };

        template <class T>
        struct SizeOf : NotAligned<T>, std::integral_constant<size_t, sizeof(T)> { };

        template <class T, size_t N>
        struct SizeOf<Aligned<T, N>> : std::integral_constant<size_t, sizeof(T)> { };

        // Note: workaround for https://gcc.gnu.org/PR88115
        template <class T>
        struct AlignOf : NotAligned<T> {
            static constexpr size_t value = alignof(T);
        };

        template <class T, size_t N>
        struct AlignOf<Aligned<T, N>> {
            static_assert(N % alignof(T) == 0,
                "Custom alignment can't be lower than the type's alignment");
            static constexpr size_t value = N;
        };

        // Does `Ts...` contain `T`?
        template <class T, class... Ts>
        using Contains = std::disjunction<std::is_same<T, Ts>...>;

        template <class From, class To>
        using CopyConst = std::conditional_t<std::is_const_v<From>, const To, To>;

        // Note: We're not qualifying this with turbo:: because it doesn't compile under
        // MSVC.
        template <class T>
        using SliceType = turbo::Span<T>;

        // This namespace contains no types. It prevents functions defined in it from
        // being found by ADL.
        namespace adl_barrier {

            template <class Needle, class... Ts>
            constexpr size_t Find(Needle, Needle, Ts...) {
                static_assert(!Contains<Needle, Ts...>(), "Duplicate element type");
                return 0;
            }

            template <class Needle, class T, class... Ts>
            constexpr size_t Find(Needle, T, Ts...) {
                return adl_barrier::Find(Needle(), Ts()...) + 1;
            }

            constexpr bool IsPow2(size_t n) {
                return !(n & (n - 1));
            }

            // Returns `q * m` for the smallest `q` such that `q * m >= n`.
            // Requires: `m` is a power of two. It's enforced by IsLegalElementType below.
            constexpr size_t Align(size_t n, size_t m) {
                return (n + m - 1) & ~(m - 1);
            }

            constexpr size_t Min(size_t a, size_t b) {
                return b < a ? b : a;
            }

            constexpr size_t Max(size_t a) {
                return a;
            }

            template <class... Ts>
            constexpr size_t Max(size_t a, size_t b, Ts... rest) {
                return adl_barrier::Max(b < a ? a : b, rest...);
            }

            template <class T>
            std::string TypeName() {
                std::string out;
#if KUMO_HAVE_RTTI
                turbo::str_append(&out, "<",
                    turbo::debugging_internal::DemangleString(typeid(T).name()),
                    ">");
#endif
                return out;
            }

        } // namespace adl_barrier

        // Can `T` be a template argument of `Layout`?
        template <class T>
        using IsLegalElementType = std::integral_constant<bool,
            !std::is_reference_v<T> && !std::is_volatile_v<T> && !std::is_reference_v<typename Type<T>::type> && !std::is_volatile_v<typename Type<T>::type> && adl_barrier::IsPow2(AlignOf<T>::value)>;

        template <class Elements, class StaticSizeSeq, class RuntimeSizeSeq,
            class SizeSeq, class OffsetSeq>
        class LayoutImpl;

        // Public base class of `Layout` and the result type of `Layout::Partial()`.
        //
        // `Elements...` contains all template arguments of `Layout` that created this
        // instance.
        //
        // `StaticSizeSeq...` is an index_sequence containing the sizes specified at
        // compile-time.
        //
        // `RuntimeSizeSeq...` is `[0, NumRuntimeSizes)`, where `NumRuntimeSizes` is the
        // number of arguments passed to `Layout::Partial()` or `Layout::Layout()`.
        //
        // `SizeSeq...` is `[0, NumSizes)` where `NumSizes` is `NumRuntimeSizes` plus
        // the number of sizes in `StaticSizeSeq`.
        //
        // `OffsetSeq...` is `[0, NumOffsets)` where `NumOffsets` is
        // `Min(sizeof...(Elements), NumSizes + 1)` (the number of arrays for which we
        // can compute offsets).
        template <class... Elements, size_t... StaticSizeSeq, size_t... RuntimeSizeSeq,
            size_t... SizeSeq, size_t... OffsetSeq>
        class LayoutImpl<std::tuple<Elements...>, std::index_sequence<StaticSizeSeq...>,
            std::index_sequence<RuntimeSizeSeq...>,
            std::index_sequence<SizeSeq...>,
            std::index_sequence<OffsetSeq...>> {
        private:
            static_assert(sizeof...(Elements) > 0, "At least one field is required");
            static_assert(std::conjunction_v<IsLegalElementType<Elements>...>,
                "Invalid element type (see IsLegalElementType)");
            static_assert(sizeof...(StaticSizeSeq) <= sizeof...(Elements),
                "Too many static sizes specified");

            enum {
                NumTypes = sizeof...(Elements),
                NumStaticSizes = sizeof...(StaticSizeSeq),
                NumRuntimeSizes = sizeof...(RuntimeSizeSeq),
                NumSizes = sizeof...(SizeSeq),
                NumOffsets = sizeof...(OffsetSeq),
            };

            // These are guaranteed by `Layout`.
            static_assert(NumStaticSizes + NumRuntimeSizes == NumSizes, "Internal error");
            static_assert(NumSizes <= NumTypes, "Internal error");
            static_assert(NumOffsets == adl_barrier::Min(NumTypes, NumSizes + 1),
                "Internal error");
            static_assert(NumTypes > 0, "Internal error");

            static constexpr std::array<size_t, sizeof...(StaticSizeSeq)> kStaticSizes = {
                StaticSizeSeq...
            };

            // Returns the index of `T` in `Elements...`. Results in a compilation error
            // if `Elements...` doesn't contain exactly one instance of `T`.
            template <class T>
            static constexpr size_t ElementIndex() {
                static_assert(Contains<Type<T>, Type<typename Type<Elements>::type>...>(),
                    "Type not found");
                return adl_barrier::Find(Type<T>(),
                    Type<typename Type<Elements>::type>()...);
            }

            template <size_t N>
            using ElementAlignment = AlignOf<typename std::tuple_element<N, std::tuple<Elements...>>::type>;

        public:
            // Element types of all arrays packed in a tuple.
            using ElementTypes = std::tuple<typename Type<Elements>::type...>;

            // Element type of the Nth array.
            template <size_t N>
            using ElementType = typename std::tuple_element<N, ElementTypes>::type;

            constexpr explicit LayoutImpl(IntToSize<RuntimeSizeSeq>... sizes)
                : size_ { sizes... } { }

            // Alignment of the layout, equal to the strictest alignment of all elements.
            // All pointers passed to the methods of layout must be aligned to this value.
            static constexpr size_t Alignment() {
                return adl_barrier::Max(AlignOf<Elements>::value...);
            }

            // Offset in bytes of the Nth array.
            //
            //   // int[3], 4 bytes of padding, double[4].
            //   Layout<int, double> x(3, 4);
            //   assert(x.Offset<0>() == 0);   // The ints starts from 0.
            //   assert(x.Offset<1>() == 16);  // The doubles starts from 16.
            //
            // Requires: `N <= NumSizes && N < sizeof...(Ts)`.
            template <size_t N>
            constexpr size_t Offset() const {
                if constexpr (N == 0) {
                    return 0;
                } else {
                    static_assert(N < NumOffsets, "Index out of bounds");
                    return adl_barrier::Align(
                        Offset<N - 1>() + SizeOf<ElementType<N - 1>>::value * Size<N - 1>(),
                        ElementAlignment<N>::value);
                }
            }

            // Offset in bytes of the array with the specified element type. There must
            // be exactly one such array and its zero-based index must be at most
            // `NumSizes`.
            //
            //   // int[3], 4 bytes of padding, double[4].
            //   Layout<int, double> x(3, 4);
            //   assert(x.Offset<int>() == 0);      // The ints starts from 0.
            //   assert(x.Offset<double>() == 16);  // The doubles starts from 16.
            template <class T>
            constexpr size_t Offset() const {
                return Offset<ElementIndex<T>()>();
            }

            // Offsets in bytes of all arrays for which the offsets are known.
            constexpr std::array<size_t, NumOffsets> Offsets() const {
                return { { Offset<OffsetSeq>()... } };
            }

            // The number of elements in the Nth array (zero-based).
            //
            //   // int[3], 4 bytes of padding, double[4].
            //   Layout<int, double> x(3, 4);
            //   assert(x.Size<0>() == 3);
            //   assert(x.Size<1>() == 4);
            //
            // Requires: `N < NumSizes`.
            template <size_t N>
            constexpr size_t Size() const {
                if constexpr (N < NumStaticSizes) {
                    return kStaticSizes[N];
                } else {
                    static_assert(N < NumSizes, "Index out of bounds");
                    return size_[N - NumStaticSizes];
                }
            }

            // The number of elements in the array with the specified element type.
            // There must be exactly one such array and its zero-based index must be
            // at most `NumSizes`.
            //
            //   // int[3], 4 bytes of padding, double[4].
            //   Layout<int, double> x(3, 4);
            //   assert(x.Size<int>() == 3);
            //   assert(x.Size<double>() == 4);
            template <class T>
            constexpr size_t Size() const {
                return Size<ElementIndex<T>()>();
            }

            // The number of elements of all arrays for which they are known.
            constexpr std::array<size_t, NumSizes> Sizes() const {
                return { { Size<SizeSeq>()... } };
            }

            // Pointer to the beginning of the Nth array.
            //
            // `Char` must be `[const] [signed|unsigned] char`.
            //
            //   // int[3], 4 bytes of padding, double[4].
            //   Layout<int, double> x(3, 4);
            //   unsigned char* p = new unsigned char[x.AllocSize()];
            //   int* ints = x.Pointer<0>(p);
            //   double* doubles = x.Pointer<1>(p);
            //
            // Requires: `N <= NumSizes && N < sizeof...(Ts)`.
            // Requires: `p` is aligned to `Alignment()`.
            template <size_t N, class Char>
            CopyConst<Char, ElementType<N>>* Pointer(Char* p) const {
                using C = std::remove_const_t<Char>;
                static_assert(
                    std::is_same<C, char>() || std::is_same<C, unsigned char>() || std::is_same<C, signed char>(),
                    "The argument must be a pointer to [const] [signed|unsigned] char");
                constexpr size_t alignment = Alignment();
                (void)alignment;
                assert(reinterpret_cast<uintptr_t>(p) % alignment == 0);
                return reinterpret_cast<CopyConst<Char, ElementType<N>>*>(p + Offset<N>());
            }

            // Pointer to the beginning of the array with the specified element type.
            // There must be exactly one such array and its zero-based index must be at
            // most `NumSizes`.
            //
            // `Char` must be `[const] [signed|unsigned] char`.
            //
            //   // int[3], 4 bytes of padding, double[4].
            //   Layout<int, double> x(3, 4);
            //   unsigned char* p = new unsigned char[x.AllocSize()];
            //   int* ints = x.Pointer<int>(p);
            //   double* doubles = x.Pointer<double>(p);
            //
            // Requires: `p` is aligned to `Alignment()`.
            template <class T, class Char>
            CopyConst<Char, T>* Pointer(Char* p) const {
                return Pointer<ElementIndex<T>()>(p);
            }

            // Pointers to all arrays for which pointers are known.
            //
            // `Char` must be `[const] [signed|unsigned] char`.
            //
            //   // int[3], 4 bytes of padding, double[4].
            //   Layout<int, double> x(3, 4);
            //   unsigned char* p = new unsigned char[x.AllocSize()];
            //
            //   int* ints;
            //   double* doubles;
            //   std::tie(ints, doubles) = x.Pointers(p);
            //
            // Requires: `p` is aligned to `Alignment()`.
            template <class Char>
            auto Pointers(Char* p) const {
                return std::tuple<CopyConst<Char, ElementType<OffsetSeq>>*...>(
                    Pointer<OffsetSeq>(p)...);
            }

            // The Nth array.
            //
            // `Char` must be `[const] [signed|unsigned] char`.
            //
            //   // int[3], 4 bytes of padding, double[4].
            //   Layout<int, double> x(3, 4);
            //   unsigned char* p = new unsigned char[x.AllocSize()];
            //   Span<int> ints = x.Slice<0>(p);
            //   Span<double> doubles = x.Slice<1>(p);
            //
            // Requires: `N < NumSizes`.
            // Requires: `p` is aligned to `Alignment()`.
            template <size_t N, class Char>
            SliceType<CopyConst<Char, ElementType<N>>> Slice(Char* p) const {
                return SliceType<CopyConst<Char, ElementType<N>>>(Pointer<N>(p), Size<N>());
            }

            // The array with the specified element type. There must be exactly one
            // such array and its zero-based index must be less than `NumSizes`.
            //
            // `Char` must be `[const] [signed|unsigned] char`.
            //
            //   // int[3], 4 bytes of padding, double[4].
            //   Layout<int, double> x(3, 4);
            //   unsigned char* p = new unsigned char[x.AllocSize()];
            //   Span<int> ints = x.Slice<int>(p);
            //   Span<double> doubles = x.Slice<double>(p);
            //
            // Requires: `p` is aligned to `Alignment()`.
            template <class T, class Char>
            SliceType<CopyConst<Char, T>> Slice(Char* p) const {
                return Slice<ElementIndex<T>()>(p);
            }

            // All arrays with known sizes.
            //
            // `Char` must be `[const] [signed|unsigned] char`.
            //
            //   // int[3], 4 bytes of padding, double[4].
            //   Layout<int, double> x(3, 4);
            //   unsigned char* p = new unsigned char[x.AllocSize()];
            //
            //   Span<int> ints;
            //   Span<double> doubles;
            //   std::tie(ints, doubles) = x.Slices(p);
            //
            // Requires: `p` is aligned to `Alignment()`.
            //
            // Note: We mark the parameter as maybe_unused because GCC detects it is not
            // used when `SizeSeq` is empty [-Werror=unused-but-set-parameter].
            template <class Char>
            auto Slices([[maybe_unused]] Char* p) const {
                return std::tuple<SliceType<CopyConst<Char, ElementType<SizeSeq>>>...>(
                    Slice<SizeSeq>(p)...);
            }

            // The size of the allocation that fits all arrays.
            //
            //   // int[3], 4 bytes of padding, double[4].
            //   Layout<int, double> x(3, 4);
            //   unsigned char* p = new unsigned char[x.AllocSize()];  // 48 bytes
            //
            // Requires: `NumSizes == sizeof...(Ts)`.
            constexpr size_t AllocSize() const {
                static_assert(NumTypes == NumSizes, "You must specify sizes of all fields");
                return Offset<NumTypes - 1>() + SizeOf<ElementType<NumTypes - 1>>::value * Size<NumTypes - 1>();
            }

            // If built with --config=asan, poisons padding bytes (if any) in the
            // allocation. The pointer must point to a memory block at least
            // `AllocSize()` bytes in length.
            //
            // `Char` must be `[const] [signed|unsigned] char`.
            //
            // Requires: `p` is aligned to `Alignment()`.
            template <class Char, size_t N = NumOffsets - 1>
            void PoisonPadding(const Char* p) const {
                if constexpr (N == 0) {
                    Pointer<0>(p); // verify the requirements on `Char` and `p`
                } else {
                    static_assert(N < NumOffsets, "Index out of bounds");
                    (void)p;
#if KUMO_HAVE_ADDRESS_SANITIZER
                    PoisonPadding<Char, N - 1>(p);
                    // The `if` is an optimization. It doesn't affect the observable behaviour.
                    if (ElementAlignment<N - 1>::value % ElementAlignment<N>::value) {
                        size_t start = Offset<N - 1>() + SizeOf<ElementType<N - 1>>::value * Size<N - 1>();
                        ASAN_POISON_MEMORY_REGION(p + start, Offset<N>() - start);
                    }
#endif
                }
            }

            // Human-readable description of the memory layout. Useful for debugging.
            // Slow.
            //
            //   // char[5], 3 bytes of padding, int[3], 4 bytes of padding, followed
            //   // by an unknown number of doubles.
            //   auto x = Layout<char, int, double>::Partial(5, 3);
            //   assert(x.DebugString() ==
            //          "@0<char>(1)[5]; @8<int>(4)[3]; @24<double>(8)");
            //
            // Each field is in the following format: @offset<type>(sizeof)[size] (<type>
            // may be missing depending on the target platform). For example,
            // @8<int>(4)[3] means that at offset 8 we have an array of ints, where each
            // int is 4 bytes, and we have 3 of those ints. The size of the last field may
            // be missing (as in the example above). Only fields with known offsets are
            // described. Type names may differ across platforms: one compiler might
            // produce "unsigned*" where another produces "unsigned int *".
            std::string DebugString() const {
                const auto offsets = Offsets();
                const size_t sizes[] = { SizeOf<ElementType<OffsetSeq>>::value... };
                const std::string types[] = {
                    adl_barrier::TypeName<ElementType<OffsetSeq>>()...
                };
                std::string res = turbo::str_cat("@0", types[0], "(", sizes[0], ")");
                for (size_t i = 0; i != NumOffsets - 1; ++i) {
                    turbo::str_append(&res, "[", DebugSize(i), "]; @", offsets[i + 1],
                        types[i + 1], "(", sizes[i + 1], ")");
                }
                // NumSizes is a constant that may be zero. Some compilers cannot see that
                // inside the if statement "size_[NumSizes - 1]" must be valid.
                int last = static_cast<int>(NumSizes) - 1;
                if (NumTypes == NumSizes && last >= 0) {
                    turbo::str_append(&res, "[", DebugSize(static_cast<size_t>(last)), "]");
                }
                return res;
            }

        private:
            size_t DebugSize(size_t n) const {
                if (n < NumStaticSizes) {
                    return kStaticSizes[n];
                } else {
                    return size_[n - NumStaticSizes];
                }
            }

            // Arguments of `Layout::Partial()` or `Layout::Layout()`.
            size_t size_[NumRuntimeSizes > 0 ? NumRuntimeSizes : 1];
        };

        template <class StaticSizeSeq, size_t NumRuntimeSizes, class... Ts>
        using LayoutType = LayoutImpl<
            std::tuple<Ts...>, StaticSizeSeq, std::make_index_sequence<NumRuntimeSizes>,
            std::make_index_sequence<NumRuntimeSizes + StaticSizeSeq::size()>,
            std::make_index_sequence<adl_barrier::Min(
                sizeof...(Ts), NumRuntimeSizes + StaticSizeSeq::size() + 1)>>;

        template <class StaticSizeSeq, class... Ts>
        class LayoutWithStaticSizes
            : public LayoutType<StaticSizeSeq,
                  sizeof...(Ts) - adl_barrier::Min(sizeof...(Ts), StaticSizeSeq::size()),
                  Ts...> {
        private:
            using Super = LayoutType<StaticSizeSeq,
                sizeof...(Ts) - adl_barrier::Min(sizeof...(Ts), StaticSizeSeq::size()),
                Ts...>;

        public:
            // The result type of `Partial()` with `NumSizes` arguments.
            template <size_t NumSizes>
            using PartialType = internal_layout::LayoutType<StaticSizeSeq, NumSizes, Ts...>;

            // `Layout` knows the element types of the arrays we want to lay out in
            // memory but not the number of elements in each array.
            // `Partial(size1, ..., sizeN)` allows us to specify the latter. The
            // resulting immutable object can be used to obtain pointers to the
            // individual arrays.
            //
            // It's allowed to pass fewer array sizes than the number of arrays. E.g.,
            // if all you need is to the offset of the second array, you only need to
            // pass one argument -- the number of elements in the first array.
            //
            //   // int[3] followed by 4 bytes of padding and an unknown number of
            //   // doubles.
            //   auto x = Layout<int, double>::Partial(3);
            //   // doubles start at byte 16.
            //   assert(x.Offset<1>() == 16);
            //
            // If you know the number of elements in all arrays, you can still call
            // `Partial()` but it's more convenient to use the constructor of `Layout`.
            //
            //   Layout<int, double> x(3, 5);
            //
            // Note: The sizes of the arrays must be specified in number of elements,
            // not in bytes.
            //
            // Requires: `sizeof...(Sizes) + NumStaticSizes <= sizeof...(Ts)`.
            // Requires: all arguments are convertible to `size_t`.
            template <class... Sizes>
            static constexpr PartialType<sizeof...(Sizes)> Partial(Sizes&&... sizes) {
                static_assert(sizeof...(Sizes) + StaticSizeSeq::size() <= sizeof...(Ts),
                    "");
                return PartialType<sizeof...(Sizes)>(
                    static_cast<size_t>(std::forward<Sizes>(sizes))...);
            }

            // Inherit LayoutType's constructor.
            //
            // Creates a layout with the sizes of all arrays specified. If you know
            // only the sizes of the first N arrays (where N can be zero), you can use
            // `Partial()` defined above. The constructor is essentially equivalent to
            // calling `Partial()` and passing in all array sizes; the constructor is
            // provided as a convenient abbreviation.
            //
            // Note: The sizes of the arrays must be specified in number of elements,
            // not in bytes.
            //
            // Implementation note: we do this via a `using` declaration instead of
            // defining our own explicit constructor because the signature of LayoutType's
            // constructor depends on RuntimeSizeSeq, which we don't have access to here.
            // If we defined our own constructor here, it would have to use a parameter
            // pack and then cast the arguments to size_t when calling the superclass
            // constructor, similar to what Partial() does. But that would suffer from the
            // same problem that Partial() has, which is that the parameter types are
            // inferred from the arguments, which may be signed types, which must then be
            // cast to size_t. This can lead to negative values being silently (i.e. with
            // no compiler warnings) cast to an unsigned type. Having a constructor with
            // size_t parameters helps the compiler generate better warnings about
            // potential bad casts, while avoiding false warnings when positive literal
            // arguments are used. If an argument is a positive literal integer (e.g.
            // `1`), the compiler will understand that it can be safely converted to
            // size_t, and hence not generate a warning. But if a negative literal (e.g.
            // `-1`) or a variable with signed type is used, then it can generate a
            // warning about a potentially unsafe implicit cast. It would be great if we
            // could do this for Partial() too, but unfortunately as of C++23 there seems
            // to be no way to define a function with a variable number of parameters of a
            // certain type, a.k.a. homogeneous function parameter packs. So we're forced
            // to choose between explicitly casting the arguments to size_t, which
            // suppresses all warnings, even potentially valid ones, or implicitly casting
            // them to size_t, which generates bogus warnings whenever literal arguments
            // are used, even if they're positive.
            using Super::Super;
        };

    } // namespace internal_layout

    // Descriptor of arrays of various types and sizes laid out in memory one after
    // another. See the top of the file for documentation.
    //
    // Check out the public API of internal_layout::LayoutWithStaticSizes and
    // internal_layout::LayoutImpl above. Those types are internal to the library
    // but their methods are public, and they are inherited by `Layout`.
    template <class... Ts>
    class Layout
        : public internal_layout::LayoutWithStaticSizes<std::make_index_sequence<0>,
              Ts...> {
    private:
        using Super = internal_layout::LayoutWithStaticSizes<std::make_index_sequence<0>,
            Ts...>;

    public:
        // If you know the sizes of some or all of the arrays at compile time, you can
        // use `WithStaticSizes` or `WithStaticSizeSequence` to create a `Layout` type
        // with those sizes baked in. This can help the compiler generate optimal code
        // for calculating array offsets and AllocSize().
        //
        // Like `Partial()`, the N sizes you specify are for the first N arrays, and
        // they specify the number of elements in each array, not the number of bytes.
        template <class StaticSizeSeq>
        using WithStaticSizeSequence = internal_layout::LayoutWithStaticSizes<StaticSizeSeq, Ts...>;

        template <size_t... StaticSizes>
        using WithStaticSizes = WithStaticSizeSequence<std::index_sequence<StaticSizes...>>;

        // Inherit LayoutWithStaticSizes's constructor, which requires you to specify
        // all the array sizes.
        using Super::Super;
    };

} // namespace fermat::base_internal
