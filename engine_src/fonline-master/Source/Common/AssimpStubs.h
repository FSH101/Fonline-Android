// Fallback definitions for Assimp math types used by the client code when the
// real Assimp headers are not available (e.g. Android builds without the
// dependency).

#pragma once

#include <array>
#include <cmath>

template<typename T>
class aiVector3t
{
public:
    T x {};
    T y {};
    T z {};

    constexpr aiVector3t() = default;
    constexpr aiVector3t(T x_, T y_, T z_) noexcept :
        x {x_},
        y {y_},
        z {z_}
    {
    }

    constexpr auto operator+(const aiVector3t& rhs) const noexcept -> aiVector3t { return {x + rhs.x, y + rhs.y, z + rhs.z}; }
    constexpr auto operator-(const aiVector3t& rhs) const noexcept -> aiVector3t { return {x - rhs.x, y - rhs.y, z - rhs.z}; }
    constexpr auto operator*(T scalar) const noexcept -> aiVector3t { return {x * scalar, y * scalar, z * scalar}; }
    constexpr auto operator/(T scalar) const noexcept -> aiVector3t { return {x / scalar, y / scalar, z / scalar}; }
    constexpr auto operator+=(const aiVector3t& rhs) noexcept -> aiVector3t&
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }
    constexpr auto operator-=(const aiVector3t& rhs) noexcept -> aiVector3t&
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }
};

template<typename T>
class aiQuaterniont
{
public:
    T w {1};
    T x {};
    T y {};
    T z {};

    constexpr aiQuaterniont() = default;
    constexpr aiQuaterniont(T w_, T x_, T y_, T z_) noexcept :
        w {w_},
        x {x_},
        y {y_},
        z {z_}
    {
    }

    static auto Interpolate(aiQuaterniont& out, const aiQuaterniont& from, const aiQuaterniont& to, T factor) noexcept -> aiQuaterniont&
    {
        auto cos_half_theta = from.w * to.w + from.x * to.x + from.y * to.y + from.z * to.z;

        aiQuaterniont target = to;
        if (cos_half_theta < 0) {
            target.w = -target.w;
            target.x = -target.x;
            target.y = -target.y;
            target.z = -target.z;
            cos_half_theta = -cos_half_theta;
        }

        constexpr T epsilon = static_cast<T>(1e-6);
        if (cos_half_theta > (1 - epsilon)) {
            out.w = from.w + (target.w - from.w) * factor;
            out.x = from.x + (target.x - from.x) * factor;
            out.y = from.y + (target.y - from.y) * factor;
            out.z = from.z + (target.z - from.z) * factor;
        }
        else {
            const auto half_theta = std::acos(cos_half_theta);
            const auto sin_half_theta = std::sqrt(static_cast<T>(1) - cos_half_theta * cos_half_theta);
            const auto weight_from = std::sin((static_cast<T>(1) - factor) * half_theta) / sin_half_theta;
            const auto weight_to = std::sin(factor * half_theta) / sin_half_theta;

            out.w = from.w * weight_from + target.w * weight_to;
            out.x = from.x * weight_from + target.x * weight_to;
            out.y = from.y * weight_from + target.y * weight_to;
            out.z = from.z * weight_from + target.z * weight_to;
        }

        return out;
    }

    auto Normalize() noexcept -> aiQuaterniont&
    {
        const auto len_sq = w * w + x * x + y * y + z * z;
        if (len_sq > static_cast<T>(0)) {
            const auto inv_len = static_cast<T>(1) / std::sqrt(len_sq);
            w *= inv_len;
            x *= inv_len;
            y *= inv_len;
            z *= inv_len;
        }
        return *this;
    }

    auto GetMatrix() const noexcept -> std::array<std::array<T, 4>, 4>
    {
        const auto ww = w * w;
        const auto xx = x * x;
        const auto yy = y * y;
        const auto zz = z * z;
        const auto xy = x * y;
        const auto xz = x * z;
        const auto yz = y * z;
        const auto wx = w * x;
        const auto wy = w * y;
        const auto wz = w * z;

        return {{{ww + xx - yy - zz, static_cast<T>(2) * (xy - wz), static_cast<T>(2) * (xz + wy), static_cast<T>(0)},
                 {static_cast<T>(2) * (xy + wz), ww - xx + yy - zz, static_cast<T>(2) * (yz - wx), static_cast<T>(0)},
                 {static_cast<T>(2) * (xz - wy), static_cast<T>(2) * (yz + wx), ww - xx - yy + zz, static_cast<T>(0)},
                 {static_cast<T>(0), static_cast<T>(0), static_cast<T>(0), static_cast<T>(1)}}};
    }
};

template<typename T>
class aiMatrix4x4t
{
public:
    using Row = std::array<T, 4>;
    std::array<Row, 4> m {{Row {static_cast<T>(1), 0, 0, 0}, Row {0, static_cast<T>(1), 0, 0}, Row {0, 0, static_cast<T>(1), 0}, Row {0, 0, 0, static_cast<T>(1)}}};

    constexpr aiMatrix4x4t() = default;

    explicit constexpr aiMatrix4x4t(const std::array<Row, 4>& rows) noexcept :
        m {rows}
    {
    }

    constexpr aiMatrix4x4t(const aiVector3t<T>& scaling, const aiQuaterniont<T>& rotation, const aiVector3t<T>& position) noexcept
    {
        const auto rot = rotation.GetMatrix();
        for (size_t r = 0; r < 3; r++) {
            for (size_t c = 0; c < 3; c++) {
                m[r][c] = rot[r][c];
            }
        }
        m[0][3] = position.x;
        m[1][3] = position.y;
        m[2][3] = position.z;
        m[3][3] = static_cast<T>(1);
        m[0][0] *= scaling.x;
        m[1][1] *= scaling.y;
        m[2][2] *= scaling.z;
    }

    constexpr auto operator[](size_t row) noexcept -> T* { return m[row].data(); }
    constexpr auto operator[](size_t row) const noexcept -> const T* { return m[row].data(); }

    constexpr auto operator*(const aiMatrix4x4t& rhs) const noexcept -> aiMatrix4x4t
    {
        aiMatrix4x4t result;
        for (size_t r = 0; r < 4; r++) {
            for (size_t c = 0; c < 4; c++) {
                result.m[r][c] = m[r][0] * rhs.m[0][c] + m[r][1] * rhs.m[1][c] + m[r][2] * rhs.m[2][c] + m[r][3] * rhs.m[3][c];
            }
        }
        return result;
    }

    constexpr auto operator*=(const aiMatrix4x4t& rhs) noexcept -> aiMatrix4x4t&
    {
        *this = *this * rhs;
        return *this;
    }

    constexpr auto operator*(const aiVector3t<T>& v) const noexcept -> aiVector3t<T>
    {
        const auto x_ = m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3];
        const auto y_ = m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3];
        const auto z_ = m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3];
        return {x_, y_, z_};
    }

    [[nodiscard]] constexpr auto Transpose() const noexcept -> aiMatrix4x4t
    {
        aiMatrix4x4t result;
        for (size_t r = 0; r < 4; r++) {
            for (size_t c = 0; c < 4; c++) {
                result.m[r][c] = m[c][r];
            }
        }
        return result;
    }

    static auto Identity(aiMatrix4x4t& out) noexcept -> aiMatrix4x4t&
    {
        out = aiMatrix4x4t {};
        return out;
    }

    static auto RotationX(T angle, aiMatrix4x4t& out) noexcept -> aiMatrix4x4t&
    {
        const auto s = std::sin(angle);
        const auto c = std::cos(angle);
        out = aiMatrix4x4t {};
        out.m[1][1] = c;
        out.m[1][2] = -s;
        out.m[2][1] = s;
        out.m[2][2] = c;
        return out;
    }

    static auto RotationY(T angle, aiMatrix4x4t& out) noexcept -> aiMatrix4x4t&
    {
        const auto s = std::sin(angle);
        const auto c = std::cos(angle);
        out = aiMatrix4x4t {};
        out.m[0][0] = c;
        out.m[0][2] = s;
        out.m[2][0] = -s;
        out.m[2][2] = c;
        return out;
    }

    static auto RotationZ(T angle, aiMatrix4x4t& out) noexcept -> aiMatrix4x4t&
    {
        const auto s = std::sin(angle);
        const auto c = std::cos(angle);
        out = aiMatrix4x4t {};
        out.m[0][0] = c;
        out.m[0][1] = -s;
        out.m[1][0] = s;
        out.m[1][1] = c;
        return out;
    }

    static auto Translation(const aiVector3t<T>& v, aiMatrix4x4t& out) noexcept -> aiMatrix4x4t&
    {
        out = aiMatrix4x4t {};
        out.m[0][3] = v.x;
        out.m[1][3] = v.y;
        out.m[2][3] = v.z;
        return out;
    }

    static auto Scaling(const aiVector3t<T>& v, aiMatrix4x4t& out) noexcept -> aiMatrix4x4t&
    {
        out = aiMatrix4x4t {};
        out.m[0][0] = v.x;
        out.m[1][1] = v.y;
        out.m[2][2] = v.z;
        return out;
    }
};

template<typename T>
class aiColor4t
{
public:
    T r {};
    T g {};
    T b {};
    T a {static_cast<T>(1)};

    constexpr aiColor4t() = default;
    constexpr aiColor4t(T r_, T g_, T b_, T a_ = static_cast<T>(1)) noexcept :
        r {r_},
        g {g_},
        b {b_},
        a {a_}
    {
    }
};
