#ifndef SYNC_CONTAINER_HPP_6TIKVYGJ
#define SYNC_CONTAINER_HPP_6TIKVYGJ

#include "sync.hpp"

#include <mutex>
#include <type_traits>

namespace spp
{
    template <typename Container, typename Element, typename Getter, typename GetterConst, typename M = std::mutex>
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

        template <typename Mem>
            requires std::is_member_object_pointer_v<Mem>
        [[nodiscard]] auto getValue(Mem mem) const    // the 'auto' removes reference and cv-qualifier
        {
            return SyncBase::read([&](const Container& container) {    // no decltype(auto) here
                const auto& value{ getContained(container) };
                return value.*mem;
            });
        }

        template <typename MemFunc, typename... Args>
        [[nodiscard]] decltype(auto) readValue(MemFunc&& f, Args&&... args) const
            requires std::is_member_function_pointer_v<MemFunc> && std::invocable<MemFunc, const Element&, Args...>
        {
            return SyncBase::read([&](const Container& container) -> decltype(auto) {
                const auto& value{ getContained(container) };
                return (value.*f)(std::forward<Args>(args)...);
            });
        }

        template <typename MemFunc, typename... Args>
        [[nodiscard]] decltype(auto) writeValue(MemFunc&& f, Args&&... args)
            requires std::is_member_function_pointer_v<MemFunc> && std::invocable<MemFunc, Element&, Args...>
        {
            return SyncBase::write([&](Container& container) -> decltype(auto) {
                auto& value{ getContained(container) };
                return (value.*f)(std::forward<Args>(args)...);
            });
        }

        template <typename Func>
            requires(!std::is_member_function_pointer_v<Func>) && std::invocable<Func, const Element&>
        [[nodiscard]] decltype(auto) readValue(Func&& f) const
        {
            return SyncBase::read([&](const Container& container) -> decltype(auto) {
                const auto& value{ getContained(container) };
                return f(value);
            });
        }

        template <typename Func>
            requires(!std::is_member_function_pointer_v<Func>) && std::invocable<Func, Element&>
        [[nodiscard]] decltype(auto) writeValue(Func&& f)
        {
            return SyncBase::write([&](Container& container) -> decltype(auto) {
                auto& value{ getContained(container) };
                return f(value);
            });
        }

    protected:
        Element& getContained(Container& container)
        {
            if constexpr (std::is_member_function_pointer_v<Getter>) {
                return (container.*(GetterConst{}))();
            } else {
                return GetterConst{}(container);
            }
        }

        const Element& getContained(const Container& container) const
        {
            if constexpr (std::is_member_function_pointer_v<GetterConst>) {
                return (container.*(GetterConst{}))();
            } else {
                return GetterConst{}(container);
            }
        }
    };
}

#endif /* end of include guard: SYNC_CONTAINER_HPP_6TIKVYGJ */
