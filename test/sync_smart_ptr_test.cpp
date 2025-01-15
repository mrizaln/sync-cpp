#include <sync_cpp/sync_smart_ptr.hpp>

#include <boost/ut.hpp>
#include <fmt/core.h>

#include <cassert>
#include <exception>
#include <memory>

class Some
{
public:
    int m_id;

    Some(int v, std::string name)
        : m_id{ ++m_id_counter }
        , m_value{ v }
        , m_name{ std::move(name) }
    {
        fmt::print("Some created: {} {}\n", m_value, m_name);
    }

    Some(const Some&)            = delete;
    Some(Some&&)                 = delete;
    Some& operator=(const Some&) = delete;
    Some& operator=(Some&&)      = delete;

    ~Some() { fmt::print("Some destroyed: {} {}\n", m_value, m_name); }

    int get() const { return m_value; }
    int modify(int v) { return m_value += v; }

private:
    inline static int m_id_counter{ 0 };

    int         m_value;
    std::string m_name;
};

// clang-format off
Some* create(int v) { return new Some{ v, "create" }; }
void destroy(Some* s) { delete s; }
// clang-format on

using namespace spp;

int main()
{
    auto val = 0;

    // plain type
    // ----------
    auto some = Sync<Some>{ 42, "stack" };    // OK

    val = some.read(&Some::get);
    fmt::print("Some::get '{}'\n", val);

    val = some.write(&Some::modify, 13);
    fmt::print("Some::modify '{}'\n", val);

    // unique_ptr
    // ----------
    auto uniq_some   = SyncUnique<Some>{ std::make_unique<Some>(43, "uniq") };    // OK
    auto uniq_some_2 = SyncUnique<Some>{ new Some{ 43, "uniq" } };                // OK: calling new manually

    val = uniq_some.read_value(&Some::get);
    fmt::print("Some::get '{}'\n", val);

    val = uniq_some.write_value(&Some::modify, 13);
    fmt::print("Some::modify '{}'\n", val);

    // shared_ptr
    // ----------
    auto shared_some   = SyncShared<Some>{ std::make_shared<Some>(44, "shared") };    // OK
    auto shared_some_2 = SyncShared<Some>{ new Some{ 44, "shared" } };    // OK: calling new manually

    val = shared_some.read_value(&Some::get);
    fmt::print("Some::get '{}'\n", val);

    val = shared_some.write_value(&Some::modify, 13);
    fmt::print("Some::modify '{}'\n", val);

    // unique_ptr with custom deleter
    // ------------------------------
    auto uniq_some_custom_ptr    = SyncUniqueCustom<Some, void (*)(Some*)>{ create(4), destroy };
    auto uniq_some_custom_lambda = SyncUniqueCustom<Some, decltype([](Some* s) { destroy(s); })>{ create(4) };

    val = uniq_some_custom_ptr.read_value(&Some::get);
    fmt::print("Some::get '{}'\n", val);

    val = uniq_some_custom_ptr.write_value(&Some::modify, 13);
    fmt::print("Some::modify '{}'\n", val);

    // shared_ptr with custom deleter
    // ------------------------------
    auto shared_some_custom_ptr    = SyncShared<Some>{ create(46), destroy };
    auto shared_some_custom_lambda = SyncShared<Some>{ create(46), [](Some* s) { destroy(s); } };

    val = shared_some_custom_ptr.read_value(&Some::get);
    fmt::print("Some::get '{}'\n", val);

    val = shared_some_custom_ptr.write_value(&Some::modify, 13);
    fmt::print("Some::modify '{}'\n", val);

    // other
    // -----
    val = uniq_some.read_value([](const Some& some) {
        auto some_value = some.get();
        return some_value + 42;
    });

    val = uniq_some.write_value([](Some& some) {
        auto some_value = some.modify(12);
        return some_value + 42;
    });

    auto ptr [[maybe_unused]] = uniq_some.read([](auto& up) { return up.get(); });

    auto b [[maybe_unused]] = static_cast<bool>(uniq_some);
    b                       = !uniq_some;

    if (uniq_some) { }
    if (!uniq_some) { }

    uniq_some.reset(new Some{ 312, "dfjh" });
    uniq_some = std::make_unique<Some>(213, "reuu");

    auto unique_some_3 = decltype(uniq_some){ new Some{ 312, "sdjfhdskfjh" } };
    // auto unique_some_4 = std::move(unique_some_3);    // FAIL: call to deleted move constructor
    // unique_some        = std::move(unique_some_3);    // FAIL: call to deleted move assignment

    using boost::ut::reflection::type_name;

    // clang-format off
    fmt::print("\n");
    using Mutex = decltype(some)::Mutex;
    fmt::print("{:>2} = sizeof '{}'\n", sizeof(Mutex), type_name<Mutex>());

    fmt::print("{:>2} = sizeof '{}'\n", sizeof(Some), type_name<Some>());
    fmt::print("{:>2} = sizeof '{}'\n", sizeof(some), type_name<decltype(some)>());

    using UniquePtr = decltype(uniq_some)::Value;
    fmt::print("{:>2} = sizeof '{}'\n", sizeof(UniquePtr), type_name<UniquePtr>());
    fmt::print("{:>2} = sizeof '{}'\n", sizeof(uniq_some), type_name<decltype(uniq_some)>());

    using UniquePtrCustom = decltype(uniq_some_custom_ptr)::Value;
    fmt::print("{:>2} = sizeof '{}'\n", sizeof(UniquePtrCustom), type_name<UniquePtrCustom>());
    fmt::print("{:>2} = sizeof '{}'\n", sizeof(uniq_some_custom_ptr), type_name<decltype(uniq_some_custom_ptr)>()
    );

    using SharedPtr = decltype(shared_some)::Value;
    fmt::print("{:>2} = sizeof '{}'\n", sizeof(SharedPtr), type_name<SharedPtr>());
    fmt::print("{:>2} = sizeof '{}'\n", sizeof(shared_some), type_name<decltype(shared_some)>());
    fmt::print("{:>2} = sizeof '{}'\n", sizeof(shared_some_custom_ptr), type_name<decltype(shared_some_custom_ptr)>());
    fmt::print("\n");
    // clang-format on

    // using deduction guides
    // ----------------------
    auto sync_unique_guide      = SyncSmartPtr{ std::make_unique<Some>(1, "one") };
    auto sync_uniq_custom_guide = SyncSmartPtr{
        std::unique_ptr<Some, void (*)(Some*)>{ new Some{ 2, "two" }, destroy },
    };

    auto sync_shared = SyncSmartPtr{ std::make_shared<Some>(3, "three") };

    // ctad for alias (works on gcc 14 not on clang 19)
    // ------------------------------------------------
    // auto sync_uniq_guide_2 = SyncUnique{ std::make_unique<Some>(1, "one") };
    // auto sync_shared_2     = SyncShared{ std::make_shared<Some>(3, "three") };

    auto id [[maybe_unused]] = sync_unique_guide.get_value(&Some::m_id);

    sync_unique_guide.reset(nullptr);

    try {
        auto ve [[maybe_unused]] = sync_unique_guide.write_value(&Some::modify, 12);
        assert("this code path should not be reachable since above expression must always throw");
    } catch (std::exception& e) {
        std::cerr << "Exception catched: " << e.what() << '\n';
    }
}
