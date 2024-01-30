#include "sync_smart_ptr.hpp"
#include "print.hpp"

#include <boost/ut.hpp>

#include <exception>
#include <format>
#include <memory>

class Some
{
public:
    int m_id;

    Some(int v, std::string name)
        : m_id{ ++m_idCounter }
        , m_value{ v }
        , m_name{ std::move(name) }
    {
        print("Some created: {} {}\n", m_value, m_name);
    }
    Some(const Some&)            = delete;
    Some(Some&&)                 = delete;
    Some& operator=(const Some&) = delete;
    Some& operator=(Some&&)      = delete;

    ~Some() { print("Some destroyed: {} {}\n", m_value, m_name); }

    int get() const { return m_value; }
    int modify(int v) { return m_value += v; }

private:
    inline static int m_idCounter{ 0 };

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
    int val;

    //
    // plain type
    Sync<Some> some{ 42, "stack" };    // OK

    val = some.read(&Some::get);
    print("Some::get '{}'\n", val);

    val = some.write(&Some::modify, 13);
    print("Some::modify '{}'\n", val);

    //
    // unique_ptr
    SyncUnique<Some> uniqSome{ std::make_unique<Some>(43, "uniq") };    // OK
    // SyncUnique<Some> uniqSome{ new Some{ 43, "uniq" } };             // OK: calling new manually

    val = uniqSome.readValue(&Some::get);
    print("Some::get '{}'\n", val);

    val = uniqSome.writeValue(&Some::modify, 13);
    print("Some::modify '{}'\n", val);

    //
    // shared_ptr
    SyncShared<Some> sharedSome{ std::make_shared<Some>(44, "shared") };    // OK
    // SyncShared<Some> sharedSome{ new Some{ 44, "shared"} };              // OK: calling new manually

    val = sharedSome.readValue(&Some::get);
    print("Some::get '{}'\n", val);

    val = sharedSome.writeValue(&Some::modify, 13);
    print("Some::modify '{}'\n", val);

    //
    // unique_ptr with custom deleter
    SyncUniqueCustom<Some, void (*)(Some*)> uniqSomeCustom{ create(45), destroy };    // OK, function pointer
    // SyncUniqueCustom<Some, decltype([](Some* s) { destroy(s); })> uniqSomeCustom{ create(45) };    // OK, lambda

    val = uniqSomeCustom.readValue(&Some::get);
    print("Some::get '{}'\n", val);

    val = uniqSomeCustom.writeValue(&Some::modify, 13);
    print("Some::modify '{}'\n", val);

    //
    // shared_ptr with custom deleter
    SyncShared<Some> sharedSomeCustom{ create(46), destroy };    // OK, function pointer
    // LockedShared<Some> sharedSomeCustom{ create(46), [](Some* s) { destroy(s); } };      // OK, lambda

    val = sharedSomeCustom.readValue(&Some::get);
    print("Some::get '{}'\n", val);

    val = sharedSomeCustom.writeValue(&Some::modify, 13);
    print("Some::modify '{}'\n", val);

    //
    // other
    val = uniqSome.readValue([](const Some& some) {
        auto someValue = some.get();
        return someValue + 42;
    });

    val = uniqSome.writeValue([](Some& some) {
        auto someValue = some.modify(12);
        return someValue + 42;
    });

    auto ptr = uniqSome.read([](auto& up) { return up.get(); });

    auto b = static_cast<bool>(uniqSome);
    b      = !uniqSome;

    if (uniqSome) { }
    if (!uniqSome) { }

    uniqSome.reset(new Some{ 312, "dfjh" });
    uniqSome = std::make_unique<Some>(213, "reuu");

    decltype(uniqSome) uniqSome2{ new Some{ 312, "sdjfhdskfjh" } };
    // decltype(uniqSome) uniqSome3{ std::move(uniqSome2) };    // FAIL: call to deleted move constructor
    // uniqSome = std::move(uniqSome3);                         // FAIL: call to deleted move assignment

    using boost::ut::reflection::type_name;

    print("\n");
    using Mutex = decltype(some)::Mutex_type;
    print("{:>2} = sizeof '{}'\n", sizeof(Mutex), type_name<Mutex>());

    print("{:>2} = sizeof '{}'\n", sizeof(Some), type_name<Some>());
    print("{:>2} = sizeof '{}'\n", sizeof(some), type_name<decltype(some)>());

    using UniquePtr = decltype(uniqSome)::Value_type;
    print("{:>2} = sizeof '{}'\n", sizeof(UniquePtr), type_name<UniquePtr>());
    print("{:>2} = sizeof '{}'\n", sizeof(uniqSome), type_name<decltype(uniqSome)>());

    using UniquePtrCustom = decltype(uniqSomeCustom)::Value_type;
    print("{:>2} = sizeof '{}'\n", sizeof(UniquePtrCustom), type_name<UniquePtrCustom>());
    print("{:>2} = sizeof '{}'\n", sizeof(uniqSomeCustom), type_name<decltype(uniqSomeCustom)>());

    using SharedPtr = decltype(sharedSome)::Value_type;
    print("{:>2} = sizeof '{}'\n", sizeof(SharedPtr), type_name<SharedPtr>());
    print("{:>2} = sizeof '{}'\n", sizeof(sharedSome), type_name<decltype(sharedSome)>());
    print("{:>2} = sizeof '{}'\n", sizeof(sharedSomeCustom), type_name<decltype(sharedSomeCustom)>());
    print("\n");

    // using deduction guides
    SyncSmartPtr syncedUniqGuide{ std::make_unique<Some>(1, "one") };
    SyncSmartPtr syncedUniqCustomGuide{ std::unique_ptr<Some, void (*)(Some*)>{ new Some{ 2, "two" }, destroy } };
    SyncSmartPtr syncedShared{ std::make_shared<Some>(3, "three") };

    // ctad for alias (works on gcc 13 not on clang 17)
    // SyncUnique syncedUniqGuide2{ std::make_unique<Some>(1, "one") };
    // SyncShared syncedShared2{ std::make_shared<Some>(3, "three") };

    auto id = syncedUniqGuide.getValue(&Some::m_id);

    syncedUniqGuide.reset(nullptr);
    // auto va = syncedUniqGuide.readValue(&Some::get);
    try {
        auto ve = syncedUniqGuide.writeValue(&Some::modify, 12);
    } catch (std::exception& e) {
        std::cerr << "Exception catched: " << e.what() << '\n';
    }
}
