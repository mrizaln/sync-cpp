#ifndef SYNC_OPT_HP7FTB9E4
#define SYNC_OPT_HP7FTB9E4

#include "sync_container.hpp"

#include <optional>

namespace spp
{
    template <typename T, bool Checked>
    struct SyncOptAccessor
    {
        // clang-format off
        using Opt = std::optional<T>;
        T&       operator()(Opt&       opt) const requires Checked   { return opt.value(); }
        const T& operator()(const Opt& opt) const requires Checked   { return opt.value(); }
        T&       operator()(Opt&       opt) const requires(!Checked) { return *opt; }
        const T& operator()(const Opt& opt) const requires(!Checked) { return *opt; }

        auto operator<=>(const SyncOptAccessor&) const = default;
        // clang-format on
    };

    template <typename T, typename Mutex = std::mutex, bool CheckedAccess = true, bool InternalMutex = true>
    class SyncOpt : public SyncContainer<
                        std::optional<T>,
                        T,
                        SyncOptAccessor<T, CheckedAccess>,
                        SyncOptAccessor<T, CheckedAccess>,
                        Mutex,
                        InternalMutex>
    {
    public:
        using SyncBase = SyncContainer<
            std::optional<T>,
            T,
            SyncOptAccessor<T, CheckedAccess>,
            SyncOptAccessor<T, CheckedAccess>,
            Mutex,
            InternalMutex>;
        using Value_type   = typename SyncBase::Value_type;
        using Mutex_type   = typename SyncBase::Mutex_type;
        using Element_type = T;

        template <typename... Args>
            requires std::constructible_from<T, Args...> && InternalMutex
        SyncOpt(Args&&... args)
            : SyncBase{ std::in_place, std::forward<Args>(args)... }
        {
        }

        SyncOpt(const SyncOpt&)            = delete;
        SyncOpt(SyncOpt&&)                 = delete;
        SyncOpt& operator=(const SyncOpt&) = delete;
        SyncOpt& operator=(SyncOpt&&)      = delete;

        explicit SyncOpt(T&& val)
            requires InternalMutex && std::movable<T>
            : SyncBase{ std::move(val) }
        {
        }

        explicit SyncOpt(const std::nullopt_t& null)
            requires InternalMutex
            : SyncBase{ null }
        {
        }

        template <typename... Args>
            requires std::constructible_from<T, Args...> && (!InternalMutex)
        SyncOpt(Mutex& mutex, Args&&... args)
            : SyncBase{ mutex, std::in_place, std::forward<Args>(args)... }
        {
        }

        explicit SyncOpt(Mutex& mutex, T&& val)
            requires(!InternalMutex && std::movable<T>)
            : SyncBase{ mutex, std::move(val) }
        {
        }

        explicit SyncOpt(Mutex& mutex, const std::nullopt_t& null)
            requires(!InternalMutex)
            : SyncBase{ mutex, null }
        {
        }

        explicit operator bool() const
        {
            return SyncBase::read([](const std::optional<T>& opt) { return opt.has_value(); });
        }

        bool hasValue() const { return static_cast<bool>(*this); }

        void reset()
        {
            SyncBase::write([](std::optional<T>& opt) { opt.reset(); });
        }

        void emplace(T&& value)
        {
            SyncBase::write([&value](std::optional<T>& opt) { opt.emplace(std::move(value)); });
        }

        SyncOpt& operator=(std::optional<T>&& opt)
        {
            SyncBase::write([&opt](std::optional<T>& o) { o = std::move(opt); });
            return *this;
        }
    };

    template <typename T>
    SyncOpt(T&&) -> SyncOpt<T>;
}

#endif /* end of include guard: SYNC_OPT_HP7FTB9E4 */
