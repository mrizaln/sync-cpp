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

#define ENABLE_OLD_TEST      1
#define ENABLE_PRINT         1
#define ENABLE_DEADLOCK_CODE 0

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
#    include <fmt/core.h>
#    define print(...) fmt::print(__VA_ARGS__)
#else
#    define print(...)
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
    inline static int s_id_counter{ 0 };
    int               m_id;

    explicit SomeClass(std::string name, int value)
        : m_id{ ++s_id_counter }
        , m_name{ std::move(name) }
        , m_value{ value }
    {
        print("\tSomeClass Constructing '{}'\n", m_name);
    }

    BClass do_modification()
    {
        m_value++;
        std::reverse(m_name.begin(), m_name.end());
        print("\tSomeClass Doing something '{}'\n", m_value);
        return { m_name.empty() ? '-' : m_name.back() };
    }

    AClass do_const_operation() const
    {
        print("\tSomeClass Doing something const: '{}' | '{}'\n", m_value, m_name);
        return { m_value };
    }

    void do_something_with_args(AClass& a, BClass b)
    {
        m_value += b.m_value - a.m_value;
        print("\tSomeClass Doing something with args: '{}', '{}'\n", a.m_value, b.m_value);
    }

    std::string concat_name(const std::string& name) const { return m_name + name; }

    const std::string& get_name() const { return m_name; }
    std::string        get_name_copy() const { return m_name; }

private:
    std::string m_name;
    int         m_value;
};

struct CopyCounter
{
    static inline int s_id_counter = 0;

    CopyCounter()
        : m_id{ ++s_id_counter }
    {
    }

    CopyCounter(const CopyCounter& other)
        : m_id{ ++s_id_counter }
        , m_copy_count{ other.m_copy_count + 1 }
    {
    }

    CopyCounter& operator=(const CopyCounter& other)
    {
        m_id         = ++s_id_counter;
        m_copy_count = other.m_copy_count + 1;
        return *this;
    }

    CopyCounter(CopyCounter&&)            = default;
    CopyCounter& operator=(CopyCounter&&) = default;

    int m_id         = 0;
    int m_copy_count = 0;
};

template <typename... Args>
std::string fmtt(std::format_string<Args...>&& fmt, Args&&... args)
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

    ut::suite sync_construction [[maybe_unused]] = [] {
        using Resource     = SomeClass;
        using SyncResource = Sync<SomeClass, Mutex>;

        "Forward construction"_test = [&] {
            ut::expect(true == std::constructible_from<SyncResource, std::string, int>) << fmtt(
                "Construction with '{}' ctor arguments is not possible when it should be!\n",
                type_name<Resource>()
            );
        };

        "Resource move construction"_test = [&] {
            ut::expect(true == std::constructible_from<SyncResource, Resource&&>) << fmtt(
                "Construction from moved '{}' instance is not possible when it should be!\n",
                type_name<Resource&&>()
            );
        };

        // NOTE: Making Sync movable will make things easier for the user, but might introduce bug if the
        // moved-from value is used just after it is moved (like if a reader/writer was in queue before it was
        // moved since the mutex will be hold for the entire moving procedure.
        "Sync move construction"_test = [&] {
            ut::expect(false == std::constructible_from<SyncResource, SyncResource&&>) << fmtt(
                "Construction from moved'{}' is possible when it should not!\n", type_name<SyncResource&&>()
            );
        };

        "Resource copy construction"_test = [&] {
            ut::expect(true == std::constructible_from<SyncResource, Resource>) << fmtt(
                "Construction from copy '{}' is not possible when it should be!\n", type_name<Resource>()
            );
        };

        "Sync copy construction"_test = [&] {
            ut::expect(false == std::constructible_from<SyncResource, SyncResource&>) << fmtt(
                "Construction from copy '{}' is possible when it should not!\n", type_name<SyncResource&>()
            );
        };
    };

    ut::suite sync_get_member [[maybe_unused]] = [] {
        using Resource     = SomeClass;
        using SyncResource = Sync<SomeClass, Mutex>;

        auto synced = SyncResource{ "resource", 42 };

        "Get member"_test = [&]<typename Mem>(Mem&& mem) {
            using Ret      = decltype(std::declval<Resource>().m_id);
            using RetNoRef = std::remove_reference_t<Ret>;

            using Result = decltype(synced.get(mem));
            ut::expect(true == std::same_as<Ret, Result>) << fmtt(
                "Get member '{}' returns '{}' instead of '{}'\n",
                type_name<Mem>(),
                type_name<Result>(),
                type_name<RetNoRef>()
            );
        } | std::tuple{ &Resource::m_id };
    };

    ut::suite sync_read [[maybe_unused]] = [] {
        using Resource     = SomeClass;
        using SyncResource = Sync<SomeClass, Mutex>;

        auto synced   = SyncResource{ "read resource", 42 };
        auto resource = Resource{ "read resource", 42 };    // identical to SyncResource stored value

        // I'll skip the invocable test, it's just too hard!
        "Read using callable"_test =
            [&]<typename Callable>(Callable&& callable) {
                using Arg = const Resource&;
                using Ret = std::invoke_result_t<Callable, Arg>;

                // type equality check
                using Result = decltype(synced.read(callable));
                ut::expect(true == std::same_as<Ret, Result>) << fmtt(
                    "Read operation using '{}' callable returns '{}' instead of '{}'\n",
                    type_name<Callable>(),
                    type_name<Result>(),
                    type_name<Ret>()
                );

                // value equality check
                if constexpr (!std::same_as<Result, void>) {
                    decltype(auto) result  = synced.read(callable);
                    decltype(auto) result2 = callable(resource);
                    ut::expect(ut::that % result == result2) << "Return value should be the same!";
                }
            }
            | std::tuple{
                  [](const Resource& r) { return r.do_const_operation(); },
                  [](const Resource& r) { return r.get_name(); },
                  [](auto& r) { return r.get_name_copy(); },    // auto& is deduced to const SomeClass&
                  [](auto& r) { r.get_name_copy(); },           // void return
              };

        // I'll skip the invocable test, it's just too hard!
        "Read using member function (no args)"_test = [&]<typename MemFunc>(MemFunc&& memFunc) {
            using Arg = const Resource&;
            using Ret = std::invoke_result_t<MemFunc, Arg>;

            // type equality check
            using Result = decltype(synced.read(memFunc));
            ut::expect(true == std::same_as<Ret, Result>) << fmtt(
                "Read operation using '{}' member function returns '{}' instead of '{}'\n",
                type_name<MemFunc>(),
                type_name<Result>(),
                type_name<Ret>()
            );

            // value equality check
            if constexpr (!std::same_as<Result, void>) {
                decltype(auto) result  = synced.read(memFunc);
                decltype(auto) result2 = (resource.*memFunc)();
                ut::expect(ut::that % result == result2) << "Return value should be the same!";
            }
        } | std::tuple{ &Resource::do_const_operation, &Resource::get_name_copy };

        "Read using member function with argument"_test = [&] {
            using MemFunc = decltype(&Resource::concat_name);
            using ThisArg = Resource&;
            using Arg     = const std::string&;
            using Ret     = std::invoke_result_t<MemFunc, ThisArg, Arg>;

            const auto suffix = std::string{ "suffix" };

            // type equality check
            using Result = decltype(synced.read(&Resource::concat_name, suffix));
            ut::expect(true == std::same_as<Ret, Result>) << fmtt(
                "Read operation using '{}' member function returns '{}' instead of '{}'\n",
                type_name<MemFunc>(),
                type_name<Result>(),
                type_name<Ret>()
            );

            // value equality check
            if constexpr (!std::same_as<Result, void>) {
                decltype(auto) result  = synced.read(&Resource::concat_name, suffix);
                decltype(auto) result2 = resource.concat_name(suffix);
                ut::expect(ut::that % result == result2) << "Return value should be the same!";
            }
        };
    };

    ut::suite sync_write [[maybe_unused]] = [] {
        using Resource     = SomeClass;
        using SyncResource = Sync<SomeClass, Mutex>;

        auto synced   = SyncResource{ "write resource", 42 };
        auto resource = Resource{ "write resource", 42 };    // identical to SyncResource stored value

        // I'll skip the invocable test, it's just too hard!
        "Write using callable"_test =
            [&]<typename Callable>(Callable&& callable) {
                using Arg = Resource&;
                using Ret = std::invoke_result_t<Callable, Arg>;

                // type equality check
                using Result = decltype(synced.write(callable));
                ut::expect(true == std::same_as<Ret, Result>) << fmtt(
                    "Write operation using '{}' callable returns '{}' instead of '{}'\n",
                    type_name<Callable>(),
                    type_name<Result>(),
                    type_name<Ret>()
                );

                // value equality check
                if constexpr (!std::same_as<Result, void>) {
                    decltype(auto) result  = synced.write(callable);
                    decltype(auto) result2 = callable(resource);
                    ut::expect(ut::that % result == result2) << "Return value should be the same!";
                }
            }
            | std::tuple{
                  [](Resource& r) { return r.do_modification(); },
                  [](Resource& r) { return r.get_name(); },
                  [](auto& r) { return r.get_name_copy(); },    // auto& is deduced to const SomeClass&
                  [](auto& r) { r.get_name_copy(); },           // void return
              };

        "Write using member function (no args)"_test =
            [&]<typename MemFunc>(MemFunc&& memFunc) {
                using Arg = Resource&;
                using Ret = std::invoke_result_t<MemFunc, Arg>;

                // type equality check
                using Result = decltype(synced.write(memFunc));
                ut::expect(true == std::same_as<Ret, Result>) << fmtt(
                    "Write operation using '{}' member function returns '{}' instead of '{}'\n",
                    type_name<MemFunc>(),
                    type_name<Result>(),
                    type_name<Ret>()
                );

                // value equality check
                if constexpr (!std::same_as<Result, void>) {
                    decltype(auto) result  = synced.write(memFunc);
                    decltype(auto) result2 = (resource.*memFunc)();
                    ut::expect(ut::that % result == result2) << "Return value should be the same!";
                }
            }
            | std::tuple{
                  &Resource::do_modification,
                  // &Resource::do_const_operation,    // calling write when no write happening considered
                  // ill-formed &Resource::get_name_copy,         // calling write when no write happening
                  // considered ill-formed
              };

        "Write using member function with argument"_test = [&] {
            using MemFunc = decltype(&Resource::do_something_with_args);
            using ThisArg = Resource&;
            using Ret     = std::invoke_result_t<MemFunc, ThisArg, AClass&, BClass>;

            auto a = AClass{ 12 };
            auto b = BClass{ 3 };

            // last param do explicit copy (in turn create a temporary) to be able to call the function that
            // requires a copy of BClass as the last argument.
            using Result = decltype(synced.write(&Resource::do_something_with_args, a, BClass{ b }));

            // type equality check
            ut::expect(true == std::same_as<Ret, Result>) << fmtt(
                "Write operation using '{}' member function returns '{}' instead of '{}'\n",
                type_name<MemFunc>(),
                type_name<Result>(),
                type_name<Ret>()
            );
        };
    };

    "Get mutex to lock it outside"_test = [&] {
        using namespace std::chrono_literals;
        using String = spp::Sync<std::string, std::mutex>;

        auto string = String{ "hello" };
        auto thread = std::jthread{ [&](const std::stop_token& st) {
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

        auto mutex  = std::mutex{};
        auto string = String{ mutex, "hello"s };

        auto thread = std::jthread{ [&](const std::stop_token& st) {
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

        auto syncData = spp::Sync<DataUser>{};

        auto data1    = Data{};
        auto id_1     = data1.m_id;
        auto id_1_res = syncData.read(&DataUser::useconst, std::move(data1));
        ut::expect(id_1 == _i(id_1_res)) << fmtt("Mismatching id");

        auto data2    = Data{};
        auto id_2     = data2.m_id;
        auto id_2_res = syncData.write(&DataUser::use, std::move(data2));
        ut::expect(id_2 == _i(id_2_res)) << fmtt("Mismatching id");
    };

#if ENABLE_OLD_TEST
    auto synced = Sync<SomeClass, Mutex>{ "SomeClass instance 1", 42 };

    // construction by moving the to-be-synced object
    auto someClass2 = SomeClass{ "SomeClass instance 2", 42 };
    auto synced2    = Sync{ std::move(someClass2) };    // can't copy the resource, remove std::move to do it

    auto& someClass22 = someClass2;
    auto  synced22    = Sync{ someClass22 };    // can't copy the resource, remove std::move to do it

    // read from or write to the synced object using lambda
    // auto& is deduced to const SomeClass&, I can't constraint this behavior
    auto a      = synced.read([](auto& v) {
        print(
            "!!! {} at {}: decltype(v) = {}\n", __FILE__, __LINE__, ut::reflection::type_name<decltype(v)>()
        );
        return v.do_const_operation();
    });
    std::ignore = synced.write([](auto& v) {
        v.do_modification();
        return true;
    });

    // invoke the member function through read (const) or write (non-const)
    std::ignore = synced.read(&SomeClass::do_const_operation);    // OK: use read to invoke const member
    // std::ignore = synced.read(&SomeClass::do_modification);    // FAIL: use read to invoke non-const member

    std::ignore = synced.write(&SomeClass::do_modification);    // OK: use write invoke non-const member
    // std::ignore = synced.write(&SomeClass::do_const_operation); // FAIL: use write to invoke const member

    // invoke the member function with arguments (constness matters)
    // synced.write(&SomeClass::do_something_with_args, a, 42);    // FAIL: mismatched args
    // synced.read(&SomeClass::do_something_with_args, a, 42);     // FAIL: mismatched args

    // auto name  = synced.read(&SomeClass::get_name);         // FAIL: member function returns a reference
    // auto name2 = synced.write(&SomeClass::get_name);        // FAIL: member function returns a reference
    auto name = synced.read(&SomeClass::get_name_copy);    // OK: member function NOT returns a reference
    // auto name2 = synced.write(&SomeClass::get_name_copy);    // FAIL: using write to invoke non-const
    // member function

    auto name3 = synced.read([](const SomeClass& c) { return c.get_name(); });          // OK: returns copy
    auto name4 = synced.write([](const SomeClass& c) { return c.get_name(); });         // OK: returns copy
    auto name5 = synced.read([](const SomeClass& c) { return c.get_name_copy(); });     // OK: returns copy
    auto name6 = synced.write([](const SomeClass& c) { return c.get_name_copy(); });    // OK: returns copy

    // multiple reader (recursive locks) [ deadlock if using std::mutex but not if using std::shared_mutex ]
    if constexpr (std::derived_from<typename decltype(synced)::Mutex, std::shared_mutex>) {
        synced.read([&synced](const auto& v) {
            using M = decltype(synced)::Mutex;
            print(
                "!!! recursive locks using {} ({})\n",
                type_name<M>(),
                std::derived_from<M, std::shared_mutex> ? "won't deadlock" : "deadlock"
            );
            v.do_const_operation();
            std::ignore = synced.read([](const auto& v) { return v.do_const_operation(); });
            std::ignore = synced.read(&SomeClass::do_const_operation);
        });
    } else {
        print("!!! using std::mutex: skipping recursive lock test\n");
    }

    // writer and reader deadlock (throws if std::shared_mutex is used)
    if constexpr (ENABLE_DEADLOCK_CODE) {
        synced.write([&synced](auto&) {
            print("!!! deadlock! you can't recover from this!\n");
            std::ignore = synced.read([](const auto& v) { return v.do_const_operation(); });
            std::ignore = synced.read(&SomeClass::do_const_operation);
        });
    } else {
        print("!!! ENABLE_DEADLOCK_CODE is disabled: skipping deadlock test\n");
    }

    // another writer and reader deadlock (no throws! silent bug!)
    if constexpr (ENABLE_DEADLOCK_CODE) {
        synced.read([&synced](const auto&) {
            print("!!! deadlock! you can't recover from this!\n");
            std::ignore = synced.write(&SomeClass::do_modification);
            std::ignore = synced.read(&SomeClass::do_const_operation);
        });
    } else {
        print("!!! ENABLE_DEADLOCK_CODE is disabled: skipping deadlock test\n");
    }

    auto& mutex = synced.mutex();

    const auto synced3 = Sync<SomeClass, Mutex>{ "SomeClass instance 1", 42 };
    std::ignore        = synced3.read(&SomeClass::do_const_operation);
    // std::ignore = synced3.write(&SomeClass::do_const_operation);

    auto& mutex3 = synced3.mutex();
#endif
}
