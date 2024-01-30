#include "print.hpp"

#include <sync_cpp/sync_container.hpp>

#include <boost/ut.hpp>

#include <optional>

void printInt(const int& i)
{
    print("{}\n", i);
}

class A
{
public:
    int m_value{ 0 };
};

void printA(const A& a)
{
    print("{}\n", a.m_value);
}

using namespace spp;

int main()
{
    // SyncContainer sync{ std::optional<int>{ 1 }, [](auto& opt) { return opt.value(); } };
    using Container   = std::optional<int>;
    using Getter      = int (std::optional<int>::*)();
    using GetterConst = decltype([](const std::optional<int>& opt) -> const int& { return opt.value(); });
    using SyncOptInt  = SyncContainer<Container, int, Getter, GetterConst, std::mutex>;

    SyncOptInt sync{ std::nullopt };

    sync.write([](auto& o) { o = 42; });

    sync.read([](const auto& o) { print("{}\n", o.has_value()); });
    sync.readValue(&printInt);

    sync.write([](auto& o) { o = (1 + 1209); });

    sync.read([](const auto& o) { print("{}\n", o.has_value()); });
    sync.readValue(&printInt);

    using boost::ut::reflection::type_name;
    print("{:>2} = {} \n", sizeof(Container), type_name<Container>());
    print("{:>2} = {} \n", sizeof(Getter), type_name<Getter>());
    print("{:>2} = {} \n", sizeof(GetterConst), type_name<GetterConst>());
    print("{:>2} = {} \n", sizeof(SyncOptInt), type_name<SyncOptInt>());
    print("{:>2} = {} \n", sizeof(SyncOptInt::SyncBase), type_name<SyncOptInt::SyncBase>());
    print("{:>2} = {} \n", sizeof(SyncOptInt::Value_type), type_name<SyncOptInt::Value_type>());
    print("{:>2} = {} \n", sizeof(SyncOptInt::Mutex_type), type_name<SyncOptInt::Mutex_type>());

    using SyncOptA = SyncContainer<
        std::optional<A>,
        A,
        decltype([](std::optional<A>& opt) -> A& { return opt.value(); }),
        decltype([](const std::optional<A>& opt) -> const A& { return opt.value(); }),
        std::mutex>;

    SyncOptA syncA{ std::nullopt };

    syncA.write([](auto& o) { o = A{ 42 }; });

    syncA.read([](const auto& o) { print("{}\n", o.has_value()); });
    syncA.readValue(&printA);

    syncA.write([](auto& o) { o = A{ 2'387'324 }; });

    syncA.read([](const auto& o) { print("{}\n", o.has_value()); });
    syncA.readValue(&printA);

    print("{}\n", syncA.getValue(&A::m_value));
}
