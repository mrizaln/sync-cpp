#ifndef SYNC_HPP_B4IRUHHF
#define SYNC_HPP_B4IRUHHF

#include <concepts>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <utility>

namespace spp
{
    template <typename T, typename... Ts>
    concept DerivedFromAny = (std::derived_from<T, Ts> || ...);

    template <typename T, typename M = std::mutex>
        requires(!std::is_reference_v<T> && !std::is_pointer_v<T>)
             && DerivedFromAny<M, std::shared_mutex, std::mutex, std::recursive_mutex>
    class Sync
    {
    public:
        using Value_type = T;
        using Mutex_type = M;

        template <typename... Args>
            requires std::constructible_from<T, Args...>
        Sync(Args&&... args)
            : m_value{ std::forward<Args>(args)... }
        {
        }

        explicit Sync(T&& value)
            : m_value{ std::forward<T>(value) }
        {
        }

        // @brief: Public member access of the wrapped value by copy
        template <typename Mem>
            requires std::is_member_object_pointer_v<Mem>
        [[nodiscard]] auto get(Mem mem) const    // the 'auto' removes reference and cv-qualifier
        {
            auto lock{ readLock() };
            return m_value.*mem;
        }

        // @brief: Convenience function for const member function call of the wrapped value (read-only access)
        template <typename MemFunc, typename... Args>
        [[nodiscard]] decltype(auto) read(MemFunc&& f, Args&&... args) const
            requires std::is_member_function_pointer_v<MemFunc> && std::invocable<MemFunc, const T&, Args...>
                  && requires(Value_type value) { (value.*f)(std::forward<Args>(args)...); }
        {
            static_assert(
                !std::is_lvalue_reference_v<decltype((m_value.*f)(std::forward<Args>(args)...))>,
                "Member function returning a reference in multithreaded context is dangerous!"
            );

            auto lock{ readLock() };
            return (m_value.*f)(std::forward<Args>(args)...);
        }

        // @brief: Convenience function for non-const member function call of the wrapped value (write access)
        template <typename MemFunc, typename... Args>
        [[nodiscard]] decltype(auto) write(MemFunc&& f, Args&&... args)
            requires std::is_member_function_pointer_v<MemFunc> && std::invocable<MemFunc, T&, Args...>
                  && requires(Value_type value) { (value.*f)(std::forward<Args>(args)...); }
        {
            static_assert(
                !std::is_lvalue_reference_v<decltype((m_value.*f)(std::forward<Args>(args)...))>,
                "Member function returning a reference in multithreaded context is dangerous!"
            );

            auto lock{ writeLock() };
            return (m_value.*f)(std::forward<Args>(args)...);
        }

        // @brief: Read-only access to the wrapped value
        template <typename Func>
            requires(!std::is_member_pointer_v<Func>) && std::invocable<Func, const T&>
        [[nodiscard]] decltype(auto) read(Func&& f) const
        {
            static_assert(
                !std::is_lvalue_reference_v<decltype(f(m_value))>,
                "Function returning a reference in multithreaded context is dangerous!"
            );

            auto lock{ readLock() };
            return f(m_value);
        }

        // @brief: Write access to the wrapped value
        template <typename Func>
            requires(!std::is_member_pointer_v<Func>) && std::invocable<Func, T&>
        [[nodiscard]] decltype(auto) write(Func&& f)
        {
            static_assert(
                !std::is_lvalue_reference_v<decltype(f(m_value))>,
                "Function returning a reference in multithreaded context is dangerous!"
            );

            auto lock{ writeLock() };
            return f(m_value);
        }

        template <typename TT>
            requires std::assignable_from<std::decay_t<T>, TT>
        Sync& operator=(TT&& value)
        {
            write([&](auto& v) { v = std::forward<TT>(value); });
            return *this;
        }

        // NOTE: at swap, make sure that both 'this' and 'other' does not have any thread waiting for resource from it
        void swap(Sync& other)
        {
            std::unique_lock lock1{ m_mutex, std::defer_lock };
            std::unique_lock lock2{ other.m_mutex, std::defer_lock };

            std::lock(lock1, lock2);

            std::swap(m_value, other.m_value);
        }

        M& getMutex() { return m_mutex; }
        M& getMutex() const { return m_mutex; }

    private:
        [[nodiscard]] auto readLock() const
        {
            if constexpr (std::derived_from<M, std::shared_mutex>) {
                return std::shared_lock{ m_mutex };
            } else {
                return std::unique_lock{ m_mutex };
            }
        }

        [[nodiscard]] auto writeLock() { return std::unique_lock{ m_mutex }; }

        mutable M  m_mutex;
        Value_type m_value;
    };

    // deduction guide
    template <typename T>
    Sync(T) -> Sync<T>;
}

#endif /* end of include guard: SYNC_HPP_B4IRUHHF */
