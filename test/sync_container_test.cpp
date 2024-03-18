#include "print.hpp"

#include <sync_cpp/sync_container.hpp>

#include <boost/ut.hpp>

#include <optional>

struct A
{
    int m_value{ 0 };
    int answer() noexcept { return 42; }
};

template <typename... Args>
std::string fmt(std::format_string<Args...>&& fmt, Args&&... args)
{
    return std::format(std::forward<std::format_string<Args...>>(fmt), std::forward<Args>(args)...);
};

using spp::SyncContainer;

int main()
{
    namespace ut = boost::ut;
    using namespace ut::literals;
    using namespace ut::operators;
    using ut::reflection::type_name;

    using Container   = std::optional<A>;
    using Getter      = decltype([](std::optional<A>& opt) -> A& { return opt.value(); });
    using GetterConst = decltype([](const std::optional<A>& opt) -> const A& { return opt.value(); });

    ut::suite basicOperations [[maybe_unused]] = [] {
        using SyncOptA = SyncContainer<std::optional<A>, A, Getter, GetterConst, std::mutex>;

        "Internal mutex"_test = [&] {
            "Default ctor"_test = [&] {
                SyncOptA syncA;
                ut::expect(syncA.read(&Container::has_value) == false);
            };

            "Value ctor"_test = [&] {
                SyncOptA syncA{ std::nullopt };
                ut::expect(syncA.read(&Container::has_value) == false);

                SyncOptA syncA2{ 42 };
                ut::expect(syncA2.read(&Container::has_value) == true);
                ut::expect(syncA2.getValue(&A::m_value) == 42_i);
            };

            "Operations"_test = [&] {
                SyncOptA syncA3{ std::nullopt };
                syncA3.write([](auto& o) { o = A{ 42 }; });

                bool hasValue = syncA3.read([](const auto& o) { return o.has_value(); });
                ut::expect(hasValue == true);

                syncA3.write([](auto& o) { o = A{ 2'387'324 }; });
                auto value = syncA3.read([](const auto& o) { return o.value().m_value; });
                ut::expect(value == 2'387'324_i);

                auto fmtA = [](const A& a) { return fmt("A = {}", a.m_value); };
                auto strA = syncA3.readValue(fmtA);
                ut::expect(strA == "A = 2387324");

                syncA3.write(&std::optional<A>::reset);
            };
        };

        "External mutex"_test = [&] {
            using SyncOptAExt = SyncContainer<Container, A, Getter, GetterConst, std::mutex, false>;
            std::mutex mutex;

            "Default ctor"_test = [&] {
                SyncOptAExt syncA{ mutex };
                ut::expect(syncA.read(&Container::has_value) == false);
            };

            "Value ctor"_test = [&] {
                SyncOptAExt syncA{ mutex, std::nullopt };
                ut::expect(syncA.read(&Container::has_value) == false);

                SyncOptAExt syncA2{ mutex, 42 };
                ut::expect(syncA2.read(&Container::has_value) == true);
                ut::expect(syncA2.getValue(&A::m_value) == 42_i);
            };

            "Operations"_test = [&] {
                SyncOptAExt syncA{ mutex, 42 };

                syncA.write([](auto& o) { o = A{ 42 }; });

                bool hasValue = syncA.read([](const auto& o) { return o.has_value(); });
                ut::expect(hasValue == true);

                syncA.write([](auto& o) { o = A{ 2'387'324 }; });
                auto value = syncA.read([](const auto& o) { return o.value().m_value; });
                ut::expect(value == 2'387'324_i);

                auto fmtA = [](const A& a) { return fmt("A = {}", a.m_value); };
                auto strA = syncA.readValue(fmtA);
                ut::expect(strA == "A = 2387324");

                syncA.write(&std::optional<A>::reset);
            };
        };
    };

    "Size constraints"_test = [&] {
        using StatelessGetter      = Getter;
        using StatelessGetterConst = GetterConst;
        using Mutex                = std::mutex;

        using StatelessSyncOptA = SyncContainer<Container, A, StatelessGetter, StatelessGetterConst, Mutex>;

        constexpr auto containerSize   = sizeof(Container);
        constexpr auto mutexSize       = sizeof(Mutex);
        constexpr auto getterSize      = sizeof(StatelessGetter);
        constexpr auto getterConstSize = sizeof(StatelessGetterConst);
        constexpr auto syncBaseSize    = containerSize + mutexSize;

        ut::expect(sizeof(StatelessSyncOptA) < syncBaseSize + getterSize + getterConstSize);
        ut::expect(sizeof(StatelessSyncOptA) == syncBaseSize);
    };
}
