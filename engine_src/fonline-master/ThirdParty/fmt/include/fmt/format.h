#pragma once

#include <algorithm>
#include <iterator>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace fmt {

using format_error = std::runtime_error;

class format_parse_context
{
public:
    using iterator = std::string_view::const_iterator;

    explicit format_parse_context(std::string_view view) noexcept :
        _view {view}
    {
    }

    [[nodiscard]] auto begin() const noexcept -> iterator { return _view.begin(); }

    [[nodiscard]] auto end() const noexcept -> iterator { return _view.end(); }

    void advance_to(iterator it) noexcept { _pos = it; }

private:
    std::string_view _view {};
    iterator _pos { _view.begin() };
};

class format_context
{
public:
    using iterator = std::back_insert_iterator<std::string>;

    explicit format_context(std::string& buffer) noexcept :
        _out {std::back_inserter(buffer)}
    {
    }

    [[nodiscard]] auto out() noexcept -> iterator { return _out; }

private:
    iterator _out;
};

namespace detail {

template<typename T, typename = void>
struct is_streamable : std::false_type
{
};

template<typename T>
struct is_streamable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>> : std::true_type
{
};

template<typename T>
inline constexpr auto is_streamable_v = is_streamable<T>::value;

inline void append_chunk(std::string& out, std::string_view chunk)
{
    out.append(chunk.data(), chunk.size());
}

inline auto parse_placeholder(std::string_view fmt, std::string_view::size_type start) -> std::string_view::size_type
{
    auto pos = fmt.find('}', start);
    if (pos == std::string_view::npos) {
        throw format_error("missing closing brace in format string");
    }
    return pos;
}

template<typename T>
inline auto to_string_any(T&& value) -> std::string
{
    using Decayed = std::decay_t<T>;

    if constexpr (is_streamable_v<Decayed>) {
        std::ostringstream ss;
        ss << std::forward<T>(value);
        return ss.str();
    }
    else {
        return std::string {"<unformattable>"};
    }
}

template<typename T>
inline auto to_string_any(const std::basic_string<T>& value) -> std::string
{
    return std::string {value.begin(), value.end()};
}

template<typename Char>
inline auto to_string_any(std::basic_string_view<Char> value) -> std::string
{
    return std::string {value.begin(), value.end()};
}

inline auto to_string_any(const char* value) -> std::string
{
    return value ? std::string {value} : std::string {};
}

inline auto to_string_any(char* value) -> std::string
{
    return value ? std::string {value} : std::string {};
}

template<typename T>
inline auto to_string_any(const T* ptr) -> std::string
{
    std::ostringstream ss;
    ss << ptr;
    return ss.str();
}

template<typename T>
inline auto to_string_any(T* ptr) -> std::string
{
    std::ostringstream ss;
    ss << ptr;
    return ss.str();
}

template<typename... Args>
inline auto collect_args(Args&&... args)
{
    return std::vector<std::string> {to_string_any(std::forward<Args>(args))...};
}

inline auto vformat_impl(std::string_view fmt, const std::vector<std::string>& args) -> std::string
{
    std::string out;
    out.reserve(fmt.size() + args.size() * 4);

    std::string_view::size_type pos = 0;
    size_t arg_index = 0;
    while (pos < fmt.size()) {
        auto brace = fmt.find('{', pos);
        if (brace == std::string_view::npos) {
            append_chunk(out, fmt.substr(pos));
            break;
        }

        // Handle escaped '{{'
        if (brace + 1 < fmt.size() && fmt[brace + 1] == '{') {
            append_chunk(out, fmt.substr(pos, brace - pos + 1));
            pos = brace + 2;
            continue;
        }

        append_chunk(out, fmt.substr(pos, brace - pos));
        auto end = parse_placeholder(fmt, brace + 1);
        if (arg_index >= args.size()) {
            throw format_error("not enough arguments for format string");
        }

        append_chunk(out, args[arg_index]);
        ++arg_index;
        pos = end + 1;
    }

    return out;
}

} // namespace detail

template<typename... Args>
using format_string = std::string_view;

template<typename... Args>
inline auto format(format_string<Args...> fmt_str, Args&&... args) -> std::string
{
    const auto collected = detail::collect_args(std::forward<Args>(args)...);
    return detail::vformat_impl(fmt_str, collected);
}

inline auto vformat(std::string_view fmt_str, const std::vector<std::string>& args) -> std::string
{
    return detail::vformat_impl(fmt_str, args);
}

template<typename... Args>
inline auto make_format_args(Args&&... args)
{
    return detail::collect_args(std::forward<Args>(args)...);
}

template<typename OutputIt, typename... Args>
inline auto format_to(OutputIt out, format_string<Args...> fmt_str, Args&&... args) -> OutputIt
{
    auto formatted = format(fmt_str, std::forward<Args>(args)...);
    return std::copy(formatted.begin(), formatted.end(), out);
}

template<typename OutputIt>
struct format_to_n_result
{
    OutputIt out;
    size_t size;
};

template<typename OutputIt, typename... Args>
inline auto format_to_n(OutputIt out, size_t n, format_string<Args...> fmt_str, Args&&... args) -> format_to_n_result<OutputIt>
{
    auto formatted = format(fmt_str, std::forward<Args>(args)...);
    const auto count = std::min(n, formatted.size());
    auto it = std::copy_n(formatted.begin(), count, out);
    return {it, count};
}

template<typename T>
struct formatter
{
    constexpr auto parse(format_parse_context& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const T& value, FormatContext& ctx) const
    {
        std::ostringstream ss;
        ss << value;
        auto str = ss.str();
        return std::copy(str.begin(), str.end(), ctx.out());
    }
};

template<>
struct formatter<std::string_view>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(std::string_view value, FormatContext& ctx) const
    {
        return std::copy(value.begin(), value.end(), ctx.out());
    }
};

template<>
struct formatter<std::string>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const std::string& value, FormatContext& ctx) const
    {
        return std::copy(value.begin(), value.end(), ctx.out());
    }
};

} // namespace fmt

