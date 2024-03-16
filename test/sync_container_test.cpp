#include "print.hpp"

#include <sync_cpp/sync_container.hpp>

#include <boost/ut.hpp>

#include <optional>

class A
{
public:
    int m_value{ 0 };
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

    using Container   = std::optional<A>;
    using Getter      = decltype([](std::optional<A>& opt) -> A& { return opt.value(); });
    using GetterConst = decltype([](const std::optional<A>& opt) -> const A& { return opt.value(); });
    using SyncOptA    = SyncContainer<std::optional<A>, A, Getter, GetterConst, std::mutex>;

    "Access"_test = [&] {
        SyncOptA syncA{ std::nullopt };
        syncA.write([](auto& o) { o = A{ 42 }; });

        bool hasValue = syncA.read([](const auto& o) { return o.has_value(); });
        ut::expect(hasValue == true);

        syncA.write([](auto& o) { o = A{ 2'387'324 }; });
        auto value = syncA.read([](const auto& o) { return o.value().m_value; });
        ut::expect(value == 2'387'324_i);

        auto fmtA = [](const A& a) { return fmt("A = {}", a.m_value); };
        auto strA = syncA.readValue(fmtA);
        ut::expect(strA == "A = 2387324");

        // syncA.write(&std::optional<A>::reset);    // won't work, i haven't considered noexcept
        syncA.write([](auto& o) { o.reset(); });    // for now for noexcept functions, use this
    };

    print("{:>2} = {} \n", sizeof(Container), type_name<Container>());
    print("{:>2} = {} \n", sizeof(Getter), type_name<Getter>());
    print("{:>2} = {} \n", sizeof(GetterConst), type_name<GetterConst>());
    print("{:>2} = {} \n", sizeof(SyncOptA), type_name<SyncOptA>());
    print("{:>2} = {} \n", sizeof(SyncOptA::SyncBase), type_name<SyncOptA::SyncBase>());
    print("{:>2} = {} \n", sizeof(SyncOptA::Value_type), type_name<SyncOptA::Value_type>());
    print("{:>2} = {} \n", sizeof(SyncOptA::Mutex_type), type_name<SyncOptA::Mutex_type>());
}
