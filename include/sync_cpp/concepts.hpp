#ifndef SYNC_CPP_CONCEPTS_HPP_RG36TC7P
#define SYNC_CPP_CONCEPTS_HPP_RG36TC7P

#include <concepts>
#include <type_traits>

namespace spp::concepts
{
    template <typename T, typename... Ts>
    concept DerivedFromAny = (std::derived_from<T, Ts> || ...);

    // clang-format off
    template <typename T>
    concept SmartPointer = requires(T sptr) {
        // Type alias
        typename T::element_type;

        // Basic operations
        { sptr.get() } -> std::same_as<typename T::element_type*>;
        { sptr.reset() } -> std::same_as<void>;
        { sptr.swap(sptr) } -> std::same_as<void>;

        // Dereference and arrow operator
        { *sptr } -> std::same_as<typename T::element_type&>;
        { sptr.operator->() } -> std::same_as<typename T::element_type*>;

        // Boolean conversion
        { static_cast<bool>(sptr) } -> std::convertible_to<bool>;

        // Comparison operators
        { sptr == nullptr } -> std::convertible_to<bool>;
        { sptr != nullptr } -> std::convertible_to<bool>;
    };
    // clang-format on

    template <typename Fn, typename From, typename To>
    concept Transformer = std::invocable<Fn, From> && std::same_as<To, std::invoke_result_t<Fn, From>>;

    template <typename Fn>
    concept StatelessLambda = std::is_empty_v<Fn>;
}

#endif /* end of include guard: SYNC_CPP_CONCEPTS_HPP_RG36TC7P */
