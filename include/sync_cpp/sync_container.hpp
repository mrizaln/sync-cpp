#ifndef SYNC_CPP_SYNC_CONTAINER_HPP_6TIKVYGJ
#define SYNC_CPP_SYNC_CONTAINER_HPP_6TIKVYGJ

#include "sync_cpp/sync.hpp"

namespace spp
{
    /**
     * @class SyncContainer
     *
     * @brief A wrapper around a container object with a mutex.
     *
     * @tparam Container The type of the container object to wrap.
     * @tparam Element The element of the container object.
     * @tparam Getter The getter to access the element of the container object.
     */
    template <
        typename Container,
        typename Element,
        concepts::Syncable  Getter,
        concepts::SyncMutex Mtx           = std::mutex,
        bool                InternalMutex = true>
        requires concepts::Transformer<Getter, Container&, Element&>                //
             and concepts::Transformer<Getter, const Container&, const Element&>    //
             and concepts::StatelessLambda<Getter>                                  //
             and concepts::Syncable<Element>                                        //
             and concepts::SyncMutex<Mtx>
    class SyncContainer : public Sync<Container, Mtx, InternalMutex>
    {
    public:
        using Value    = Container;
        using Mutex    = Mtx;
        using SyncBase = Sync<Container, Mtx, InternalMutex>;

        template <typename... Args>
            requires std::constructible_from<Container, Args...> and InternalMutex
        SyncContainer(Args&&... args)
            : SyncBase{ std::forward<Args>(args)... }
        {
        }

        template <typename... Args>
            requires std::constructible_from<Container, Args...> and (not InternalMutex)
        SyncContainer(Mtx& mutex, Args&&... args)
            : SyncBase{ mutex, std::forward<Args>(args)... }
        {
        }

        /**
         * @brief Get the contained wrapped value's member by copy.
         *
         * @tparam TT [TODO:tparam]
         * @return [TODO:return]
         */
        template <typename TT>
        [[nodiscard]] TT get_value(TT Element::* mem) const
        {
            return SyncBase::read([&](const Container& container) {
                decltype(auto) value = get_contained(container);
                return value.*mem;
            });
        }

        /**
         * @brief Call the contained wrapped value's const member function.
         *
         * @tparam Ret The return type of the member function.
         * @tparam Args The parameter types of the member function.
         *
         * @param args The argument to the member function.
         *
         * @return The value of the call to the member function.
         */
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

        /**
         * @brief Call the contained wrapped value's const member function.
         *
         * @tparam Ret The return type of the member function.
         * @tparam Args The parameter types of the member function.
         *
         * @param args The argument to the member function.
         *
         * @return The value of the call to the member function.
         */
        template <typename Ret, typename... Args>
        [[nodiscard]] Ret read_value(
            Ret (Element::*fn)(Args...) const noexcept,
            std::type_identity_t<Args>... args
        ) const
        {
            return SyncBase::read([&](const Container& container) {
                decltype(auto) value = get_contained(container);
                return (value.*fn)(std::forward<Args>(args)...);
            });
        }

        /**
         * @brief Call the contained wrapped value's non-const member function.
         *
         * @tparam Ret The return type of the member function.
         * @tparam Args The parameter types of the member function.
         *
         * @param args The argument to the member function.
         *
         * @return The value of the call to the member function.
         */
        template <typename Ret, typename... Args>
        [[nodiscard]] Ret write_value(
            Ret (Element::*fn)(Args...),    //
            std::type_identity_t<Args>... args
        )
        {
            return SyncBase::write([&](Container& container) {
                decltype(auto) value = get_contained(container);
                return (value.*fn)(std::forward<Args>(args)...);
            });
        }

        /**
         * @brief Call the contained wrapped value's non-const member function.
         *
         * @tparam Ret The return type of the member function.
         * @tparam Args The parameter types of the member function.
         *
         * @param args The argument to the member function.
         *
         * @return The value of the call to the member function.
         */
        template <typename Ret, typename... Args>
        [[nodiscard]] Ret write_value(
            Ret (Element::*fn)(Args...) noexcept,
            std::type_identity_t<Args>... args
        )
        {
            return SyncBase::write([&](Container& container) {
                decltype(auto) value = get_contained(container);
                return (value.*fn)(std::forward<Args>(args)...);
            });
        }

        /**
         * @brief Access the contained wrapped value in a read-only context.
         *
         * @param fn The function to call with the value.
         *
         * @return The return value of the function.
         */
        [[nodiscard]] decltype(auto) read_value(std::invocable<const Element&> auto&& fn) const
        {
            return SyncBase::read([&](const Container& container) {
                decltype(auto) value = get_contained(container);
                return fn(value);
            });
        }

        /**
         * @brief Access the contained wrapped value in a read-write context.
         *
         * @param fn The function to call with the value.
         *
         * @return The return value of the function.
         */
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
