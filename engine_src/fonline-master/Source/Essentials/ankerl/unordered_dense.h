#pragma once
// Tiny compatibility shim for ankerl::unordered_dense.
//
// Why this exists:
// - The engine expects <ankerl/unordered_dense.h>
// - On some source drops/submodules this header is missing
// - We only need a small subset (map/set + segmented_* aliases + wyhash helper)
//
// IMPORTANT:
// This is NOT the real ankerl::unordered_dense implementation.
// It is a minimal drop-in to make the project compile on Android.
// If you later vendor the real library, delete this shim.

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace ankerl::unordered_dense
{
    // Basic aliases (backed by std containers).
    template <class Key,
              class T,
              class Hash = std::hash<Key>,
              class KeyEqual = std::equal_to<Key>,
              class Allocator = std::allocator<std::pair<const Key, T>>>
    using map = std::unordered_map<Key, T, Hash, KeyEqual, Allocator>;

    template <class Key,
              class Hash = std::hash<Key>,
              class KeyEqual = std::equal_to<Key>,
              class Allocator = std::allocator<Key>>
    using set = std::unordered_set<Key, Hash, KeyEqual, Allocator>;

    // The engine code uses segmented_* types. For this shim we alias them.
    template <class Key,
              class T,
              class Hash = std::hash<Key>,
              class KeyEqual = std::equal_to<Key>,
              class Allocator = std::allocator<std::pair<const Key, T>>>
    using segmented_map = map<Key, T, Hash, KeyEqual, Allocator>;

    template <class Key,
              class Hash = std::hash<Key>,
              class KeyEqual = std::equal_to<Key>,
              class Allocator = std::allocator<Key>>
    using segmented_set = set<Key, Hash, KeyEqual, Allocator>;

    // Some engine files reference an internal helper:
    //   ankerl::unordered_dense::detail::wyhash::hash(...)
    // We provide a lightweight, deterministic 64-bit hasher with the same API.
    namespace detail
    {
        struct wyhash
        {
            static constexpr uint64_t mix(uint64_t x) noexcept
            {
                // splitmix64 mix (fast avalanche)
                x += 0x9E3779B97F4A7C15ull;
                x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
                x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
                return x ^ (x >> 31);
            }

            static constexpr uint64_t hash(uint64_t x) noexcept
            {
                return mix(x);
            }

            static inline uint64_t hash(void const* data, std::size_t len) noexcept
            {
                // FNV-1a 64-bit, then mix for better avalanche.
                auto const* p = static_cast<unsigned char const*>(data);
                uint64_t h = 1469598103934665603ull;
                for (std::size_t i = 0; i < len; ++i) {
                    h ^= static_cast<uint64_t>(p[i]);
                    h *= 1099511628211ull;
                }
                return mix(h ^ static_cast<uint64_t>(len));
            }
        };
    } // namespace detail
} // namespace ankerl::unordered_dense
