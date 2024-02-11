# sync-cpp

Synchronized object wrapper for C++20

## Dependencies

- C++20

## Usage

### Setting up

Clone this repository (or as submodule) into your project somewhere. Then you can just `add_subdirectory` to this repo, then link your target against `sync_cpp`.

```cmake
#...

# for example, I cloned this repository into {PROJECT_ROOT}/lib directory
add_subdirectory(./lib/sync-cpp)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE sync_cpp)

#...
```

Setting up complete, now you can use the library.

### Example

This is an example usage of this library.

```cpp
#include <sync_cpp/sync.hpp>             // for Sync<T, M>;
#include <sync_cpp/sync_container.hpp>   // for container adapter
#include <sync_cpp/sync_smart_ptr.hpp>   // SyncUnique, SyncUniqueCustom, SyncShared: wrapper for Sync<std::unique_ptr, M> (also shared_ptr)
#include <sync_cpp/sync_opt.hpp>         // same as above, but for optional

#include <iostream>

class Foo
{
public:
    int bar(double d) const { return d / 2; };
};

int main() {
    using namespace std::string_literals;

    spp::Sync string{ "sdafjh"s };                                        // deduction guide -> spp::Sync<std::string, std::mutex>
    auto substr = string.write([](auto& s) {                              // mutate the value inside Sync
        s = "hello world!";
        return s.substr(6, 5);
    });
    std::cout << substr << '\n';


    spp::Sync<Foo, std::shared_mutex> syncA;                              // use std::shared_mutex for multiple reader single writer
    int v = syncA.read(&Foo::bar, 403.9);                                 // calling (const) member function


    spp::SyncUnique<Foo> syncUnique{ new Foo{} };                         // Sync<std::unique_ptr<T>, M> but with more convenient API
    bool notNull = syncUnique.read([](const std::unique_ptr<Foo>& sp){    // access (read) the unique_ptr
        return sp != nullptr;
    });

    if (syncUnique) {                                                     // bool conversion just like std::unique_ptr
        int n = syncUnique.readValue([](const Foo& a) {                   // read the value contained within unique_ptr
            int v = a.bar(42.0);
            return v * 12;
        });
    }
}
```

> See also an example project [here](./example)

### Next step

The class [`SyncContainer`](./include/sync_container.hpp) is an adapter(?) that flattens the accessor to the value inside Sync. You can extend from this class to work with other container so it will be easier to work with. For the example of implementation, see [SyncSmartPtr](./include/sync_smart_ptr.hpp) and [SyncOpt](./include/sync_opt.hpp).
