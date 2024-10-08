#ifndef SYNC_SMART_PTR_PSV2QWED
#define SYNC_SMART_PTR_PSV2QWED

#include "sync_container.hpp"

#include <memory>

namespace spp
{
    template <bool Checked>
    struct SyncSmartPtrAccessor
    {
        template <typename From>
        static decltype(auto) deref(From&& from) noexcept
        {
            if constexpr (std::is_const_v<std::remove_reference_t<From>>) {
                return std::as_const(*from);
            } else {
                return *from;
            }
        }

        decltype(auto) operator()(auto&& sp) const noexcept(not Checked)
            requires Checked
        {
            if constexpr (Checked) {
                return sp ? deref(sp) : throw std::runtime_error{ "Trying to access SyncSmartPtr with nullptr value!" };
            } else {
                return deref(sp);
            }
        }
    };

    template <
        detail::concepts::SmartPointer SP,
        typename Mutex     = std::mutex,
        bool CheckedAccess = true,
        bool InternalMutex = true>
    class SyncSmartPtr
        : public SyncContainer<SP, typename SP::element_type, SyncSmartPtrAccessor<CheckedAccess>, Mutex, InternalMutex>
    {
    public:
        using SyncBase
            = SyncContainer<SP, typename SP::element_type, SyncSmartPtrAccessor<CheckedAccess>, Mutex, InternalMutex>;

        using Value_type   = typename SyncBase::Value_type;
        using Mutex_type   = typename SyncBase::Mutex_type;
        using Element_type = typename SP::element_type;

        template <typename... Args>
            requires std::constructible_from<SP, Args...> && InternalMutex
        SyncSmartPtr(Args&&... args)
            : SyncBase{ std::forward<Args>(args)... }
        {
        }

        SyncSmartPtr(SP&& sptr)
            requires InternalMutex
            : SyncBase{ std::move(sptr) }
        {
        }

        template <typename... Args>
            requires std::constructible_from<SP, Args...> && (!InternalMutex)
        SyncSmartPtr(Mutex& mutex, Args&&... args)
            : SyncBase{ mutex, std::forward<Args>(args)... }
        {
        }

        SyncSmartPtr(Mutex& mutex, SP&& sptr)
            requires(!InternalMutex)
            : SyncBase{ mutex, std::move(sptr) }
        {
        }

        explicit operator bool() const
        {
            return SyncBase::read([](const SP& sptr) { return static_cast<bool>(sptr); });
        }

        bool hasValue() const { return static_cast<bool>(*this); }

        void reset(Element_type* ptr = nullptr)
        {
            SyncBase::write([ptr](SP& sp) { sp.reset(ptr); });
        }

        SyncSmartPtr& operator=(SP&& sptr)
        {
            SyncBase::write([&sptr](SP& sp) { sp = std::move(sptr); });
            return *this;
        }
    };

    // aliases
    template <typename T, typename M = std::mutex, bool CheckedAccess = true, bool InternalMutex = true>
    using SyncUnique = SyncSmartPtr<std::unique_ptr<T>, M, CheckedAccess, InternalMutex>;

    template <typename T, typename D, typename M = std::mutex, bool CheckedAccess = true, bool InternalMutex = true>
    using SyncUniqueCustom = SyncSmartPtr<std::unique_ptr<T, D>, M, CheckedAccess, InternalMutex>;

    template <typename T, typename M = std::mutex, bool CheckedAccess = true, bool InternalMutex = true>
    using SyncShared = SyncSmartPtr<std::shared_ptr<T>, M, CheckedAccess, InternalMutex>;

    // deduction guides
    template <typename T, typename D = std::default_delete<T>>
    SyncSmartPtr(std::unique_ptr<T, D>) -> SyncSmartPtr<std::unique_ptr<T, D>>;

    template <typename T>
    SyncSmartPtr(std::shared_ptr<T>) -> SyncSmartPtr<std::shared_ptr<T>>;
}

#endif /* end of include guard: SYNC_SMART_PTR_PSV2QWED */
