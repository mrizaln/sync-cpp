#ifndef SYNC_OPT_HP7FTB9E4
#define SYNC_OPT_HP7FTB9E4

#include "sync_container.hpp"

#include <optional>

namespace spp
{
    template <bool Checked>
    struct SyncOptAccessor
    {
        decltype(auto) operator()(auto&& opt) const
        {
            if constexpr (Checked) {
                return opt.value();
            } else {
                return *opt;
            }
        }

        auto operator<=>(const SyncOptAccessor&) const = default;
    };

    template <typename T, typename Mtx = std::mutex, bool CheckedAccess = true, bool InternalMutex = true>
    class SyncOpt
        : public SyncContainer<std::optional<T>, T, SyncOptAccessor<CheckedAccess>, Mtx, InternalMutex>
    {
    public:
        using SyncBase
            = SyncContainer<std::optional<T>, T, SyncOptAccessor<CheckedAccess>, Mtx, InternalMutex>;

        using Value   = typename SyncBase::Value;
        using Mutex   = typename SyncBase::Mutex;
        using Element = T;

        SyncOpt(const SyncOpt&)            = delete;
        SyncOpt(SyncOpt&&)                 = delete;
        SyncOpt& operator=(const SyncOpt&) = delete;
        SyncOpt& operator=(SyncOpt&&)      = delete;

        template <typename... Args>
            requires std::constructible_from<T, Args...> && InternalMutex
        SyncOpt(Args&&... args)
            : SyncBase{ std::in_place, std::forward<Args>(args)... }
        {
        }

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
            requires std::constructible_from<T, Args...> && (not InternalMutex)
        SyncOpt(Mtx& mutex, Args&&... args)
            : SyncBase{ mutex, std::in_place, std::forward<Args>(args)... }
        {
        }

        explicit SyncOpt(Mtx& mutex, T&& val)
            requires (not InternalMutex && std::movable<T>)
            : SyncBase{ mutex, std::move(val) }
        {
        }

        explicit SyncOpt(Mtx& mutex, const std::nullopt_t& null)
            requires (not InternalMutex)
            : SyncBase{ mutex, null }
        {
        }

        explicit operator bool() const
        {
            return SyncBase::read([](const std::optional<T>& opt) { return opt.has_value(); });
        }

        bool has_value() const { return static_cast<bool>(*this); }

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
