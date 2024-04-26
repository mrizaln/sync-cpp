#include <sync_cpp/sync.hpp>

#include <boost/ut.hpp>

#include <algorithm>
#include <concepts>
#include <format>
#include <iostream>
#include <shared_mutex>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

#define ENABLE_OLD_TEST 0

#ifndef ENABLE_OLD_TEST
#    define ENABLE_OLD_TEST 0
#endif

#ifndef ENABLE_DEADLOCK_CODE
#    define ENABLE_DEADLOCK_CODE 0
#endif

#ifndef ENABLE_PRINT
#    define ENABLE_PRINT 0
#endif

#if ENABLE_PRINT
#    include "print.hpp"
#else
void print(auto&&...)
{
    /* no-op*/
}
#endif

class Mutex : public std::shared_mutex
{
public:
    Mutex()                        = default;
    Mutex(const Mutex&)            = delete;
    Mutex& operator=(const Mutex&) = delete;
    Mutex(Mutex&&)                 = delete;
    Mutex& operator=(Mutex&&)      = delete;

    void lock()
    {
        print(">>> unique lock\n");
        std::shared_mutex::lock();
    }

    void unlock()
    {
        print("<<< unique unlock\n");
        std::shared_mutex::unlock();
    }

    void lock_shared()
    {
        print("+++ shared lock\n");
        std::shared_mutex::lock_shared();
    }

    void unlock_shared()
    {
        print("--- shared unlock\n");
        std::shared_mutex::unlock_shared();
    }
};

struct AClass
{
    int                  m_value;
    auto                 operator<=>(const AClass&) const = default;
    friend std::ostream& operator<<(std::ostream& os, const AClass& a)
    {
        return os << "AClass { " << a.m_value << " }";
    }
};

struct BClass
{
    char                 m_value;
    auto                 operator<=>(const BClass&) const = default;
    friend std::ostream& operator<<(std::ostream& os, const BClass& b)
    {
        return os << "BClass { " << b.m_value << " }";
    }
};

class SomeClass
{
public:
    inline static int m_idCounter{ 0 };
    int               m_id;

    explicit SomeClass(std::string name, int value)
        : m_id{ ++m_idCounter }
        , m_name{ std::move(name) }
        , m_value{ value }
    {
        print("\tSomeClass Constructing '{}'\n", m_name);
    }

    BClass doModification()
    {
        m_value++;
        std::reverse(m_name.begin(), m_name.end());
        print("\tSomeClass Doing something '{}'\n", m_value);
        return { m_name.empty() ? '-' : m_name.back() };
    }

    AClass doConstOperation() const
    {
        print("\tSomeClass Doing something const: '{}' | '{}'\n", m_value, m_name);
        return { m_value };
    }

    void doSomethingWithArgs(AClass& aClass, BClass bClass)
    {
        m_value += bClass.m_value - aClass.m_value;
        print("\tSomeClass Doing something with args: '{}', '{}'\n", aClass.m_value, bClass.m_value);
    }

    std::string concatName(const std::string& name) const { return m_name + name; }

    const std::string& getName() const { return m_name; }
    std::string        getNameCopy() const { return m_name; }

private:
    std::string m_name;
    int         m_value;
};

struct CopyCounter
{
    CopyCounter()
        : m_id{ ++s_idCounter }
    {
    }
    CopyCounter(const CopyCounter& other)
        : m_id{ ++s_idCounter }
        , m_copyCount{ other.m_copyCount + 1 }
    {
    }
    CopyCounter& operator=(const CopyCounter& other)
    {
        m_id        = ++s_idCounter;
        m_copyCount = other.m_copyCount + 1;
        return *this;
    }

    CopyCounter(CopyCounter&&)            = default;
    CopyCounter& operator=(CopyCounter&&) = default;

    static inline int s_idCounter = 0;
    int               m_id        = 0;
    int               m_copyCount = 0;
};

template <typename... Args>
std::string fmt(std::format_string<Args...>&& fmt, Args&&... args)
{
    return std::format(std::forward<std::format_string<Args...>>(fmt), std::forward<Args>(args)...);
};

using namespace spp;

int main()
{
    namespace ut = boost::ut;
    using namespace ut::literals;
    using namespace ut::operators;
    using ut::reflection::type_name;

    ut::suite syncConstruction [[maybe_unused]] = [] {
        using Resource     = SomeClass;
        using SyncResource = Sync<SomeClass, Mutex>;

        "Forward construction"_test = [&] {
            ut::expect(true == std::constructible_from<SyncResource, std::string, int>)
                << fmt("Construction with '{}' ctor arguments is not possible when it should be!\n",
                       type_name<Resource>());
        };

        "Resource move construction"_test = [&] {
            ut::expect(true == std::constructible_from<SyncResource, Resource&&>)
                << fmt("Construction from moved '{}' instance is not possible when it should be!\n",
                       type_name<Resource&&>());
        };

        "Resource copy construction"_test = [&] {
            ut::expect(true == std::constructible_from<SyncResource, Resource>)
                << fmt("Construction from copy '{}' is not possible when it should be!\n", type_name<Resource>());
        };

        "Sync copy construction"_test = [&] {
            ut::expect(false == std::constructible_from<SyncResource, SyncResource&>)
                << fmt("Construction from copy '{}' is possible when it should not!\n", type_name<SyncResource&>());
        };

        // // I need to rethink this decision
        // ut::skip / "Sync Move construction"_test = [&] {
        //     ut::expect(false == std::constructible_from<SyncResource, SyncResource&&>)
        //         << fmt("Construction from copy '{}' is possible when it should not!\n", type_name<SyncResource&&>());
        // };
    };

    ut::suite syncGetMember [[maybe_unused]] = [] {
        using Resource     = SomeClass;
        using SyncResource = Sync<SomeClass, Mutex>;

        SyncResource synced{ "resource", 42 };

        "Get member"_test =
            [&]<typename Mem>(Mem&& mem) {
                using Ret      = decltype(std::declval<Resource>().m_id);
                using RetNoRef = std::remove_reference_t<Ret>;

                using Result = decltype(synced.get(mem));
                ut::expect(true == std::same_as<Ret, Result>)
                    << fmt("Get member '{}' returns '{}' instead of '{}'\n",
                           type_name<Mem>(),
                           type_name<Result>(),
                           type_name<RetNoRef>());
            }
            | std::tuple{
                  &Resource::m_id,
              };
    };

    ut::suite syncRead [[maybe_unused]] = [] {
        using Resource     = SomeClass;
        using SyncResource = Sync<SomeClass, Mutex>;

        SyncResource synced{ "read resource", 42 };
        Resource     resource{ "read resource", 42 };    // identical to SyncResource stored value

        // I'll skip the invocable test, it's just too hard!
        "Read using callable"_test =
            [&]<typename Callable>(Callable&& callable) {
                using Arg = const Resource&;
                using Ret = std::invoke_result_t<Callable, Arg>;

                // type equality check
                using Result = decltype(synced.read(callable));
                ut::expect(true == std::same_as<Ret, Result>)
                    << fmt("Read operation using '{}' callable returns '{}' instead of '{}'\n",
                           type_name<Callable>(),
                           type_name<Result>(),
                           type_name<Ret>());

                // value equality check
                if constexpr (!std::same_as<Result, void>) {
                    decltype(auto) result  = synced.read(callable);
                    decltype(auto) result2 = callable(resource);
                    ut::expect(ut::that % result == result2) << "Return value should be the same!";
                }
            }
            | std::tuple{
                  [](const Resource& r) { return r.doConstOperation(); },
                  [](const Resource& r) { return r.getName(); },
                  [](auto& r) { return r.getNameCopy(); },    // auto& is deduced to const SomeClass&
                  [](auto& r) { r.getNameCopy(); },           // void return
              };

        // I'll skip the invocable test, it's just too hard!
        "Read using member function (no args)"_test =
            [&]<typename MemFunc>(MemFunc&& memFunc) {
                using Arg = const Resource&;
                using Ret = std::invoke_result_t<MemFunc, Arg>;

                // type equality check
                using Result = decltype(synced.read(memFunc));
                ut::expect(true == std::same_as<Ret, Result>)
                    << fmt("Read operation using '{}' member function returns '{}' instead of '{}'\n",
                           type_name<MemFunc>(),
                           type_name<Result>(),
                           type_name<Ret>());

                // value equality check
                if constexpr (!std::same_as<Result, void>) {
                    decltype(auto) result  = synced.read(memFunc);
                    decltype(auto) result2 = (resource.*memFunc)();
                    ut::expect(ut::that % result == result2) << "Return value should be the same!";
                }
            }
            | std::tuple{
                  &Resource::doConstOperation,
                  &Resource::getNameCopy,
              };

        "Read using member function with argument"_test = [&] {
            using MemFunc = decltype(&Resource::concatName);
            using ThisArg = Resource&;
            using Arg     = const std::string&;
            using Ret     = std::invoke_result_t<MemFunc, ThisArg, Arg>;

            const std::string suffix{ "suffix" };

            // type equality check
            using Result = decltype(synced.read(&Resource::concatName, suffix));
            ut::expect(true == std::same_as<Ret, Result>)
                << fmt("Read operation using '{}' member function returns '{}' instead of '{}'\n",
                       type_name<MemFunc>(),
                       type_name<Result>(),
                       type_name<Ret>());

            // value equality check
            if constexpr (!std::same_as<Result, void>) {
                decltype(auto) result  = synced.read(&Resource::concatName, suffix);
                decltype(auto) result2 = resource.concatName(suffix);
                ut::expect(ut::that % result == result2) << "Return value should be the same!";
            }
        };
    };

    ut::suite syncWrite [[maybe_unused]] = [] {
        using Resource     = SomeClass;
        using SyncResource = Sync<SomeClass, Mutex>;

        SyncResource synced{ "write resource", 42 };
        Resource     resource{ "write resource", 42 };    // identical to SyncResource stored value

        // I'll skip the invocable test, it's just too hard!
        "Write using callable"_test =
            [&]<typename Callable>(Callable&& callable) {
                using Arg = Resource&;
                using Ret = std::invoke_result_t<Callable, Arg>;

                // type equality check
                using Result = decltype(synced.write(callable));
                ut::expect(true == std::same_as<Ret, Result>)
                    << fmt("Write operation using '{}' callable returns '{}' instead of '{}'\n",
                           type_name<Callable>(),
                           type_name<Result>(),
                           type_name<Ret>());

                // value equality check
                if constexpr (!std::same_as<Result, void>) {
                    decltype(auto) result  = synced.write(callable);
                    decltype(auto) result2 = callable(resource);
                    ut::expect(ut::that % result == result2) << "Return value should be the same!";
                }
            }
            | std::tuple{
                  [](Resource& r) { return r.doModification(); },
                  [](Resource& r) { return r.getName(); },
                  [](auto& r) { return r.getNameCopy(); },    // auto& is deduced to const SomeClass&
                  [](auto& r) { r.getNameCopy(); },           // void return
              };

        "Write using member function (no args)"_test =
            [&]<typename MemFunc>(MemFunc&& memFunc) {
                using Arg = Resource&;
                using Ret = std::invoke_result_t<MemFunc, Arg>;

                // type equality check
                using Result = decltype(synced.write(memFunc));
                ut::expect(true == std::same_as<Ret, Result>)
                    << fmt("Write operation using '{}' member function returns '{}' instead of '{}'\n",
                           type_name<MemFunc>(),
                           type_name<Result>(),
                           type_name<Ret>());

                // value equality check
                if constexpr (!std::same_as<Result, void>) {
                    decltype(auto) result  = synced.write(memFunc);
                    decltype(auto) result2 = (resource.*memFunc)();
                    ut::expect(ut::that % result == result2) << "Return value should be the same!";
                }
            }
            | std::tuple{
                  &Resource::doModification,
                  // &Resource::doConstOperation,    // calling write when no write happening considered ill-formed
                  // &Resource::getNameCopy,         // calling write when no write happening considered ill-formed
              };

        "Write using member function with argument"_test = [&] {
            using MemFunc = decltype(&Resource::doSomethingWithArgs);
            using ThisArg = Resource&;
            using Ret     = std::invoke_result_t<MemFunc, ThisArg, AClass&, BClass>;

            AClass aClass{ 12 };
            BClass bClass{ 3 };

            // last param do explicit copy (in turn create a temporary) to be able to call the function that requires a
            // copy of BClass as the last argument.
            using Result = decltype(synced.write(&Resource::doSomethingWithArgs, aClass, BClass{ bClass }));

            // type equality check
            ut::expect(true == std::same_as<Ret, Result>)
                << fmt("Write operation using '{}' member function returns '{}' instead of '{}'\n",
                       type_name<MemFunc>(),
                       type_name<Result>(),
                       type_name<Ret>());
        };
    };

    "Get mutex to lock it outside"_test = [&] {
        using namespace std::chrono_literals;
        using String = spp::Sync<std::string, std::mutex>;

        String       string{ "hello" };
        std::jthread thread{ [&](const std::stop_token& st) {
            while (!st.stop_requested()) {
                string.write([](std::string& str) { str.push_back('o'); });
                std::this_thread::sleep_for(100ms);
            }
        } };

        std::this_thread::sleep_for(1s);
        {
            std::unique_lock lock{ string.mutex() };
            std::this_thread::sleep_for(200ms);
        }
        std::this_thread::sleep_for(1s);
        {
            const String&    stringRef{ string };
            std::unique_lock lock{ stringRef.mutex() };
            std::this_thread::sleep_for(200ms);
        }
        thread.request_stop();
    };

    "Using external mutex"_test = [&] {
        using namespace std::literals;
        using String = spp::Sync<std::string, std::mutex, false>;

        std::mutex mutex;
        String     string{ mutex, "hello"s };

        std::jthread thread{ [&](const std::stop_token& st) {
            while (!st.stop_requested()) {
                string.write([](std::string& str) { str.push_back('o'); });
                std::this_thread::sleep_for(100ms);
            }
        } };

        std::this_thread::sleep_for(1s);
        {
            std::unique_lock lock{ mutex };
            std::this_thread::sleep_for(200ms);
        }
        std::this_thread::sleep_for(1s);
        {
            std::unique_lock lock{ mutex };
            std::this_thread::sleep_for(200ms);
        }
        thread.request_stop();
    };

    "Perfect forwarding"_test = [&] {
        using boost::ut::_i;
        using Data = CopyCounter;
        class DataUser
        {
        public:
            int useconst(Data&& data) const
            {
                Data moved{ std::move(data) };
                return moved.m_id;
            }
            int use(Data&& data)
            {
                Data moved{ std::move(data) };
                return moved.m_id;
            }
        };

        spp::Sync<DataUser> syncData{};

        Data data1;
        auto id_1     = data1.m_id;
        auto id_1_res = syncData.read(&DataUser::useconst, std::move(data1));
        ut::expect(id_1 == _i(id_1_res)) << fmt("Mismatching id");

        Data data2;
        auto id_2     = data2.m_id;
        auto id_2_res = syncData.write(&DataUser::use, std::move(data2));
        ut::expect(id_2 == _i(id_2_res)) << fmt("Mismatching id");
    };

#if ENABLE_OLD_TEST
    Sync<SomeClass, Mutex> synced{ "SomeClass instance 1", 42 };

    // construction by moving the to-be-synced object
    SomeClass someClass2{ "SomeClass instance 2", 42 };
    Sync      synced2{ std::move(someClass2) };    // can't copy the resource, remove std::move to do it

    SomeClass& someClass22{ someClass2 };
    Sync       synced22{ someClass22 };    // can't copy the resource, remove std::move to do it

    // read from or write to the synced object using lambda
    std::ignore = synced.read([](auto& v) {    // auto& is deduced to const SomeClass&, I can't constraint this behavior
        print("[DEBUG] {} at {}: decltype(v) = {}\n", __FILE__, __LINE__, ut::reflection::type_name<decltype(v)>());
        return v.doConstOperation();
    });
    std::ignore = synced.write([](auto& v) {
        v.doModification();
        return true;
    });

    // invoke the member function through read (const) or write (non-const)
    auto aClass = synced.read(&SomeClass::doConstOperation);    // OK: using read to invoke const member function
    // std::ignore = synced.read(&SomeClass::doModification);   // FAIL: using read to invoke non-const member
    // function

    std::ignore = synced.write(&SomeClass::doModification);    // OK: using write to invoke non-const member function
    // std::ignore = synced.write(&SomeClass::doConstOperation);    // FAIL: using write to invoke const member function

    // invoke the member function with arguments (constness matters)
    // synced.write(&SomeClass::doSomethingWithArgs, aClass, 42); // FAIL: using write to invoke non-const member fn
    // synced.read(&SomeClass::doSomethingWithArgs, aClass, 42);  // FAIL: using read to invoke non-const member
    // func

    // auto name  = synced.read(&SomeClass::getName);         // FAIL: member function returns a reference
    // auto name2 = synced.write(&SomeClass::getName);        // FAIL: member function returns a reference
    auto name = synced.read(&SomeClass::getNameCopy);    // OK: member function NOT returns a reference
    // auto name2 = synced.write(&SomeClass::getNameCopy);    // FAIL: using write to invoke non-const member function

    auto name3 = synced.read([](const SomeClass& c) { return c.getName(); });         // OK: returns copy
    auto name4 = synced.write([](const SomeClass& c) { return c.getName(); });        // OK: returns copy
    auto name5 = synced.read([](const SomeClass& c) { return c.getNameCopy(); });     // OK: returns copy
    auto name6 = synced.write([](const SomeClass& c) { return c.getNameCopy(); });    // OK: returns copy

    // multiple reader (recursive locks) (will deadlock if using std::mutex but not if using std::shared_mutex)
    if constexpr (std::derived_from<typename decltype(synced)::Mutex_type, std::shared_mutex>) {
        synced.read([&synced](const auto& v) {
            using M = decltype(synced)::Mutex_type;
            print(
                "[DEBUG]: recursive locks using {} ({})\n",
                type_name<M>(),
                std::derived_from<M, std::shared_mutex> ? "won't deadlock" : "deadlock"
            );
            v.doConstOperation();
            std::ignore = synced.read([](const auto& v) { return v.doConstOperation(); });
            std::ignore = synced.read(&SomeClass::doConstOperation);
        });
    } else {
        print("[DEBUG]: using std::mutex: skipping recursive lock test\n");
    }

    // writer and reader deadlock (throws if std::shared_mutex is used)
    if constexpr (ENABLE_DEADLOCK_CODE) {
        synced.write([&synced](auto&) {
            print("[DEBUG]: deadlock! you can't recover from this!\n");
            std::ignore = synced.read([](const auto& v) { return v.doConstOperation(); });
            std::ignore = synced.read(&SomeClass::doConstOperation);
        });
    } else {
        print("[DEBUG]: ENABLE_DEADLOCK_CODE is disabled: skipping deadlock test\n");
    }

    // another writer and reader deadlock (no throws! silent bug!)
    if constexpr (ENABLE_DEADLOCK_CODE) {
        synced.read([&synced](const auto&) {
            print("[DEBUG]: deadlock! you can't recover from this!\n");
            std::ignore = synced.write(&SomeClass::doModification);
            std::ignore = synced.read(&SomeClass::doConstOperation);
        });
    }
    {
        print("[DEBUG]: ENABLE_DEADLOCK_CODE is disabled: skipping deadlock test\n");
    }

    auto& mutex = synced.mutex();

    const Sync<SomeClass, Mutex> synced3{ "SomeClass instance 1", 42 };
    std::ignore = synced3.read(&SomeClass::doConstOperation);
    // std::ignore = synced3.write(&SomeClass::doConstOperation);

    auto& mutex3 = synced3.mutex();
#endif
}
