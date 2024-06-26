#pragma once

#include <type_traits>

namespace ext::reflection::details {

template <class T>
extern const T external;

template <size_t FieldsCount, typename Type>
constexpr auto get_object_fields_impl(Type&& obj)
{
#define WRAP_FIELDS(...)                \
    {                                   \
        auto&& [__VA_ARGS__] = obj;     \
        return std::tie(__VA_ARGS__);   \
    }

    /* Generated with script:
    macro_name_with_indent = "    WRAP_FIELDS"
    max_line_length = 80

    def generate_cpp_code(fields_count):
        for count in range(1, fields_count + 1):
            fields = ", ".join(f"f{i}" for i in range(0, count))
            code_line = f"{macro_name_with_indent}({fields})"
            wrapped_lines = wrap_line(code_line, max_line_length)

            print(f"if constexpr (FieldsCount == {count})")
            wrapped_lines = wrapped_lines
            for line in wrapped_lines:
                print(line)
            print(f"else ", end='')

    def wrap_line(line, max_length):
        words = line.split(", ")
        wrapped_lines = []
        current_line = words[0]
        
        for word in words[1:]:
            if len(current_line) + len(word) + 2 <= max_length:
                current_line += ", " + word
            else:
                wrapped_lines.append(current_line + ",")
                current_line = ' ' * len(f"{macro_name_with_indent}(") + word

        wrapped_lines.append(current_line)
        return wrapped_lines
                    
    generate_cpp_code(100)
    */

    if constexpr (FieldsCount == 1)
        WRAP_FIELDS(f0)
    else if constexpr (FieldsCount == 2)
        WRAP_FIELDS(f0, f1)
    else if constexpr (FieldsCount == 3)
        WRAP_FIELDS(f0, f1, f2)
    else if constexpr (FieldsCount == 4)
        WRAP_FIELDS(f0, f1, f2, f3)
    else if constexpr (FieldsCount == 5)
        WRAP_FIELDS(f0, f1, f2, f3, f4)
    else if constexpr (FieldsCount == 6)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5)
    else if constexpr (FieldsCount == 7)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6)
    else if constexpr (FieldsCount == 8)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7)
    else if constexpr (FieldsCount == 9)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8)
    else if constexpr (FieldsCount == 10)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9)
    else if constexpr (FieldsCount == 11)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10)
    else if constexpr (FieldsCount == 12)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11)
    else if constexpr (FieldsCount == 13)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12)
    else if constexpr (FieldsCount == 14)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13)
    else if constexpr (FieldsCount == 15)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14)
    else if constexpr (FieldsCount == 16)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15)
    else if constexpr (FieldsCount == 17)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16)
    else if constexpr (FieldsCount == 18)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17)
    else if constexpr (FieldsCount == 19)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18)
    else if constexpr (FieldsCount == 20)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19)
    else if constexpr (FieldsCount == 21)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20)
    else if constexpr (FieldsCount == 22)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21)
    else if constexpr (FieldsCount == 23)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22)
    else if constexpr (FieldsCount == 24)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23)
    else if constexpr (FieldsCount == 25)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24)
    else if constexpr (FieldsCount == 26)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25)
    else if constexpr (FieldsCount == 27)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26)
    else if constexpr (FieldsCount == 28)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27)
    else if constexpr (FieldsCount == 29)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28)
    else if constexpr (FieldsCount == 30)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29)
    else if constexpr (FieldsCount == 31)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30)
    else if constexpr (FieldsCount == 32)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31)
    else if constexpr (FieldsCount == 33)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32)
    else if constexpr (FieldsCount == 34)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33)
    else if constexpr (FieldsCount == 35)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34)
    else if constexpr (FieldsCount == 36)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35)
    else if constexpr (FieldsCount == 37)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36)
    else if constexpr (FieldsCount == 38)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37)
    else if constexpr (FieldsCount == 39)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38)
    else if constexpr (FieldsCount == 40)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39)
    else if constexpr (FieldsCount == 41)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40)
    else if constexpr (FieldsCount == 42)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41)
    else if constexpr (FieldsCount == 43)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42)
    else if constexpr (FieldsCount == 44)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43)
    else if constexpr (FieldsCount == 45)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44)
    else if constexpr (FieldsCount == 46)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45)
    else if constexpr (FieldsCount == 47)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46)
    else if constexpr (FieldsCount == 48)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47)
    else if constexpr (FieldsCount == 49)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48)
    else if constexpr (FieldsCount == 50)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49)
    else if constexpr (FieldsCount == 51)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50)
    else if constexpr (FieldsCount == 52)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51)
    else if constexpr (FieldsCount == 53)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52)
    else if constexpr (FieldsCount == 54)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53)
    else if constexpr (FieldsCount == 55)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54)
    else if constexpr (FieldsCount == 56)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55)
    else if constexpr (FieldsCount == 57)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56)
    else if constexpr (FieldsCount == 58)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57)
    else if constexpr (FieldsCount == 59)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58)
    else if constexpr (FieldsCount == 60)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59)
    else if constexpr (FieldsCount == 61)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60)
    else if constexpr (FieldsCount == 62)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61)
    else if constexpr (FieldsCount == 63)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62)
    else if constexpr (FieldsCount == 64)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63)
    else if constexpr (FieldsCount == 65)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64)
    else if constexpr (FieldsCount == 66)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65)
    else if constexpr (FieldsCount == 67)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66)
    else if constexpr (FieldsCount == 68)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67)
    else if constexpr (FieldsCount == 69)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68)
    else if constexpr (FieldsCount == 70)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69)
    else if constexpr (FieldsCount == 71)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70)
    else if constexpr (FieldsCount == 72)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71)
    else if constexpr (FieldsCount == 73)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72)
    else if constexpr (FieldsCount == 74)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73)
    else if constexpr (FieldsCount == 75)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74)
    else if constexpr (FieldsCount == 76)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75)
    else if constexpr (FieldsCount == 77)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76)
    else if constexpr (FieldsCount == 78)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77)
    else if constexpr (FieldsCount == 79)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78)
    else if constexpr (FieldsCount == 80)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79)
    else if constexpr (FieldsCount == 81)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80)
    else if constexpr (FieldsCount == 82)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81)
    else if constexpr (FieldsCount == 83)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82)
    else if constexpr (FieldsCount == 84)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83)
    else if constexpr (FieldsCount == 85)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83, f84)
    else if constexpr (FieldsCount == 86)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83, f84, f85)
    else if constexpr (FieldsCount == 87)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83, f84, f85, f86)
    else if constexpr (FieldsCount == 88)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83, f84, f85, f86, f87)
    else if constexpr (FieldsCount == 89)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83, f84, f85, f86, f87, f88)
    else if constexpr (FieldsCount == 90)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83, f84, f85, f86, f87, f88, f89)
    else if constexpr (FieldsCount == 91)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90)
    else if constexpr (FieldsCount == 92)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90, f91)
    else if constexpr (FieldsCount == 93)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90, f91, f92)
    else if constexpr (FieldsCount == 94)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90, f91, f92,
                    f93)
    else if constexpr (FieldsCount == 95)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90, f91, f92,
                    f93, f94)
    else if constexpr (FieldsCount == 96)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90, f91, f92,
                    f93, f94, f95)
    else if constexpr (FieldsCount == 97)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90, f91, f92,
                    f93, f94, f95, f96)
    else if constexpr (FieldsCount == 98)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90, f91, f92,
                    f93, f94, f95, f96, f97)
    else if constexpr (FieldsCount == 99)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90, f91, f92,
                    f93, f94, f95, f96, f97, f98)
    else if constexpr (FieldsCount == 100)
        WRAP_FIELDS(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14,
                    f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27,
                    f28, f29, f30, f31, f32, f33, f34, f35, f36, f37, f38, f39, f40,
                    f41, f42, f43, f44, f45, f46, f47, f48, f49, f50, f51, f52, f53,
                    f54, f55, f56, f57, f58, f59, f60, f61, f62, f63, f64, f65, f66,
                    f67, f68, f69, f70, f71, f72, f73, f74, f75, f76, f77, f78, f79,
                    f80, f81, f82, f83, f84, f85, f86, f87, f88, f89, f90, f91, f92,
                    f93, f94, f95, f96, f97, f98, f99)
    else
        static_assert(
            FieldsCount <= 100,
            "\n\nThis error occurs:\n"
            "1) Your struct has more than 100 fields which is unsupported"
            "2) You have added a custom constructor to your struct which is unsupported.\n\n");
#undef WRAP_FIELDS
}

} // namespace ext::reflection::details