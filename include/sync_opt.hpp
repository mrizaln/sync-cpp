#ifndef SYNC_OPT_HP7FTB9E4
#define SYNC_OPT_HP7FTB9E4

#include "sync_container.hpp"

#include <optional>
#include <mutex>

namespace spp
{
    template <typename T, bool Checked>
    class SyncOptAccessor
    {
    private:
        using Opt = std::optional<T>;

    public:
        using Getter = std::conditional_t<
            Checked,
            decltype([](Opt& o) -> decltype(auto) { return o.value(); }),    // std::bad_optional_access
            decltype([](Opt& o) -> decltype(auto) { return *o; })>;
        using GetterConst = std::conditional_t<
            Checked,
            decltype([](const Opt& o) -> decltype(auto) { return o.value(); }),    // std::bad_optional_access
            decltype([](const Opt& opt) -> decltype(auto) { return *opt; })>;
    };

    template <typename T, typename M = std::mutex, bool CheckedAccess = false>
    class SyncOpt : public SyncContainer<
                        std::optional<T>,
                        T,
                        typename SyncOptAccessor<T, CheckedAccess>::Getter,
                        typename SyncOptAccessor<T, CheckedAccess>::GetterConst,
                        M>
    {
    public:
        using SyncBase = SyncContainer<
            std::optional<T>,
            T,
            typename SyncOptAccessor<T, CheckedAccess>::Getter,
            typename SyncOptAccessor<T, CheckedAccess>::GetterConst,
            M>;
        using Value_type   = typename SyncBase::Value_type;
        using Mutex_type   = typename SyncBase::Mutex_type;
        using Element_type = T;

        template <typename... Args>
            requires std::constructible_from<T, Args...>
        SyncOpt(Args&&... args)
            : SyncBase(std::in_place, std::forward<Args>(args)...)
        {
        }

        SyncOpt(std::optional<T>&& opt)
            : SyncBase(std::move(opt))
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
}

#endif /* end of include guard: SYNC_OPT_HP7FTB9E4 */
