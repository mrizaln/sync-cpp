#ifndef SYNC_CONTAINER_HPP_6TIKVYGJ
#define SYNC_CONTAINER_HPP_6TIKVYGJ

#include "sync.hpp"

#include <concepts>
#include <mutex>
#include <type_traits>

namespace spp
{
    template <typename Container, typename Element, typename Getter, typename GetterConst, typename M = std::mutex>
        requires std::is_class_v<Element>
    class SyncContainer : public Sync<Container, M>
    {
    public:
        using Value_type = Container;
        using Mutex_type = M;
        using SyncBase   = Sync<Container, M>;

        template <typename... Args>
            requires std::constructible_from<Container, Args...>
        SyncContainer(Args&&... args)
            : SyncBase(std::forward<Args>(args)...)
        {
        }

        SyncContainer(Container&& container)
            : SyncBase(std::move(container))
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
        [[nodiscard]] Ret readValue(Ret (Element::*fn)(Args...) const, Args&&... args) const
        {
            return SyncBase::read([&](const Container& container) {
                decltype(auto) value{ getContained(container) };
                return (value.*fn)(std::forward<Args>(args)...);
            });
        }

        template <typename Ret, typename... Args>
        [[nodiscard]] Ret writeValue(Ret (Element::*fn)(Args...), Args&&... args)
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
        Element&       getContained(Container& container) { return Getter{}(container); }
        const Element& getContained(const Container& container) const { return GetterConst{}(container); }
    };
}

#endif /* end of include guard: SYNC_CONTAINER_HPP_6TIKVYGJ */
