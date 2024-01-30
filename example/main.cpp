#include "print.hpp"

#include "sync.hpp"

#include <iostream>
#include <string>
#include <thread>
#include <vector>

class Foo
{
private:
    inline static int s_idCounter{ 0 };

public:
    Foo(std::string name)
        : m_id{ s_idCounter++ }
        , m_name{ std::move(name) }
        , m_data{ 1, 2, 3, 4, 5 }
    {
    }

    void print() const
    {
        std::string data;
        for (const auto& i : m_data) {
            data += std::to_string(i) + ", ";
        }
        ::print("Foo = {{\n\tid: {},\n\tname: {},\n\tdata: [{}]\n}}\n", m_id, m_name, data);
    }

    std::size_t size() const { return m_data.size(); }

    std::size_t add(int i)
    {
        m_data.push_back(i);
        return m_data.size();
    }

    std::size_t erase(int i)
    {
        auto n = size();
        m_data.erase(m_data.begin() + i, m_data.end());
        return n - size();
    }

    const auto& getData() const { return m_data; }
    const auto& getName() const { return m_name; }

    const int m_id;

private:
    std::string      m_name;
    std::vector<int> m_data;
};

int main()
{
    using namespace std::chrono_literals;

    spp::Sync<Foo> foo{ "Example" };

    std::jthread t1([&foo]() {
        for (std::size_t i = 0; i < 10; ++i) {
            auto n = foo.write(&Foo::add, static_cast<int>(i));
            print("Thread 1: Added {}, data size is now {}\n", i, n);
            std::this_thread::sleep_for(100ms);
        }
    });

    std::jthread t2([&foo]() {
        for (std::size_t i = 0; i < 10; ++i) {
            auto name = foo.read([](const Foo& f) { return f.getName(); });
            print("Thread 2: Foo name is '{}'\n", name);
            std::this_thread::sleep_for(100ms);
        }
    });

    std::jthread t3([&foo]() {
        for (std::size_t i = 0; i < 5; ++i) {
            foo.write([](Foo& f) {
                std::this_thread::sleep_for(50ms);
                auto n = f.size();
                auto r = n > 3 ? f.erase(1) : 0;
                print("Thread 3: Erased {} elements, data size is now {}\n", r, n - r);
                print("Thread 3: ");
                f.print();
            });
            std::this_thread::sleep_for(150ms);
        }
    });

    std::jthread t4([&foo] {
        for (std::size_t i = 0; i < 10; ++i) {
            // auto data = foo.read(&Foo::getData);    // won't compile, member function returns a reference
            auto data = foo.read([](const Foo& f) { return f.getData(); });    // OK: gets copy
            print("Thread 4: {} -> ", foo.get(&Foo::m_id));                    // OK: copied
            for (const auto& i : data) {
                print("{} ", i);
            }
            print("\n");
            std::this_thread::sleep_for(100ms);
        }
    });
}