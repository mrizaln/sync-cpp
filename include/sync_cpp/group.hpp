#ifndef SYNC_CPP_SYNC_GROUP_HPP_43OEW98IRFDU
#define SYNC_CPP_SYNC_GROUP_HPP_43OEW98IRFDU

#include "sync_cpp/concepts.hpp"

#include <mutex>
#include <shared_mutex>

namespace spp
{
    template <typename... Ts>
    class [[nodiscard]] Group;

    template <concepts::SyncDerivative... Ts>
    Group<Ts...> group(Ts&... syncs);

    /**
     * @class Group
     *
     * @brief A group of Sync objects wrapper.
     *
     * @tparam Ts The Sync objects actual types (derived from spp::Sync).
     */
    template <typename... Ts>
    class [[nodiscard]] Group
    {
    private:
        template <typename T>
        using Value = std::conditional_t<std::is_const_v<T>, const typename T::Value, typename T::Value>;

    public:
        friend Group<Ts...> group<Ts...>(Ts&... syncs);

        /**
         * @brief Access all the Sync object's values in a read-only context.
         *
         * @param fn The function to call with the value.
         *
         * @return The return value of the function.
         */
        [[nodiscard]] decltype(auto) read(std::invocable<const Value<Ts>&...> auto&& fn) const
        {
            auto handler = [&]<std::size_t... Is>(std::index_sequence<Is...>) -> decltype(auto) {
                return std::forward<decltype(fn)>(fn)(std::get<Is>(m_syncs).m_value...);
            };

            static_assert(
                not std::is_lvalue_reference_v<decltype(handler(std::index_sequence_for<Ts...>{}))>,
                "Function returning a reference in multithreaded context is dangerous! Consider copying "
                "instead."
            );

            auto locks = lock_read();
            return handler(std::index_sequence_for<Ts...>{});
        }

        /**
         * @brief Access all the Sync object's values in a read-write context.
         *
         * @param fn The function to call with the value.
         *
         * @return The return value of the function.
         */
        [[nodiscard]] decltype(auto) write(std::invocable<Value<Ts>&...> auto&& fn) const
            requires (not std::is_const_v<Ts> and ...)
        {
            auto handler = [&]<std::size_t... Is>(std::index_sequence<Is...>) -> decltype(auto) {
                return std::forward<decltype(fn)>(fn)(std::get<Is>(m_syncs).m_value...);
            };

            static_assert(
                not std::is_lvalue_reference_v<decltype(handler(std::index_sequence_for<Ts...>{}))>,
                "Function returning a reference in multithreaded context is dangerous! Consider copying "
                "instead."
            );

            auto locks = lock_write();
            return handler(std::index_sequence_for<Ts...>{});
        }

        /**
         * @brief Access all the Sync object's values in a const or non-const context depending on the
         * constness of its Sync.
         *
         * @param fn The function to call with the value.
         *
         * @return The return value of the function.
         */
        [[nodiscard]] decltype(auto) lock(std::invocable<Value<Ts>&...> auto&& fn) const
        {
            auto lock_all = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                auto lock_read = []<typename M>(M& mutex) {
                    if constexpr (std::derived_from<M, std::shared_mutex>) {
                        return std::shared_lock{ mutex };
                    } else {
                        return std::unique_lock{ mutex };
                    }
                };
                auto lock = [&]<typename T>(T& sync) {
                    if constexpr (std::is_const_v<T>) {
                        return lock_read(sync.mutex());
                    } else {
                        return std::unique_lock{ sync.mutex() };
                    }
                };
                return std::tuple{ lock(std::get<Is>(m_syncs))... };
            };

            auto handler = [&]<std::size_t... Is>(std::index_sequence<Is...>) -> decltype(auto) {
                return std::forward<decltype(fn)>(fn)(std::get<Is>(m_syncs).m_value...);
            };

            static_assert(
                not std::is_lvalue_reference_v<decltype(handler(std::index_sequence_for<Ts...>{}))>,
                "Function returning a reference in multithreaded context is dangerous! Consider copying "
                "instead."
            );

            auto locks = lock_all(std::index_sequence_for<Ts...>{});
            return handler(std::index_sequence_for<Ts...>{});
        }

    private:
        Group(std::tuple<Ts&...> syncs)
            : m_syncs{ std::move(syncs) }
        {
        }

        [[nodiscard]] auto lock_read() const
        {
            auto lock = []<typename M>(M& mutex) {
                if constexpr (std::derived_from<M, std::shared_mutex>) {
                    return std::shared_lock{ mutex };
                } else {
                    return std::unique_lock{ mutex };
                }
            };
            auto handler = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return std::tuple{ lock(std::get<Is>(m_syncs).mutex())... };
            };
            return handler(std::index_sequence_for<Ts...>{});
        }

        [[nodiscard]] auto lock_write() const
        {
            auto handler = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return std::tuple{ std::unique_lock{ std::get<Is>(m_syncs).mutex() }... };
            };
            return handler(std::index_sequence_for<Ts...>{});
        }

        std::tuple<Ts&...> m_syncs;
    };

    /**
     * @brief Make a group of Sync objects.
     *
     * @tparam Ts The Sync objects actual types (derived from spp::Sync).
     *
     * @param syncs The Sync objects to group
     *
     * @return A Group object containing the Sync objects.
     *
     * TODO: add constraints to Ts
     *
     */
    template <concepts::SyncDerivative... Ts>
    Group<Ts...> group(Ts&... syncs)
    {
        return Group<Ts...>{ std::tie(syncs...) };
    }
}

#endif /* end of include guard: SYNC_CPP_SYNC_GROUP_HPP_43OEW98IRFDU */
