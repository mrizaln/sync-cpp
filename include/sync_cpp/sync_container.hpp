#ifndef SYNC_CPP_SYNC_CONTAINER_HPP_6TIKVYGJ
#define SYNC_CPP_SYNC_CONTAINER_HPP_6TIKVYGJ

#include "sync_cpp/sync.hpp"

namespace spp
{
    template <
        typename Container,
        typename Element,
        typename Getter,
        typename Mtx       = std::mutex,
        bool InternalMutex = true>
        requires concepts::Transformer<Getter, Container&, Element&>                //
              && concepts::Transformer<Getter, const Container&, const Element&>    //
              && concepts::StatelessLambda<Getter>                                  //
              && std::is_class_v<Element>
    class SyncContainer : public Sync<Container, Mtx, InternalMutex>
    {
    public:
        using Value    = Container;
        using Mutex    = Mtx;
        using SyncBase = Sync<Container, Mtx, InternalMutex>;

        template <typename... Args>
            requires std::constructible_from<Container, Args...> && InternalMutex
        SyncContainer(Args&&... args)
            : SyncBase{ std::forward<Args>(args)... }
        {
        }

        template <typename... Args>
            requires std::constructible_from<Container, Args...> && (not InternalMutex)
        SyncContainer(Mtx& mutex, Args&&... args)
            : SyncBase{ mutex, std::forward<Args>(args)... }
        {
        }

        template <typename TT>
        [[nodiscard]] TT get_value(TT Element::* mem) const
        {
            return SyncBase::read([&](const Container& container) {
                decltype(auto) value = get_contained(container);
                return value.*mem;
            });
        }

        template <typename Ret, typename... Args>
        [[nodiscard]] Ret read_value(
            Ret (Element::*fn)(Args...) const,
            std::type_identity_t<Args>... args
        ) const
        {
            return SyncBase::read([&](const Container& container) {
                decltype(auto) value = get_contained(container);
                return (value.*fn)(std::forward<Args>(args)...);
            });
        }

        template <typename Ret, typename... Args>
        [[nodiscard]] Ret write_value(Ret (Element::*fn)(Args...), std::type_identity_t<Args>... args)
        {
            return SyncBase::write([&](Container& container) {
                decltype(auto) value = get_contained(container);
                return (value.*fn)(std::forward<Args>(args)...);
            });
        }

        [[nodiscard]] decltype(auto) read_value(std::invocable<const Element&> auto&& fn) const
        {
            return SyncBase::read([&](const Container& container) {
                decltype(auto) value = get_contained(container);
                return fn(value);
            });
        }

        [[nodiscard]] decltype(auto) write_value(std::invocable<Element&> auto&& fn)
        {
            return SyncBase::write([&](Container& container) {
                decltype(auto) value = get_contained(container);
                return fn(value);
            });
        }

    protected:
        Element&       get_contained(Container& container) { return m_getter(container); }
        const Element& get_contained(const Container& container) const { return m_getter(container); }

    private:
        [[no_unique_address]] Getter m_getter;
    };
}

#endif /* end of include guard: SYNC_CPP_SYNC_CONTAINER_HPP_6TIKVYGJ */
