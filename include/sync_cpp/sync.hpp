#ifndef SYNC_CPP_SYNC_CPP_49PEW7R6FEO7DS
#define SYNC_CPP_SYNC_CPP_49PEW7R6FEO7DS

#include "sync_cpp/concepts.hpp"

#include <mutex>
#include <shared_mutex>
#include <utility>

namespace spp
{
    /**
     * @class Sync
     *
     * @brief A wrapper around a class object with a mutex.
     *
     * @tparam T The type of the object to wrap.
     */
    template <concepts::Syncable T, concepts::SyncMutex M = std::mutex, bool InternalMutex = true>
    class Sync : public tag::SyncTag
    {
    public:
        template <typename... Ts>
        friend class Group;

        using Value = T;
        using Mutex = M;

        Sync(const Sync&)            = delete;
        Sync& operator=(const Sync&) = delete;
        Sync(Sync&&)                 = delete;
        Sync& operator=(Sync&&)      = delete;

        // prevent class from being created using new and destructed using delete
        static void* operator new(size_t)     = delete;
        static void* operator new[](size_t)   = delete;
        static void  operator delete(void*)   = delete;
        static void  operator delete[](void*) = delete;

        template <typename... Args>
            requires std::constructible_from<T, Args...> and InternalMutex
        Sync(Args&&... args)
            : m_value{ std::forward<Args>(args)... }
            , m_mutex{}
        {
        }

        template <typename... Args>
            requires std::constructible_from<T, Args...> and (not InternalMutex)
        Sync(M& mutex, Args&&... args)
            : m_value{ std::forward<Args>(args)... }
            , m_mutex{ &mutex }
        {
        }

        /**
         * @brief Get member object by copy.
         *
         * @tparam The type of the member object.
         * @return Copy of the member object.
         */
        template <typename TT>
        [[nodiscard]] TT get(TT T::* mem) const
        {
            auto lock = lock_read();
            return m_value.*mem;
        }

        /**
         * @brief Call a const member function of the wrapped value.
         *
         * @tparam Ret The return type of the member function.
         * @tparam Args The parameter types of the member function.
         *
         * @param args The argument to the member function.
         *
         * @return The value of the call to the member function.
         */
        template <typename Ret, typename... Args>
        [[nodiscard]] Ret read(Ret (T::*fn)(Args...) const, std::type_identity_t<Args>... args) const
        {
            static_assert(
                not std::is_lvalue_reference_v<Ret>,
                "Member function returning a reference in multithreaded context is dangerous! Consider "
                "copying instead."
            );

            auto lock = lock_read();
            return (m_value.*fn)(std::forward<Args>(args)...);
        }

        /**
         * @brief Call a non-const member function of the wrapped value.
         *
         * @tparam Ret The return type of the member function.
         * @tparam Args The parameter types of the member function.
         *
         * @param args The argument to the member function.
         *
         * @return The value of the call to the member function.
         */
        template <typename Ret, typename... Args>
        [[nodiscard]] Ret write(Ret (T::*fn)(Args...), std::type_identity_t<Args>... args)
        {
            static_assert(
                not std::is_lvalue_reference_v<Ret>,
                "Member function returning a reference in multithreaded context is dangerous! Consider "
                "copying instead."
            );

            auto lock = lock_write();
            return (m_value.*fn)(std::forward<Args>(args)...);
        }

        /**
         * @brief Call a const member function of the wrapped value.
         *
         * @tparam Ret The return type of the member function.
         * @tparam Args The parameter types of the member function.
         *
         * @param args The argument to the member function.
         *
         * @return The value of the call to the member function.
         */
        template <typename Ret, typename... Args>
        [[nodiscard]] Ret read(Ret (T::*fn)(Args...) const noexcept, std::type_identity_t<Args>... args) const
        {
            static_assert(
                not std::is_lvalue_reference_v<Ret>,
                "Member function returning a reference in multithreaded context is dangerous! Consider "
                "copying instead."
            );

            auto lock = lock_read();
            return (m_value.*fn)(std::forward<Args>(args)...);
        }

        /**
         * @brief Call a non-const member function of the wrapped value.
         *
         * @tparam Ret The return type of the member function.
         * @tparam Args The parameter types of the member function.
         *
         * @param args The argument to the member function.
         *
         * @return The value of the call to the member function.
         */
        template <typename Ret, typename... Args>
        [[nodiscard]] Ret write(Ret (T::*fn)(Args...) noexcept, std::type_identity_t<Args>... args)
        {
            static_assert(
                not std::is_lvalue_reference_v<Ret>,
                "Member function returning a reference in multithreaded context is dangerous! Consider "
                "copying instead."
            );

            auto lock = lock_write();
            return (m_value.*fn)(std::forward<Args>(args)...);
        }

        /**
         * @brief Access the wrapped value in a read-only context.
         *
         * @param fn The function to call with the value.
         *
         * @return The return value of the function.
         */
        [[nodiscard]] decltype(auto) read(std::invocable<const T&> auto&& fn) const
        {
            static_assert(
                not std::is_lvalue_reference_v<decltype(fn(m_value))>,
                "Function returning a reference in multithreaded context is dangerous! Consider copying "
                "instead"
            );

            auto lock = lock_read();
            return std::forward<decltype(fn)>(fn)(m_value);
        }

        /**
         * @brief Access the wrapped value in a read-write context.
         *
         * @param fn The function to call with the value.
         *
         * @return The return value of the function.
         */
        [[nodiscard]] decltype(auto) write(std::invocable<T&> auto&& fn)
        {
            static_assert(
                not std::is_lvalue_reference_v<decltype(fn(m_value))>,
                "Function returning a reference in multithreaded context is dangerous! Consider copying "
                "instead."
            );

            auto lock = lock_write();
            return std::forward<decltype(fn)>(fn)(m_value);
        }

        /**
         * @brief Assign a new value to the wrapped object.
         *
         * @tparam TT The type of the new value.
         *
         * @param value The new value to assign.
         */
        template <typename TT>
            requires std::assignable_from<std::decay_t<T>, TT>
        Sync& operator=(TT&& value)
        {
            write([&](auto& v) { v = std::forward<TT>(value); });
            return *this;
        }

        /**
         * @brief Swap the value of two Sync objects.
         *
         * @param other The other Sync object to swap with.
         *
         * NOTE: at swap, make sure that both 'this' and 'other' does not have any thread waiting for resource
         * from it.
         */
        void swap(Sync& other)
        {
            auto lock1 = std::unique_lock{ mutex(), std::defer_lock };
            auto lock2 = std::unique_lock{ other.mutex(), std::defer_lock };

            std::lock(lock1, lock2);

            std::swap(m_value, other.m_value);
        }

        /**
         * @brief Get the mutex object.
         */
        Mutex& mutex() const
        {
            if constexpr (std::is_pointer_v<UnderlyingMutex>) {
                return *m_mutex;
            } else {
                return m_mutex;
            }
        }

    private:
        using UnderlyingMutex = std::conditional_t<InternalMutex, Mutex, Mutex*>;

        [[nodiscard]] auto lock_read() const
        {
            if constexpr (std::derived_from<M, std::shared_mutex>) {
                return std::shared_lock{ mutex() };
            } else {
                return std::unique_lock{ mutex() };
            }
        }

        [[nodiscard]] auto lock_write() { return std::unique_lock{ mutex() }; }

        Value                   m_value;
        mutable UnderlyingMutex m_mutex;
    };

    // deduction guide
    template <typename T>
    Sync(T) -> Sync<T>;
}

#endif /* end of include guard: SYNC_CPP_SYNC_CPP_49PEW7R6FEO7DS */
