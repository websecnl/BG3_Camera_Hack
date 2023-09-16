#pragma once
#include <iostream>
#include <vector>
namespace Utils {
	std::uint8_t* PatternScan(const char* signature, std::uint8_t* startAddress = nullptr, const char* module_name = nullptr);
	std::vector<std::int64_t> FindAllPatterns(const char* signature, const char* module_name = nullptr);
}
