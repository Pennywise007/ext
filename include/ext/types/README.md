# Useful types

## Constexpr map with compile time search

Example:
```c++
constexpr const_map kMapValues = { std::pair{1, "one"}, std::pair{2, "two"}};

static_assert(!kMapValues.contain_duplicate_keys());
static_assert(kMapValues.contains_key(1));
static_assert(kMapValues.get_value(1) == "one");
```

- [Source](https://github.com/Pennywise007/ext/blob/main/include/ext/types/const_map.h)
- [Tests](https://github.com/Pennywise007/ext/blob/main/test/types/const_map_test.cpp)

## Lazy type

Allows to define an object with an object getter function. Allows to simplify resources management.
The object will be created only of first object call and get function will be freed.

Example:
```c++
struct Test
{
	Test(bool,int) {}
	void Get() {}
};
ext::lazy_type<Test> lazyObject([]() { return Test(100, 500); });
// Object not created yet

// Object will be created by provided GetObjectFunction during value() call
lazyObject.value().Get();
```

- [Source](https://github.com/Pennywise007/ext/blob/main/include/ext/types/lazy.h)

## UUID

- [Source](https://github.com/Pennywise007/ext/blob/main/include/ext/types/uuid.h)
