#ifndef SYNC_CPP_SYNC_GROUP_HPP_43OEW98IRFDU
#define SYNC_CPP_SYNC_GROUP_HPP_43OEW98IRFDU

#include <mutex>
#include <shared_mutex>

namespace spp
{
    template <typename... Ts>
    struct Group;

    template <typename... Ts>
    Group<Ts...> group(Ts&... syncs);

    template <typename... Ts>
    struct Group
    {
    private:
        template <typename T>
        using Value = std::conditional_t<std::is_const_v<T>, const typename T::Value, typename T::Value>;

    public:
        friend Group<Ts...> group<Ts...>(Ts&... syncs);

        // read-only access
        decltype(auto) read(std::invocable<const Value<Ts>&...> auto&& fn) const
        {
            auto handler = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return std::forward<decltype(fn)>(fn)(std::get<Is>(m_syncs).m_value...);
            };
            auto locks = lock_read();
            return handler(std::index_sequence_for<Ts...>{});
        }

        // write access
        decltype(auto) write(std::invocable<Value<Ts>&...> auto&& fn) const
            requires (not std::is_const_v<Ts> && ...)
        {
            auto handler = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return std::forward<decltype(fn)>(fn)(std::get<Is>(m_syncs).m_value...);
            };
            auto locks = lock_write();
            return handler(std::index_sequence_for<Ts...>{});
        }

        // dependent access mutability based on the T constness
        decltype(auto) lock(std::invocable<Value<Ts>&...> auto&& fn) const
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

            auto handler = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return std::forward<decltype(fn)>(fn)(std::get<Is>(m_syncs).m_value...);
            };

            auto locks = lock_all(std::index_sequence_for<Ts...>{});
            return handler(std::index_sequence_for<Ts...>{});
        }

    private:
        Group(std::tuple<Ts&...> syncs)
            : m_syncs{ std::move(syncs) }
        {
        }

        auto lock_read() const
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

        auto lock_write() const
        {
            auto handler = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                return std::tuple{ std::unique_lock{ std::get<Is>(m_syncs).mutex() }... };
            };
            return handler(std::index_sequence_for<Ts...>{});
        }

        std::tuple<Ts&...> m_syncs;
    };

    template <typename... Ts>
    Group<Ts...> group(Ts&... syncs)
    {
        return Group<Ts...>{ std::tie(syncs...) };
    }
}

#endif /* end of include guard: SYNC_CPP_SYNC_GROUP_HPP_43OEW98IRFDU */
