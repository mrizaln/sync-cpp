#ifndef SYNC_CONTAINER_HPP_6TIKVYGJ
#define SYNC_CONTAINER_HPP_6TIKVYGJ

#include "sync.hpp"

namespace spp
{
    template <
        typename Container,
        typename Element,
        typename Getter,
        typename Mutex     = std::mutex,
        bool InternalMutex = true>
        requires detail::concepts::Transformer<Getter, Container&, Element&>                //
              && detail::concepts::Transformer<Getter, const Container&, const Element&>    //
              && detail::concepts::StatelessLambda<Getter>                                  //
              && std::is_class_v<Element>
    class SyncContainer : public Sync<Container, Mutex, InternalMutex>
    {
    public:
        using Value_type = Container;
        using Mutex_type = Mutex;
        using SyncBase   = Sync<Container, Mutex, InternalMutex>;

        template <typename... Args>
            requires std::constructible_from<Container, Args...> && InternalMutex
        SyncContainer(Args&&... args)
            : SyncBase{ std::forward<Args>(args)... }
        {
        }

        SyncContainer(Container&& container)
            requires InternalMutex
            : SyncBase{ std::move(container) }
        {
        }

        template <typename... Args>
            requires std::constructible_from<Container, Args...> && (!InternalMutex)
        SyncContainer(Mutex& mutex, Args&&... args)
            : SyncBase{ mutex, std::forward<Args>(args)... }
        {
        }

        SyncContainer(Mutex& mutex, Container&& container)
            requires(!InternalMutex)
            : SyncBase{ mutex, std::move(container) }
        {
        }

        template <typename TT>
        [[nodiscard]] TT getValue(TT Element::*mem) const
        {
            return SyncBase::read([&](const Container& container) {
                decltype(auto) value{ getContained(container) };
                return value.*mem;
            });
        }

        template <typename Ret, typename... Args>
        [[nodiscard]] Ret readValue(Ret (Element::*fn)(Args...) const, std::type_identity_t<Args>... args) const
        {
            return SyncBase::read([&](const Container& container) {
                decltype(auto) value{ getContained(container) };
                return (value.*fn)(std::forward<Args>(args)...);
            });
        }

        template <typename Ret, typename... Args>
        [[nodiscard]] Ret writeValue(Ret (Element::*fn)(Args...), std::type_identity_t<Args>... args)
        {
            return SyncBase::write([&](Container& container) {
                decltype(auto) value{ getContained(container) };
                return (value.*fn)(std::forward<Args>(args)...);
            });
        }

        [[nodiscard]] decltype(auto) readValue(std::invocable<const Element&> auto&& fn) const
        {
            return SyncBase::read([&](const Container& container) {
                decltype(auto) value{ getContained(container) };
                return fn(value);
            });
        }

        [[nodiscard]] decltype(auto) writeValue(std::invocable<Element&> auto&& fn)
        {
            return SyncBase::write([&](Container& container) {
                decltype(auto) value{ getContained(container) };
                return fn(value);
            });
        }

    protected:
        Element&       getContained(Container& container) { return m_getter(container); }
        const Element& getContained(const Container& container) const { return m_getter(container); }

    private:
        [[no_unique_address]] Getter m_getter;
    };
}

#endif /* end of include guard: SYNC_CONTAINER_HPP_6TIKVYGJ */
