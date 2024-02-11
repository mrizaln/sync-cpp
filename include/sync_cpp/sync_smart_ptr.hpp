#ifndef SYNC_SMART_PTR_PSV2QWED
#define SYNC_SMART_PTR_PSV2QWED

#include "detail/concepts.hpp"
#include "sync_container.hpp"

#include <concepts>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

namespace spp
{
    template <detail::concepts::SmartPointer SP, bool Checked>
    struct SyncSmartPtrAccessor
    {
        using Getter = std::conditional_t<
            Checked,
            decltype([](SP& sp) -> decltype(auto) {
                return sp ? *sp : throw std::runtime_error{ "Trying to access SyncSmartPtr with nullptr value!" };
            }),
            decltype([](SP& sp) -> decltype(auto) { return *sp; })>;
        using GetterConst = std::conditional_t<
            Checked,
            decltype([](const SP& sp) -> decltype(auto) {
                return sp ? *sp : throw std::runtime_error{ "Trying to access SyncSmartPtr with nullptr value!" };
            }),
            decltype([](const SP& sp) -> decltype(auto) { return *sp; })>;
    };

    template <detail::concepts::SmartPointer SP, typename M = std::mutex, bool CheckedAccess = true>
    class SyncSmartPtr : public SyncContainer<
                             SP,
                             typename SP::element_type,
                             typename SyncSmartPtrAccessor<SP, CheckedAccess>::Getter,
                             typename SyncSmartPtrAccessor<SP, CheckedAccess>::GetterConst,
                             M>
    {
    public:
        using SyncBase = SyncContainer<
            SP,
            typename SP::element_type,
            typename SyncSmartPtrAccessor<SP, CheckedAccess>::Getter,
            typename SyncSmartPtrAccessor<SP, CheckedAccess>::GetterConst,
            M>;
        using Value_type   = typename SyncBase::Value_type;
        using Mutex_type   = typename SyncBase::Mutex_type;
        using Element_type = typename SP::element_type;

        template <typename... Args>
            requires std::constructible_from<SP, Args...>
        SyncSmartPtr(Args&&... args)
            : SyncBase(std::forward<Args>(args)...)
        {
        }

        SyncSmartPtr(SP&& sptr)
            : SyncBase(std::move(sptr))
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
    template <typename T, typename M = std::mutex, bool CheckedAccess = true>
    using SyncUnique = SyncSmartPtr<std::unique_ptr<T>, M, CheckedAccess>;

    template <typename T, typename D, typename M = std::mutex, bool CheckedAccess = true>
    using SyncUniqueCustom = SyncSmartPtr<std::unique_ptr<T, D>, M, CheckedAccess>;

    template <typename T, typename M = std::mutex, bool CheckedAccess = true>
    using SyncShared = SyncSmartPtr<std::shared_ptr<T>, M, CheckedAccess>;

    // deduction guides
    template <typename T, typename D = std::default_delete<T>>
    SyncSmartPtr(std::unique_ptr<T, D>) -> SyncSmartPtr<std::unique_ptr<T, D>>;

    template <typename T>
    SyncSmartPtr(std::shared_ptr<T>) -> SyncSmartPtr<std::shared_ptr<T>>;
}

#endif /* end of include guard: SYNC_SMART_PTR_PSV2QWED */
