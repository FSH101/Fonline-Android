//      __________        ___               ______            _
//     / ____/ __ \____  / (_)___  ___     / ____/___  ____ _(_)___  ___
//    / /_  / / / / __ \/ / / __ \/ _ \   / __/ / __ \/ __ `/ / __ \/ _ `
//   / __/ / /_/ / / / / / / / / /  __/  / /___/ / / / /_/ / / / / /  __/
//  /_/    \____/_/ /_/_/_/_/ /_/\___/  /_____/_/ /_/\__, /_/_/ /_/\___/
//                                                  /____/
// FOnline Engine
// https://fonline.ru
// https://github.com/cvet/fonline
//
// MIT License
//
// Copyright (c) 2006 - 2025, Anton Tsvetinskiy aka cvet <cvet@tut.by>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#pragma once

#include "BasicCore.h"
#include "ExceptionHadling.h"

FO_BEGIN_NAMESPACE();

// Float comparator
template<typename T>
    requires(std::floating_point<T>)
[[nodiscard]] constexpr auto float_abs(T f) noexcept -> T
{
    return f < 0 ? -f : f;
}

template<typename T>
    requires(std::floating_point<T>)
[[nodiscard]] constexpr auto is_float_equal(T f1, T f2, T epsilon = 1.0e-5f) noexcept -> bool
{
    if (float_abs(f1 - f2) <= epsilon) {
        return true;
    }
    return float_abs(f1 - f2) <= epsilon * std::max(float_abs(f1), float_abs(f2));
}

// Numeric cast
FO_DECLARE_EXCEPTION(OverflowException);
FO_DECLARE_EXCEPTION(DivisionByZeroException);

#ifndef FO_SAFE_ARITH_RELAXED
#    if defined(__ANDROID__)
#        define FO_SAFE_ARITH_RELAXED 1
#    else
#        define FO_SAFE_ARITH_RELAXED 0
#    endif
#endif

namespace detail
{
    template<typename...>
    inline constexpr bool dependent_false_v = false;

    template<typename T, typename Enable = void>
    struct enum_underlying
    {
        using type = std::remove_cvref_t<T>;
        static constexpr bool valid = std::is_arithmetic_v<type>;
    };

    template<typename T>
    struct enum_underlying<T, std::enable_if_t<std::is_enum_v<std::remove_cvref_t<T>>>>
    {
        using type = std::underlying_type_t<std::remove_cvref_t<T>>;
        static constexpr bool valid = std::is_arithmetic_v<type>;
    };

    template<typename T>
    struct numeric_value
    {
        using enum_helper = enum_underlying<T>;
        using type = typename enum_helper::type;
        static constexpr bool valid = enum_helper::valid;
    };

    template<typename T>
    using numeric_value_t = typename numeric_value<T>::type;

    template<typename T>
    concept numeric_like = numeric_value<T>::valid;

    template<numeric_like T, numeric_like U>
    [[nodiscard]] constexpr auto cast_with_range(U value) -> T
    {
        using To = numeric_value_t<T>;
        using From = numeric_value_t<U>;

        static_assert(!std::same_as<To, bool> && !std::same_as<From, bool>, "Bool type is not convertible");

        if constexpr (std::floating_point<To> || std::floating_point<From>) {
#if !FO_SAFE_ARITH_RELAXED
            static_assert(std::floating_point<To>, "Use iround for float to int conversion");
#endif
        }

        using Common = std::common_type_t<To, From>;
        constexpr auto target_min = static_cast<Common>(std::numeric_limits<To>::lowest());
        constexpr auto target_max = static_cast<Common>(std::numeric_limits<To>::max());
        const auto common_value = static_cast<Common>(value);

        if (common_value < target_min || common_value > target_max) {
            throw OverflowException("Numeric cast overflow", typeid(U).name(), typeid(T).name(), common_value, target_max);
        }

        return static_cast<T>(static_cast<To>(common_value));
    }

    template<numeric_like T, numeric_like U>
    [[nodiscard]] constexpr auto clamp_with_range(U value) noexcept -> T
    {
        using To = numeric_value_t<T>;
        using From = numeric_value_t<U>;

        static_assert(!std::same_as<To, bool> && !std::same_as<From, bool>, "Bool type is not convertible");

        if constexpr (std::floating_point<To> || std::floating_point<From>) {
#if !FO_SAFE_ARITH_RELAXED
            static_assert(std::floating_point<To>, "Use iround for float to int conversion");
#endif
        }

        using Common = std::common_type_t<To, From>;
        constexpr auto target_min = static_cast<Common>(std::numeric_limits<To>::lowest());
        constexpr auto target_max = static_cast<Common>(std::numeric_limits<To>::max());

        auto common_value = static_cast<Common>(value);
        if (common_value < target_min) {
            common_value = target_min;
        }
        else if (common_value > target_max) {
            common_value = target_max;
        }

        return static_cast<T>(static_cast<To>(common_value));
    }
}

template<detail::numeric_like T, detail::numeric_like U>
[[nodiscard]] constexpr auto numeric_cast(U value) -> T
{
    return detail::cast_with_range<T>(value);
}

template<detail::numeric_like T, detail::numeric_like U>
[[nodiscard]] constexpr auto safe_numeric_cast(U value) noexcept -> T
{
    return detail::clamp_with_range<T>(value);
}

template<typename T, typename U>
[[nodiscard]] consteval auto const_numeric_cast(U value) noexcept -> T // NOLINT(bugprone-exception-escape)
{
    return numeric_cast<T>(value);
}

template<typename T, typename U>
    requires(std::is_arithmetic_v<T> && std::is_arithmetic_v<U>)
[[nodiscard]] constexpr auto clamp_to(U value) noexcept -> T
{
    static_assert(!std::same_as<T, bool> && !std::same_as<U, bool>, "Bool type is not convertible");

    if constexpr (std::floating_point<T> || std::floating_point<U>) {
#if !FO_SAFE_ARITH_RELAXED
        static_assert(std::floating_point<T>, "Use iround for float to int conversion");
#endif
    }
    else if constexpr (std::is_unsigned_v<T> && std::is_unsigned_v<U> && sizeof(T) >= sizeof(U)) {
        // Always fit
    }
    else if constexpr (std::is_unsigned_v<T> && std::is_unsigned_v<U> && sizeof(T) < sizeof(U)) {
        if (value > static_cast<U>(std::numeric_limits<T>::max())) {
            value = static_cast<U>(std::numeric_limits<T>::max());
        }
    }
    else if constexpr (std::is_signed_v<T> && std::is_signed_v<U> && sizeof(T) >= sizeof(U)) {
        // Always fit
    }
    else if constexpr (std::is_signed_v<T> && std::is_signed_v<U> && sizeof(T) < sizeof(U)) {
        if (value > static_cast<U>(std::numeric_limits<T>::max())) {
            value = static_cast<U>(std::numeric_limits<T>::max());
        }
        if (value < static_cast<U>(std::numeric_limits<T>::min())) {
            value = static_cast<U>(std::numeric_limits<T>::min());
        }
    }
    else if constexpr (std::is_unsigned_v<T> && std::is_signed_v<U> && sizeof(T) >= sizeof(U)) {
        if (value < 0) {
            value = 0;
        }
    }
    else if constexpr (std::is_unsigned_v<T> && std::is_signed_v<U> && sizeof(T) < sizeof(U)) {
        if (value > static_cast<U>(std::numeric_limits<T>::max())) {
            value = static_cast<U>(std::numeric_limits<T>::max());
        }
        if (value < 0) {
            value = 0;
        }
    }
    else if constexpr (std::is_signed_v<T> && std::is_unsigned_v<U> && sizeof(T) == sizeof(U)) {
        if (value > static_cast<U>(std::numeric_limits<T>::max())) {
            value = static_cast<U>(std::numeric_limits<T>::max());
        }
    }
    else if constexpr (std::is_signed_v<T> && std::is_unsigned_v<U> && sizeof(T) > sizeof(U)) {
        // Always fit
    }
    else if constexpr (std::is_signed_v<T> && std::is_unsigned_v<U> && sizeof(T) < sizeof(U)) {
        if (value > static_cast<U>(std::numeric_limits<T>::max())) {
            value = static_cast<U>(std::numeric_limits<T>::max());
        }
    }
    else {
#if FO_SAFE_ARITH_RELAXED
        // Android builds prefer a permissive fallback instead of halting
        // the compilation on uncommon cross-signed conversions.
        value = static_cast<U>(static_cast<T>(value));
#else
        static_assert(detail::dependent_false_v<T, U>, "Unsupported numeric cast combination");
#endif
    }

    return static_cast<T>(value);
}

template<typename T, typename U>
    requires(std::floating_point<U> && std::integral<T>)
[[nodiscard]] constexpr auto iround(U value) -> T
{
    return numeric_cast<T>(std::llround(value));
}

// Safe arithmetics
template<typename T, typename U>
    requires(std::is_arithmetic_v<T> && std::same_as<T, U>)
[[nodiscard]] constexpr auto checked_add(const U& value1, const U& value2) -> U
{
    return numeric_cast<T>(value1 + value2);
}

template<typename T, typename U>
    requires(std::is_arithmetic_v<T> && std::same_as<T, U>)
[[nodiscard]] constexpr auto checked_sub(const U& value1, const U& value2) -> U
{
    return numeric_cast<T>(value1 - value2);
}

template<typename T, typename U>
    requires(std::is_arithmetic_v<T> && std::same_as<T, U>)
[[nodiscard]] constexpr auto checked_mul(const U& value1, const U& value2) -> U
{
    return numeric_cast<T>(value1 * value2);
}

template<typename T, typename U>
    requires(std::is_arithmetic_v<T> && std::same_as<T, U>)
[[nodiscard]] constexpr auto checked_div(const U& value1, const U& value2) -> U
{
    if constexpr (std::floating_point<T>) {
        if (std::fabs(value2) < std::numeric_limits<T>::epsilon()) {
            throw DivisionByZeroException("Float division by zero", typeid(T).name(), value1, value2);
        }
    }
    else {
        if (value2 == 0) {
            throw DivisionByZeroException("Integer division by zero", typeid(T).name(), value1, value2);
        }
    }

    return numeric_cast<T>(value1 / value2);
}

// Lerp
template<typename T, typename U = std::decay_t<T>>
    requires(std::floating_point<U>)
[[nodiscard]] constexpr auto lerp(T v1, T v2, float32 t) -> U
{
    return (t <= 0.0f) ? v1 : ((t >= 1.0f) ? v2 : v1 + (v2 - v1) * t);
}

template<typename T, typename U = std::decay_t<T>>
    requires(std::signed_integral<U>)
[[nodiscard]] constexpr auto lerp(T v1, T v2, float32 t) -> U
{
    return (t <= 0.0f) ? v1 : ((t >= 1.0f) ? v2 : v1 + iround<U>((v2 - v1) * t));
}

template<typename T, typename U = std::decay_t<T>>
    requires(std::unsigned_integral<U>)
[[nodiscard]] constexpr auto lerp(T v1, T v2, float32 t) -> U
{
    return (t <= 0.0f) ? v1 : ((t >= 1.0f) ? v2 : iround<U>(v1 * (1 - t) + v2 * t));
}

// Foreach helper
template<typename T>
class irange_iterator final
{
public:
    constexpr explicit irange_iterator(T v) noexcept :
        _value {v}
    {
    }
    constexpr auto operator==(const irange_iterator& other) const noexcept -> bool { return _value == other._value; }
    constexpr auto operator*() const noexcept -> const T& { return _value; }
    constexpr auto operator++() noexcept -> irange_iterator&
    {
        ++_value;
        return *this;
    }

private:
    T _value;
};

template<typename T>
class irange_loop final
{
public:
    constexpr explicit irange_loop(T to) noexcept :
        _fromValue {0},
        _toValue {to}
    {
    }
    constexpr explicit irange_loop(T from, T to) noexcept :
        _fromValue {from},
        _toValue {to}
    {
    }
    [[nodiscard]] constexpr auto begin() const noexcept -> irange_iterator<T> { return irange_iterator<T>(_fromValue); }
    [[nodiscard]] constexpr auto end() const noexcept -> irange_iterator<T> { return irange_iterator<T>(_toValue); }

private:
    T _fromValue;
    T _toValue;
};

template<typename T>
    requires(std::integral<T>)
constexpr auto iterate_range(T value) noexcept
{
    return irange_loop<T> {0, value};
}

template<typename T>
    requires has_member<T, &T::size, size_t>
constexpr auto iterate_range(const T& value) noexcept
{
    return irange_loop<decltype(value.size())> {0, value.size()};
}

FO_END_NAMESPACE();
