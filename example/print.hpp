#ifndef PRINT_HPP_985ECFRWB6T
#define PRINT_HPP_985ECFRWB6T

#include <format>
#include <iostream>

template <typename... Args>
void print(std::format_string<Args...>&& fmt, Args&&... args)
{
    std::cout << std::format(std::forward<std::format_string<Args...>>(fmt), std::forward<Args>(args)...) << std::flush;
}

#endif /* end of include guard: PRINT_HPP_985ECFRWB6T */
