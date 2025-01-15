#ifndef SYNC_CPP_SYNC_SMART_PTR_PSV2QWED
#define SYNC_CPP_SYNC_SMART_PTR_PSV2QWED

#include "sync_container.hpp"

#include <memory>

namespace spp
{
    /**
     * @class SyncSmartPtrAccessor
     *
     * @brief Smart pointer accessor for SyncSmartPtr.
     */
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
        {
            if constexpr (Checked) {
                return sp ? deref(sp)
                          : throw std::runtime_error{ "Trying to access SyncSmartPtr with nullptr value!" };
            } else {
                return deref(sp);
            }
        }
    };

    /**
     * @class SyncSmartPtr
     *
     * @brief A wrapper around a smart pointer with a mutex.
     */
    template <
        concepts::SmartPointer SP,
        concepts::SyncMutex    Mtx           = std::mutex,
        bool                   CheckedAccess = true,
        bool                   InternalMutex = true>
        requires concepts::Syncable<typename SP::element_type>
    class SyncSmartPtr : public SyncContainer<
                             SP,
                             typename SP::element_type,
                             SyncSmartPtrAccessor<CheckedAccess>,
                             Mtx,
                             InternalMutex>
    {
    public:
        using SyncBase = SyncContainer<
            SP,
            typename SP::element_type,
            SyncSmartPtrAccessor<CheckedAccess>,
            Mtx,
            InternalMutex>;

        using Value   = typename SyncBase::Value;
        using Mutex   = typename SyncBase::Mutex;
        using Element = typename SP::element_type;

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
            requires std::constructible_from<SP, Args...> && (not InternalMutex)
        SyncSmartPtr(Mtx& mutex, Args&&... args)
            : SyncBase{ mutex, std::forward<Args>(args)... }
        {
        }

        SyncSmartPtr(Mtx& mutex, SP&& sptr)
            requires (not InternalMutex)
            : SyncBase{ mutex, std::move(sptr) }
        {
        }

        /**
         * @brief Check if the underlying pointer is not nullptr.
         */
        explicit operator bool() const
        {
            return SyncBase::read([](const SP& sptr) { return static_cast<bool>(sptr); });
        }

        /**
         * @brief Check if the underlying pointer is not nullptr.
         */
        bool has_value() const { return static_cast<bool>(*this); }

        /**
         * @brief Reset the underlying pointer to nullptr.
         */
        void reset(Element* ptr = nullptr)
        {
            SyncBase::write([ptr](SP& sp) { sp.reset(ptr); });
        }

        /**
         * @brief Replace the underlying pointer with a new one.
         *
         * @param sptr The new smart pointer.
         */
        SyncSmartPtr& operator=(SP&& sptr)
        {
            SyncBase::write([&sptr](SP& sp) { sp = std::move(sptr); });
            return *this;
        }
    };

    // aliases
    // -------
    /**
     * @brief A wrapper around a unique pointer with a mutex.
     *
     * @tparam T The type of the unique pointer element.
     */
    template <typename T, typename Mtx = std::mutex, bool CheckedAccess = true, bool InternalMutex = true>
    using SyncUnique = SyncSmartPtr<std::unique_ptr<T>, Mtx, CheckedAccess, InternalMutex>;

    /**
     * @brief A wrapper around a unique pointer with a custom deleter and a mutex.
     *
     * @tparam T The type of the unique pointer element.
     * @tparam Delete The type of the deleter.
     */
    template <
        typename T,
        typename Delete,
        typename Mtx       = std::mutex,
        bool CheckedAccess = true,
        bool InternalMutex = true>
    using SyncUniqueCustom = SyncSmartPtr<std::unique_ptr<T, Delete>, Mtx, CheckedAccess, InternalMutex>;

    /**
     * @brief A wrapper around a shared pointer with a mutex.
     *
     * @tparam T The type of the shared pointer element.
     */
    template <typename T, typename Mtx = std::mutex, bool CheckedAccess = true, bool InternalMutex = true>
    using SyncShared = SyncSmartPtr<std::shared_ptr<T>, Mtx, CheckedAccess, InternalMutex>;
    // -------

    // deduction guides
    // ----------------
    template <typename T, typename Delete = std::default_delete<T>>
    SyncSmartPtr(std::unique_ptr<T, Delete>) -> SyncSmartPtr<std::unique_ptr<T, Delete>>;

    template <typename T>
    SyncSmartPtr(std::shared_ptr<T>) -> SyncSmartPtr<std::shared_ptr<T>>;
    // ----------------
}

#endif /* end of include guard: SYNC_CPP_SYNC_SMART_PTR_PSV2QWED */
