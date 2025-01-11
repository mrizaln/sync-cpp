#include <sync_cpp/sync_opt.hpp>

#include <boost/ut.hpp>
#include <fmt/core.h>

#include <string>

class Some
{
public:
    int m_id;

    Some(int v, std::string name)
        : m_id{ ++m_idCounter }
        , m_value{ v }
        , m_name{ std::move(name) }
    {
        fmt::print("Some created: {} {}\n", m_value, m_name);
    }

    Some(const Some&)            = delete;
    Some& operator=(const Some&) = delete;
    Some(Some&&)                 = delete;
    Some& operator=(Some&&)      = delete;
    // Some(Some&&)                 = default;
    // Some& operator=(Some&&)      = default;

    ~Some() { fmt::print("Some destroyed: {} {}\n", m_value, m_name); }

    int get() const { return m_value; }
    int modify(int v) { return m_value += v; }

private:
    inline static int m_idCounter{ 0 };

    int         m_value;
    std::string m_name;
};

int main()
{
    using Opt = spp::SyncOpt<Some>;
    Opt::Value opt{};
    Opt        opt1{ 10, "hello" };
    Opt        opt2{ std::nullopt };
    // Opt             opt3{ Some(10, "hello") };

    std::mutex mutex;
    using EOpt = spp::SyncOpt<Some, std::mutex, true, false>;
    EOpt opt5{ mutex, 10, "hello" };
    EOpt opt6{ mutex, std::nullopt };
    // EOpt opt7{ mutex, Some(10, "hello") };       // use forwarding instead

    namespace ut = boost::ut;
    using namespace ut::literals;
    "throws"_test = [] {
        spp::SyncOpt<Some> opt{ std::nullopt };
        ut::expect(ut::throws([&] { std::ignore = opt.read_value(&Some::get); }));
    };
}
