# sync-cpp

Synchronized object wrapper for C++20

## TODO

- [ ] Allow stateful lambda for `SyncContainer` `Getter`and`GetterConst` (but should I though?)
- [ ] Rework `Sync::read` and `Sync::write` that has member function arguments with some kind of function traits ([see](https://breese.github.io/2022/03/06/deducing-function-signatures.html)) to reduce repetition

## Dependencies

- C++20

## Usage

### Setting up

Clone this repository (or as submodule) into your project somewhere. Then you can just `add_subdirectory` to this repo, then link your target against `sync_cpp`.

```cmake
#...

# for example, we clone this repository into {PROJECT_ROOT}/lib/ directory
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

## Limitation

Member functions of `Sync` that receives member function pointer can't disambiguate an overloaded function. A work around is to use a lambda.

```cpp
class Foo {
    int something() &;
    int something() &&;
};

spp::Sync<Foo> foo;
// int value = foo.write(&Foo::something);                      // won't compile
int value = foo.write([](auto& f) { return f.something(); });   // compiles
```

We might want to return a reference to a value that may have longer lifetime or have static storage duration from an instance of a class. We can't directly do that because the constraint on the `Sync::read` and `Sync::write` functions prohibits returning a reference. However, we still can bypass it by doing this:

> This pattern is heavily discouraged, you are basically trying to access a resource that might not be synchronized by doing this.

```cpp
class Foo {
    Something& getGlobalSomething();
};

spp::Sync<Foo> foo;

// ...

Something* sPtr;
foo.read([&sPtr](const Foo& f) { sPtr = &f.getGlobalSomething(); });
```

Unfortunately, currently we can pass a `nullptr` as member function pointer to the read and write function. For now, I'll ignore this case, and assume that every member function pointer passed to the functions are not `nullptr`. I don't know what to do when it's `nullptr` (`throw`? `assert`? do nothing?)

```cpp
class Foo {
    int bar(int v);
};

spp::Sync<Foo> foo;

// ...

using F = decltype(&Foo::bar);
F f     = nullptr;
int v   = foo.write(f, 1);
```

## Customization

The class [`SyncContainer`](./include/sync_cpp/sync_container.hpp) is an adapter(?) class that flattens the accessor to the value inside Sync. You can extend from this class to work with other container (or your custom type) so it will be easier to work with. For the example of implementation, see [SyncSmartPtr](./include/sync_cpp/sync_smart_ptr.hpp) and [SyncOpt](./include/sync_cpp/sync_opt.hpp).
