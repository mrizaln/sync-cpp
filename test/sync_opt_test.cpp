#include <sync_cpp/sync_opt.hpp>

#include <boost/ut.hpp>

namespace ut = boost::ut;

class Some
{
public:
    int m_id;

    Some(int v, std::string name)
        : m_id{ ++m_id_counter }
        , m_value{ v }
        , m_name{ std::move(name) }
    {
        ut::log("Some created: {} {}\n", m_value, m_name);
    }

    Some(const Some&)            = delete;
    Some& operator=(const Some&) = delete;
    Some(Some&&)                 = delete;
    Some& operator=(Some&&)      = delete;
    // Some(Some&&)                 = default;
    // Some& operator=(Some&&)      = default;

    ~Some() { ut::log("Some destroyed: {} {}\n", m_value, m_name); }

    int get() const { return m_value; }
    int modify(int v) { return m_value += v; }

private:
    inline static int m_id_counter{ 0 };

    int         m_value;
    std::string m_name;
};

int main()
{
    using Opt = spp::SyncOpt<Some>;

    auto opt   = Opt::Value{};
    auto opt_1 = Opt{ 10, "hello" };
    auto opt_2 = Opt{ std::nullopt };
    // auto opt_3 = Opt{ Some(10, "hello") };       // FAIL: Some is not movable

    auto mutex = std::mutex{};
    using EOpt = spp::SyncOpt<Some, std::mutex, true, false>;

    auto opt_4 = EOpt{ mutex, 10, "hello" };
    auto opt_5 = EOpt{ mutex, std::nullopt };
    // auto opt_6 = EOpt{ mutex, Some(10, "hello") };    // FAIL: Some is not movable

    ut::test("throws") = [] {
        auto opt = spp::SyncOpt<Some>{ std::nullopt };
        ut::expect(ut::throws([&] { std::ignore = opt.read_value(&Some::get); }));
    };
}
