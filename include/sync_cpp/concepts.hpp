#ifndef SYNC_CPP_CONCEPTS_HPP_RG36TC7P
#define SYNC_CPP_CONCEPTS_HPP_RG36TC7P

#include <concepts>
#include <mutex>
#include <shared_mutex>
#include <type_traits>

namespace spp::tag
{
    /**
     * @class SyncTag
     * @brief A tag to identify a class as a Sync derivative.
     */
    struct SyncTag
    {
    };
}

namespace spp::concepts
{
    /**
     * @brief The requirements for type to be considered syncable (used in Sync derivatives).
     */
    template <typename T>
    concept Syncable = std::is_class_v<T> and not std::is_reference_v<T> and not std::is_const_v<T>;

    template <typename T>
    concept SyncMutex = requires {
        requires not std::is_reference_v<T>;
        requires not std::is_const_v<T>;

        requires std::derived_from<T, std::mutex>                  //
                     or std::derived_from<T, std::shared_mutex>    //
                     or std::derived_from<T, std::recursive_mutex>;
    };

    /**
     * @brief The requirements for a type to be considered a Sync derivative.
     */
    template <typename T>
    concept SyncDerivative = requires {
        typename T::Value;
        typename T::Mutex;

        requires Syncable<typename T::Value>;
        requires SyncMutex<typename T::Mutex>;

        requires std::derived_from<T, tag::SyncTag>;
    };

    /**
     * @brief An approximate concept for standard smart pointers.
     */
    template <typename T>
    concept SmartPointer = requires (T sptr) {
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

    /**
     * @brief Check if a Fn can transform From to To.
     */
    template <typename Fn, typename From, typename To>
    concept Transformer = std::invocable<Fn, From> && std::same_as<To, std::invoke_result_t<Fn, From>>;

    /**
     * @brief Check if a lambda or function object is stateless.
     */
    template <typename Fn>
    concept StatelessLambda = std::is_empty_v<Fn>;
}

#endif /* end of include guard: SYNC_CPP_CONCEPTS_HPP_RG36TC7P */
