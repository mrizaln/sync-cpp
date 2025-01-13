# sync-cpp

Synchronized object wrapper for C++20

## TODO

- [x] Documentation
- [ ] Allow stateful lambda for `SyncContainer` `Getter` (but should I though?)
- [ ] Rework `Sync::read` and `Sync::write` that has member function arguments with some kind of function traits ([see](https://breese.github.io/2022/03/06/deducing-function-signatures.html)) to reduce repetition

## Dependencies

- C++20
  > The compiler must have support for constraints and concepts both in language and library, coroutines is not necessary

## Usage

### Setting up

Clone this repository (or as submodule) into your project somewhere, even easier, use FetchContent. Then link your target against `sync-cpp`.

```cmake
# If you are using FetchContent
include(FetchContent)
FetchContent_Declare(
  sync-cpp
  GIT_REPOSITORY https://github.com/mrizaln/sync-cpp
  GIT_TAG main)

# # If you clone/submodule the repository
# add_subdirectory(path/to/the/cloned/repository)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE sync-cpp)
```

### Example

This is an example usage of this library.

```cpp
#include <sync_cpp/sync.hpp>                // for Sync<T, M>;
#include <sync_cpp/sync_smart_ptr.hpp>      // SyncUnique, SyncUniqueCustom, SyncShared: wrapper for Sync<std::unique_ptr, M> (also shared_ptr)
#include <sync_cpp/sync_opt.hpp>            // same as above, but for std::optional
#include <sync_cpp/group.hpp>               // allow grouped lock through spp::Group wrapper and spp::group factory function

// #include <sync_cpp/sync_container.hpp>   // Sync container adapter (for your own container, single valued like std::unique_ptr)

#include <iostream>

struct Foo
{
    int bar(double d) const { return static_cast<int>(d / 2); };
};

int main()
{
    using namespace std::string_literals;

    auto string = spp::Sync{ "sdafjh"s };                         // deduction guide -> spp::Sync<std::string, std::mutex>
    auto substr = string.write([](auto& str) {                    // mutate the value inside Sync
        str = "hello world!";
        return str.substr(6, 5);
    });

    std::cout << substr << '\n';

    auto sync_a = spp::Sync<Foo, std::shared_mutex>{};            // using std::shared_mutex
    auto value  = sync_a.read(&Foo::bar, 403.9);                  // calling (const) member function

    auto sync_unique = spp::SyncUnique<Foo>{ new Foo{} };         // Sync<std::unique_ptr<T>, M> but with more convenient API
    auto not_null    = sync_unique.read([](const auto& sp) {      // access (read) the unique_ptr
        return sp != nullptr;
    });

    if (sync_unique) {                                            // bool conversion just like std::unique_ptr
        auto n = sync_unique.read_value([](const Foo& a) {        // read the value contained within unique_ptr
            int v = a.bar(42.0);
            return v * 12;
        });
    }

    // grouped lock                         ---- or use read()/write() for const/non-const access to all
    //                                      ---- lock() access for each element is dependent on the constness of the underlying Sync
    spp::group(string, sync_a, sync_unique).lock([](auto&& s, auto&& a, auto&& b) {
        std::cout << "string: " << s << '\n'
                  << "Foo 1 : " << a.bar(3.14) << '\n'
                  << "Foo 2 : " << b->bar(1000.0) << '\n';
    });
}
```

> See also an example project [here](./example)

## Limitations

- Member functions of `Sync` that receives member function pointer can't disambiguate an overloaded function. A work around is to use a lambda.

```cpp
class Foo {
    int something() &;
    int something() &&;
};

spp::Sync<Foo> foo;
// int value = foo.write(&Foo::something);                      // won't compile
int value = foo.write([](auto& f) { return f.something(); });   // compiles
```

- We might want to return a reference to a value that may have longer lifetime or have static storage duration from an instance of a class. We can't directly do that because the constraint on the `Sync::read` and `Sync::write` functions prohibits returning a reference. However, we can return a pointer.

  > This pattern is heavily discouraged, you are basically trying to access a resource that might not be synchronized by doing this.

```cpp
struct Something;

struct Foo
{
    Something& get_global_something() const;
};

auto foo = spp::Sync<Foo>{};

// ...

auto* something = foo.read([](const Foo& f) { return &f.get_global_something(); });

// even this
auto* innerFoo = foo.write([](Foo& f) { return &f; });    // don't do this
```

- Unfortunately, currently we can pass a `nullptr` as member function pointer to the read and write function. For now, I'll ignore this case, and assume that every member function pointer passed to the functions are not `nullptr`. I don't know what to do when it's `nullptr` (`throw`? `assert`? do nothing?)

```cpp
struct Foo
{
    int bar(int v);
};

spp::Sync<Foo> foo;

// ...

using F = decltype(&Foo::bar);
F f     = nullptr;
int v   = foo.write(f, 1);    // passing a nullptr, very bad
```

## Customization

The class [`SyncContainer`](./include/sync_cpp/sync_container.hpp) is an adapter class that flattens the accessor (read and write) to the value inside Sync (read_value and writeValue). You can extend from this class to work with other container (or your custom type) so it will be easier to work with

> For examples, see how [SyncSmartPtr](./include/sync_cpp/sync_smart_ptr.hpp) and [SyncOpt](./include/sync_cpp/sync_opt.hpp) implemented.
