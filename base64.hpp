#pragma once
#include <type_traits>
#include <string>
#include <array>

#if defined(__cplusplus) && __cplusplus >= 202002L || defined(_MSVC_LANG) && _MSVC_LANG >= 202002L
#include <ranges>
#endif

namespace base64 {

using byte = unsigned char;

namespace detail {
    
template<typename...>
using void_t = void;

template<bool p, typename T = void>
using enable_if_t = typename std::enable_if<p, T>::type;

template<bool cond, typename L, typename R>
using conditional_t = typename std::conditional<cond, L, R>::type;

template<typename T, typename = void>
struct is_input_iter : std::false_type {};

template<typename T>
struct is_input_iter<T, enable_if_t<std::is_pointer<T>::value>> : std::true_type {};

template<typename T>
struct is_input_iter<T, enable_if_t<std::is_base_of<std::input_iterator_tag, typename T::iterator_category>::value>> : std::true_type {};

template<typename T, typename = void>
struct is_output_iter : std::false_type {};

template<typename T>
struct is_output_iter<T, enable_if_t<std::is_pointer<T>::value>> : std::true_type {};

template<typename T>
struct is_output_iter<T, enable_if_t<std::is_base_of<std::output_iterator_tag, typename T::iterator_category>::value>> : std::true_type {};

template<typename T, typename = void>
struct is_byte_type : std::false_type {};
template<typename T>
struct is_byte_type<T, enable_if_t<
    std::is_same<char, T>::value ||
    std::is_same<signed char, T>::value ||
    std::is_same<unsigned char, T>::value ||
    std::is_same<byte, T>::value
#if defined(__cplusplus) && __cplusplus >= 201703L || defined(_MSVC_LANG) && _MSVC_LANG >= 201703L
    || std::is_same<std::byte, T>::value
#endif
    >> : std::true_type {};

template<typename T, typename = void>
struct is_byte_type_iter : std::false_type {};

template<typename T>
struct is_byte_type_iter<T, enable_if_t<std::is_pointer<T>::value && is_byte_type<typename std::remove_pointer<T>::type>::value>> : std::true_type {};

template<typename T>
struct is_byte_type_iter<T, enable_if_t<is_byte_type<typename T::value_type>::value>> : std::true_type {};

template<typename T, typename = void>
struct is_rdonly_byte_type_iter : std::false_type {};

template<typename T>
struct is_rdonly_byte_type_iter<T, enable_if_t<std::is_pointer<T>::value && is_byte_type<typename std::remove_cv<typename std::remove_pointer<T>::type>::type>::value>> : std::true_type {};

template<typename T>
struct is_rdonly_byte_type_iter<T, enable_if_t<is_byte_type<typename std::remove_cv<typename T::value_type>::type>::value>> : std::true_type {};


#if defined(__cplusplus) && __cplusplus >= 202002L || defined(_MSVC_LANG) && _MSVC_LANG >= 202002L

template<typename T>
struct is_range : std::bool_constant<std::ranges::range<T>> {};

#else

template<typename T, typename = void>
struct is_range : std::false_type {};
template<typename T>
struct is_range<T, void_t<decltype(std::begin(std::declval<T&>()), std::end(std::declval<T&>()), 0)>> : std::true_type {};

#endif

}

template<typename InputIt, typename TargetIt>
auto encode_to(InputIt first, InputIt second, TargetIt target) -> void {
    static_assert(detail::is_input_iter<InputIt>::value, "type `InputIt` must be a input-iterator type");
    static_assert(
        detail::is_input_iter<TargetIt>::value ||
        detail::is_output_iter<TargetIt>::value,
        "type `TargetIt` must be a input-iterator or output-iterator type"
    );
    static_assert(detail::is_rdonly_byte_type_iter<InputIt>::value, "the type of value in the range [first, second) must be `char`, `unsigned char`, `signed char`, `byte` or `std::byte`");
    static const std::array<char, 65> table_{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
    size_t flag_{};
    byte prev{};
    for (auto it = first; it != second; ++it) {
        auto value = static_cast<byte>(*it);
        switch (flag_) {
        case 0:
            *target = table_[value >> 2];
            prev = byte(value << 6) >> 2;
            break;
        case 1:
            *target = table_[prev | (value >> 4)];
            prev = byte(value << 4) >> 2;
            break;
        case 2:
            *target = table_[prev | (value >> 6)];
            ++target;
            *target = table_[byte(value << 2) >> 2];
            prev = static_cast<byte>(0);
            break;
        }
        ++target;
        ++flag_;
        flag_ %= 3;
    }
    if (flag_) {
        *target = table_[prev];
        ++target;
    }
    for (auto i = flag_; i > 0 && i < 3; ++i) {
        *target = '=';
        ++target;
    }
}

template<typename Range, typename TargetIt>
auto encode_to(const Range& rng_, TargetIt target) -> void {
    static_assert(detail::is_range<Range>::value, "type `Range` must be a range type: [begin, end)");
    encode_to(std::begin(rng_), std::end(rng_), target);
}

template<typename InputIt>
auto encode(InputIt first, InputIt second) ->std::string {
    std::string res{};
    encode_to(first, second, std::back_inserter(res));
    return res;
}

template<typename Range>
auto encode(const Range& rng_) ->std::string {
    static_assert(detail::is_range<Range>::value, "type `Range` must be a range type: [begin, end)");
    std::string res{};
    encode_to(std::begin(rng_), std::end(rng_), std::back_inserter(res));
    return res;
}


template<typename InputIt, typename TargetIt>
auto decode_to(InputIt first, InputIt second, TargetIt target) -> void {
    static_assert(detail::is_input_iter<InputIt>::value, "type `InputIt` must be a input-iterator type");
    static_assert(
        detail::is_input_iter<TargetIt>::value ||
        detail::is_output_iter<TargetIt>::value,
        "type `TargetIt` must be a input-iterator or output-iterator type"
    );
    static_assert(detail::is_byte_type_iter<InputIt>::value, "the type of value in the range [first, second) must be `char`, `unsigned char`, `signed char`, `byte` or `std::byte`");
    size_t flag_{};
    std::array<byte, 3> buff_{};
    for (auto it = first; it != second && *it != '='; ++it) {
        auto value = static_cast<byte>(*it);
        if (value <= '/') value = value == '+' ? 62 : 63;
        else if (value <= '9') value = value - '0' + 52;
        else if (value <= 'Z') value = value - 'A';
        else if (value <= 'z') value = value - 'a' + 26;
        switch (flag_)
        {
        case 0:
            buff_[0] = byte(value << 2);
            break;
        case 1:
            buff_[0] += value >> 4;
            buff_[1] = value << 4;
            break;
        case 2:
            buff_[1] += value >> 2;
            buff_[2] = value << 6;
            break;
        case 3:
            buff_[2] += value;
            for (auto c : buff_) {
                *target = c;
                ++target;
            }
            buff_.fill(0);
            break;
        }
        ++flag_;
        flag_ %= 4;
    }
    for (size_t i{}; flag_ && i < flag_ - 1; ++i) {
        *target = buff_[i];
        ++target;
    }
}

template<typename Range, typename TargetIt>
auto decode_to(const Range& rng_, TargetIt target) ->void {
    static_assert(detail::is_range<Range>::value, "type `Range` must be a range type: [begin, end)");
    decode_to(std::begin(rng_), std::end(rng_), target);
}


template<typename Byte = char>
class byte_stream_input_iterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = Byte;
    using difference_type = std::ptrdiff_t;
    using pointer = const Byte*;
    using reference = const Byte&;
public:
    byte_stream_input_iterator() noexcept : istm(nullptr) {};
    explicit byte_stream_input_iterator(std::ifstream& istm_) noexcept : istm(std::addressof(istm_)) {}
    byte_stream_input_iterator(const byte_stream_input_iterator&) = default;
    auto operator= (const byte_stream_input_iterator&) ->byte_stream_input_iterator& = default;
    ~byte_stream_input_iterator() = default;

    auto operator++() ->byte_stream_input_iterator& {
        return *this;
    }
    auto operator++(int) ->byte_stream_input_iterator& {
        return *this;
    }
    auto operator*() const ->Byte {
        return static_cast<Byte>(buf);
    }
    auto operator== (const byte_stream_input_iterator&) const ->bool {
        istm->read(&buf, 1);
        return istm->eof();
    }
    auto operator!= (const byte_stream_input_iterator& iit) const ->bool {
        return !(*this == iit);
    }
private:
    std::ifstream* istm;
    mutable char buf;
};


template<typename Byte = char>
class byte_stream_output_iterator {
public:
    using iterator_category = std::output_iterator_tag;
    using value_type = Byte;
    using difference_type = std::ptrdiff_t;
    using pointer = Byte*;
    using reference = Byte&;
public:
    byte_stream_output_iterator() noexcept : ostm(nullptr) {};
    explicit byte_stream_output_iterator(std::ofstream& ostm_) noexcept : ostm(std::addressof(ostm_)) {}
    byte_stream_output_iterator(const byte_stream_output_iterator&) = default;
    auto operator= (const byte_stream_output_iterator&) ->byte_stream_output_iterator& = default;
    auto operator=(Byte byte_) ->byte_stream_output_iterator& {
        auto buf = static_cast<byte>(byte_);
        ostm->write(reinterpret_cast<char*>(&buf), 1);
        return *this;
    }
    ~byte_stream_output_iterator() = default;
    auto operator++() ->byte_stream_output_iterator& {
        return *this;
    }
    auto operator++(int) ->byte_stream_output_iterator& {
        return *this;
    }
    auto operator*() ->byte_stream_output_iterator& {
        return *this;
    }
private:
    std::ofstream* ostm;
};


inline auto encode_from_file(std::ifstream& istm) ->std::string {
    return encode(byte_stream_input_iterator<>{istm}, {});
}

template<typename TargetIt>
auto encode_from_file_to(std::ifstream& istm, TargetIt target) ->void {
    static_assert(
        detail::is_input_iter<TargetIt>::value ||
        detail::is_output_iter<TargetIt>::value,
        "type `TargetIt` must be a input-iterator or output-iterator type"
    );
    encode_to(byte_stream_input_iterator<>{istm}, {}, target);
}

template<typename InputIt>
auto decode_to_file(InputIt first, InputIt second, std::ofstream& ostm) ->void {
    static_assert(detail::is_rdonly_byte_type_iter<InputIt>::value, "the type of value in the range [first, second) must be `char`, `unsigned char`, `signed char`, `byte` or `std::byte`");
    decode_to(first, second, byte_stream_output_iterator<>{ostm});
}

template<typename Range>
auto decode_to_file(const Range& rng_, std::ofstream& ostm) ->void {
    static_assert(detail::is_range<Range>::value, "type `Range` must be a range type: [begin, end)");
    decode_to_file(std::begin(rng_), std::end(rng_), ostm);
}

}
