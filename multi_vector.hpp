#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <new>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

template <typename... Ts>
class multi_vector {

    static_assert(sizeof...(Ts) > 0, "multi_vector requires at least one type");

    template <typename T, typename... Us>
    struct index_of;

    template <typename T, typename... Us>
    struct index_of<T, T, Us...> : std::integral_constant<std::size_t, 0> {};

    template <typename T, typename U, typename... Us>
    struct index_of<T, U, Us...>
        : std::integral_constant<std::size_t, 1 + index_of<T, Us...>::value> {};

    template <typename T>
    static constexpr std::size_t idx_v = index_of<T, Ts...>::value;

    template <std::size_t I>
    using type_at = std::tuple_element_t<I, std::tuple<Ts...>>;

    static constexpr std::size_t N = sizeof...(Ts);
    using type_tuple = std::tuple<Ts...>;

    static constexpr std::array<std::size_t, N> type_sizes_{sizeof(Ts)...};
    static constexpr std::array<std::size_t, N> type_aligns_{alignof(Ts)...};
    static constexpr std::size_t block_align_ = std::max({alignof(Ts)...});

    static constexpr std::size_t align_up(std::size_t value, std::size_t alignment) {
        return alignment ? (value + (alignment - 1)) & ~(alignment - 1) : value;
    }

    template <std::size_t... Is>
    void destroy_elements(std::index_sequence<Is...>) {
        (destroy_elements_at<Is>(), ...);
    }

    template <std::size_t I>
    void destroy_elements_at() {
        using T = type_at<I>;
        if constexpr (!std::is_trivially_destructible_v<T>) {
            T* ptr = static_cast<T*>(data_ptrs_[I]);
            for (std::size_t j = 0; j < sizes_[I]; ++j) {
                ptr[j].~T();
            }
        }
    }

    void* data_ptrs_[N]{};
    std::size_t sizes_[N]{};
    std::size_t capacities_[N]{};
    void* block_ = nullptr;
    std::size_t block_size_ = 0;

    friend struct builder;

public:
    multi_vector() noexcept = default;

    ~multi_vector() {
        if (!block_) return;
        destroy_elements(std::make_index_sequence<N>{});
        ::operator delete(block_, std::align_val_t{block_align_});
    }

    multi_vector(multi_vector&& other) noexcept
        : block_(other.block_), block_size_(other.block_size_)
    {
        for (std::size_t i = 0; i < N; ++i) {
            data_ptrs_[i] = other.data_ptrs_[i];
            sizes_[i] = other.sizes_[i];
            capacities_[i] = other.capacities_[i];
            other.data_ptrs_[i] = nullptr;
            other.sizes_[i] = 0;
            other.capacities_[i] = 0;
        }
        other.block_ = nullptr;
        other.block_size_ = 0;
    }

    template <typename T>
    std::size_t size() const {
        static_assert((std::is_same_v<T, Ts> || ...), "T must be in multi_vector");
        return sizes_[idx_v<T>];
    }

    template <std::size_t idx>
    std::size_t size() const {
        static_assert(idx < N, "Index out of bounds");
        return sizes_[idx];
    }

    template <typename T>
    T* data() const {
        static_assert((std::is_same_v<T, Ts> || ...), "T must be in multi_vector");
        return static_cast<T*>(data_ptrs_[idx_v<T>]);
    }

    template <std::size_t idx>
    auto data() const -> type_at<idx>* {
        static_assert(idx < N, "Index out of bounds");
        return static_cast<type_at<idx>*>(data_ptrs_[idx]);
    }

    template <typename T>
    std::size_t capacity() const {
        static_assert((std::is_same_v<T, Ts> || ...), "T must be in multi_vector");
        return capacities_[idx_v<T>];
    }

    template <std::size_t idx>
    std::size_t capacity() const {
        static_assert(idx < N, "Index out of bounds");
        return capacities_[idx];
    }

    template <typename T>
    void push_back(const T& value) {
        static_assert((std::is_same_v<T, Ts> || ...), "T must be in multi_vector");
        if (size<T>() >= capacity<T>()) {
            throw std::length_error("multi_vector capacity exceeded for this type");
        }
        ::new (static_cast<void*>(data<T>() + size<T>())) T(value);
        sizes_[idx_v<T>]++;
    }

    template <std::size_t idx>
    void push_back(const type_at<idx>& value) {
        static_assert(idx < N, "Index out of bounds");
        if (size<idx>() >= capacity<idx>()) {
            throw std::length_error("multi_vector capacity exceeded for this type");
        }
        ::new (static_cast<void*>(data<idx>() + size<idx>())) type_at<idx>(value);
        sizes_[idx]++;
    }

    struct builder {
        std::array<std::size_t, N> caps_{};
        std::tuple<std::optional<Ts>...> defaults_;

        template <typename T>
        builder& capacity(std::size_t cap) {
            static_assert((std::is_same_v<T, Ts> || ...), "T must be in multi_vector");
            caps_[idx_v<T>] = cap;
            return *this;
        }

        template <std::size_t idx>
        builder& capacity(std::size_t cap) {
            static_assert(idx < N, "Index out of bounds");
            caps_[idx] = cap;
            return *this;
        }

        template <typename T>
        builder& default_value(const T& value) {
            static_assert((std::is_same_v<T, Ts> || ...), "T must be in multi_vector");
            std::get<idx_v<T>>(defaults_) = value;
            return *this;
        }

        template <std::size_t idx>
        builder& default_value(const type_at<idx>& value) {
            static_assert(idx < N, "Index out of bounds");
            std::get<idx>(defaults_) = value;
            return *this;
        }

    private:
        template <std::size_t... Is>
        void init_defaults(multi_vector& mv, std::index_sequence<Is...>) const {
            (init_default_at<Is>(mv), ...);
        }

        template <std::size_t I>
        void init_default_at(multi_vector& mv) const {
            const auto& opt_default = std::get<I>(defaults_);
            if (opt_default.has_value()) {
                using T = type_at<I>;
                T* ptr = static_cast<T*>(mv.data_ptrs_[I]);
                for (std::size_t j = 0; j < caps_[I]; ++j) {
                    ::new (static_cast<void*>(ptr + j)) T(opt_default.value());
                }
                mv.sizes_[I] = caps_[I];
            }
        }

    public:
        multi_vector build() const {
            multi_vector mv{};
            std::array<std::size_t, N> offsets{};
            std::size_t off = 0;
            for (std::size_t i = 0; i < N; ++i) {
                off = align_up(off, type_aligns_[i]);
                offsets[i] = off;
                off += caps_[i] * type_sizes_[i];
            }

            void* block = ::operator new(off, std::align_val_t{block_align_});
            mv.block_ = block;
            if (block) {
                for (std::size_t i = 0; i < N; ++i) {
                    mv.data_ptrs_[i] = static_cast<void*>(reinterpret_cast<std::byte*>(block) + offsets[i]);
                    mv.capacities_[i] = caps_[i];
                }
                mv.block_size_ = off;
                init_defaults(mv, std::make_index_sequence<N>{});
            }

            return mv;
        }
    };
};