#include <sync_cpp/sync_container.hpp>

#include <boost/ut.hpp>

#include <format>
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

template <typename... Fs>
struct Overload : Fs...
{
    using Fs::operator()...;
};

int main()
{
    namespace ut = boost::ut;
    using namespace ut::literals;
    using namespace ut::operators;
    using ut::reflection::type_name;

    using Container = std::optional<A>;
    using Getter    = decltype([](auto&& opt) -> decltype(auto) { return opt.value(); });

    ut::suite basic_operations [[maybe_unused]] = [] {
        using SyncOptA = SyncContainer<std::optional<A>, A, Getter, std::mutex>;

        "Internal mutex"_test = [&] {
            "Default ctor"_test = [&] {
                auto sync_a = SyncOptA{};
                ut::expect(sync_a.read(&Container::has_value) == false);
            };

            "Value ctor"_test = [&] {
                auto sync_a = SyncOptA{ std::nullopt };
                ut::expect(sync_a.read(&Container::has_value) == false);

                auto sync_a_2 = SyncOptA{ 42 };
                ut::expect(sync_a_2.read(&Container::has_value) == true);
                ut::expect(sync_a_2.get_value(&A::m_value) == 42_i);
            };

            "Operations"_test = [&] {
                auto sync_a_3 = SyncOptA{ std::nullopt };
                sync_a_3.write([](auto& o) { o = A{ 42 }; });

                auto has_value = sync_a_3.read([](const auto& o) { return o.has_value(); });
                ut::expect(has_value == true);

                sync_a_3.write([](auto& o) { o = A{ 2'387'324 }; });
                auto value = sync_a_3.read([](const auto& o) { return o.value().m_value; });
                ut::expect(value == 2'387'324_i);

                auto fmt_a = [](const A& a) { return fmt("A = {}", a.m_value); };
                auto str_a = sync_a_3.read_value(fmt_a);
                ut::expect(str_a == "A = 2387324");

                sync_a_3.write(&std::optional<A>::reset);
            };
        };

        "External mutex"_test = [&] {
            using SyncOptAExt = SyncContainer<Container, A, Getter, std::mutex, false>;
            auto mutex        = std::mutex{};

            "Default ctor"_test = [&] {
                auto sync_a = SyncOptAExt{ mutex };
                ut::expect(sync_a.read(&Container::has_value) == false);
            };

            "Value ctor"_test = [&] {
                auto sync_a = SyncOptAExt{ mutex, std::nullopt };
                ut::expect(sync_a.read(&Container::has_value) == false);

                auto sync_a_2 = SyncOptAExt{ mutex, 42 };
                ut::expect(sync_a_2.read(&Container::has_value) == true);
                ut::expect(sync_a_2.get_value(&A::m_value) == 42_i);
            };

            "Operations"_test = [&] {
                auto sync_a = SyncOptAExt{ mutex, 42 };

                sync_a.write([](auto& o) { o = A{ 42 }; });

                auto has_value = sync_a.read([](const auto& o) { return o.has_value(); });
                ut::expect(has_value == true);

                sync_a.write([](auto& o) { o = A{ 2'387'324 }; });
                auto value = sync_a.read([](const auto& o) { return o.value().m_value; });
                ut::expect(value == 2'387'324_i);

                auto fmt_a = [](const A& a) { return fmt("A = {}", a.m_value); };
                auto str_a = sync_a.read_value(fmt_a);
                ut::expect(str_a == "A = 2387324");

                sync_a.write(&std::optional<A>::reset);
            };
        };
    };

    "Size constraints"_test = [&] {
        using StatelessGetter = Getter;
        using Mutex           = std::mutex;

        using StatelessSyncOptA = SyncContainer<Container, A, StatelessGetter, Mutex>;

        constexpr auto container_size = sizeof(Container);
        constexpr auto mutex_size     = sizeof(Mutex);
        constexpr auto getter_size    = sizeof(StatelessGetter);
        constexpr auto sync_base_size = container_size + mutex_size;

        ut::expect(sizeof(StatelessSyncOptA) < sync_base_size + getter_size);
        ut::expect(sizeof(StatelessSyncOptA) == sync_base_size);
    };
}
