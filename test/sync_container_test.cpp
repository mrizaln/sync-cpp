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
    using Container   = std::optional<A>;
    using Getter      = decltype([](std::optional<A>& opt) -> A& { return opt.value(); });
    using GetterConst = decltype([](const std::optional<A>& opt) -> const A& { return opt.value(); });
    using SyncOptA    = SyncContainer<std::optional<A>, A, Getter, GetterConst, std::mutex>;

    SyncOptA syncA{ std::nullopt };

    syncA.write([](auto& o) { o = A{ 42 }; });

    syncA.read([](const auto& o) { print("{}\n", o.has_value()); });
    syncA.readValue(&printA);

    syncA.write([](auto& o) { o = A{ 2'387'324 }; });

    syncA.read([](const auto& o) { print("{}\n", o.has_value()); });
    syncA.readValue(&printA);

    print("{}\n", syncA.getValue(&A::m_value));

    using boost::ut::reflection::type_name;
    print("{:>2} = {} \n", sizeof(Container), type_name<Container>());
    print("{:>2} = {} \n", sizeof(Getter), type_name<Getter>());
    print("{:>2} = {} \n", sizeof(GetterConst), type_name<GetterConst>());
    print("{:>2} = {} \n", sizeof(SyncOptA), type_name<SyncOptA>());
    print("{:>2} = {} \n", sizeof(SyncOptA::SyncBase), type_name<SyncOptA::SyncBase>());
    print("{:>2} = {} \n", sizeof(SyncOptA::Value_type), type_name<SyncOptA::Value_type>());
    print("{:>2} = {} \n", sizeof(SyncOptA::Mutex_type), type_name<SyncOptA::Mutex_type>());
}
