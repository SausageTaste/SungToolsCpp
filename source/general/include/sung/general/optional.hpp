#pragma once

#if defined(__cpp_lib_optional)
    #include <optional>
#else
    #include <tl/optional.hpp>
#endif


namespace sung {

#if defined(__cpp_lib_optional)
    template <class T>
    using Optional = std::optional<T>;

    using bad_optional_access = std::bad_optional_access;

    static constexpr auto nullopt = std::nullopt;

    #pragma message("[SUNG] std::optional will be used")

#else
    template <class T>
    using Optional = tl::optional<T>;

    using bad_optional_access = tl::bad_optional_access;

    static constexpr auto nullopt = tl::nullopt;

    #pragma message("[SUNG] tl::optional will be used")

#endif

}  // namespace sung
