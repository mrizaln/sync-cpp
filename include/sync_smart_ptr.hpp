#ifndef SYNC_SMART_PTR_PSV2QWED
#define SYNC_SMART_PTR_PSV2QWED

#include "sync_container.hpp"

#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

namespace spp
{
    // clang-format off
    template <typename T>
    concept SmartPointer = requires(T sptr) {
        // Type alias
        typename T::element_type;

        // Basic operations
        { sptr.get() } -> std::same_as<typename T::element_type*>;
        { sptr.reset() } -> std::same_as<void>;
        { sptr.swap(sptr) } -> std::same_as<void>;

        // Dereference and arrow operator
        { *sptr } -> std::same_as<typename T::element_type&>;
        { sptr.operator->() } -> std::same_as<typename T::element_type*>;

        // Boolean conversion
        { static_cast<bool>(sptr) } -> std::convertible_to<bool>;

        // Comparison operators
        { sptr == nullptr } -> std::convertible_to<bool>;
        { sptr != nullptr } -> std::convertible_to<bool>;
    };
    // clang-format on

    template <SmartPointer SP, bool Checked>
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

    template <SmartPointer SP, typename M = std::mutex, bool CheckedAccess = true>
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
    template <typename T, typename M = std::mutex>
    using SyncUnique = SyncSmartPtr<std::unique_ptr<T>, M>;

    template <typename T, typename D, typename M = std::mutex>
    using SyncUniqueCustom = SyncSmartPtr<std::unique_ptr<T, D>, M>;

    template <typename T, typename M = std::mutex>
    using SyncShared = SyncSmartPtr<std::shared_ptr<T>, M>;

    // deduction guides
    template <typename T, typename D = std::default_delete<T>>
    SyncSmartPtr(std::unique_ptr<T, D>) -> SyncSmartPtr<std::unique_ptr<T, D>>;

    template <typename T>
    SyncSmartPtr(std::shared_ptr<T>) -> SyncSmartPtr<std::shared_ptr<T>>;
}

#endif /* end of include guard: SYNC_SMART_PTR_PSV2QWED */