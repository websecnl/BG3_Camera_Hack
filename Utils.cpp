#include <iostream>
#include "Utils.h"
#include <Windows.h>
#include <vector>
#include <sstream>

namespace Utils {
    std::uint8_t* PatternScan(const char* signature, std::uint8_t* startAddress, const char* module_name) {


        const auto module_handle = GetModuleHandleA(nullptr);

        if (!module_handle) {
            return nullptr;
        }

        static auto pattern_to_byte = [](const char* pattern) {
            auto bytes = std::vector<uint8_t>{};
            auto start = const_cast<char*>(pattern);
            auto end = const_cast<char*>(pattern) + std::strlen(pattern);

            for (auto current = start; current < end; ++current) {
                if (*current == '?') {
                    ++current;

                    if (*current == '?')
                        ++current;

                    bytes.push_back(-1);
                }
                else {
                    bytes.push_back((uint8_t)std::strtoul(current, &current, 16));
                }
            }
            return bytes;
        };

        auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);
        auto nt_headers =
            reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module_handle) + dos_header->e_lfanew);

        auto size_of_image = (size_t)nt_headers->OptionalHeader.SizeOfImage;
        auto pattern_bytes = pattern_to_byte(signature);

        if (startAddress == nullptr)
            startAddress = (uint8_t*)GetModuleHandle(nullptr);

        auto scan_bytes = reinterpret_cast<std::uint8_t*>(startAddress);


        size_t dif = (size_t)(size_of_image)-(size_t)(startAddress);


        auto s = pattern_bytes.size();
        auto d = pattern_bytes.data();

        int match_count = 0;
        std::uint8_t* ptr = startAddress;

        auto end = (uint8_t*)((uintptr_t)startAddress + (uintptr_t)size_of_image);
        
        if (!pattern_bytes.empty()) {
            std::uint8_t* ptr = startAddress;
            std::uint8_t* end = (std::uint8_t * )((uintptr_t)GetModuleHandle(nullptr) + (uintptr_t)size_of_image);

            while (ptr < end) {
                size_t match_count = 0;

                uint8_t* match = nullptr;
                int j = 0;
                while (j < s && ptr != nullptr)
                {
                    if (pattern_bytes[j] == 0xFF || *ptr == pattern_bytes[j]) {
                        if (match == nullptr)
                            match = ptr;
                        
                        j++;
                        ptr++;
                        if (j == pattern_bytes.size())
                            return match; // Pattern found
                    }
                    else {
                        match = nullptr;
                        break;
                    }

                }
               
                ptr++;
            }
        }

        /*for (auto i = 0ul; i < size_of_image - s - dif; ++i) {
            bool found = true;

            for (auto j = 0ul; j < s; ++j) {
                if (scan_bytes[i + j] != d[j] && d[j] != -1) {
                    found = false;
                    break;
                }
            }
            if (found)
                return &scan_bytes[i];
        }*/
        std::stringstream ss;
        ss << "Invalid pattern " << signature << std::endl;

        throw std::runtime_error(ss.str());
        return nullptr;
    }
    std::vector<std::int64_t> FindAllPatterns(const char* signature, const char* module_name)
    {
        std::vector<std::int64_t> list;
        const auto module_handle = GetModuleHandleA(nullptr);

        if (!module_handle)
            return list;

        static auto pattern_to_byte = [](const char* pattern) {
            auto bytes = std::vector<int>{};
            auto start = const_cast<char*>(pattern);
            auto end = const_cast<char*>(pattern) + std::strlen(pattern);

            for (auto current = start; current < end; ++current) {
                if (*current == '?') {
                    ++current;

                    if (*current == '?')
                        ++current;

                    bytes.push_back(-1);
                }
                else {
                    bytes.push_back(std::strtoul(current, &current, 16));
                }
            }
            return bytes;
        };

        auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);
        auto nt_headers =
            reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module_handle) + dos_header->e_lfanew);

        auto size_of_image = nt_headers->OptionalHeader.SizeOfImage;
        auto pattern_bytes = pattern_to_byte(signature);


        auto scan_bytes = reinterpret_cast<std::uint8_t*>(GetModuleHandle(nullptr));



        auto s = pattern_bytes.size();
        auto d = pattern_bytes.data();

        for (auto i = 0ul; i < size_of_image - s ; ++i) {
            bool found = true;

            for (auto j = 0ul; j < s; ++j) {
                if (scan_bytes[i + j] != d[j] && d[j] != -1) {
                    found = false;
                    break;
                }
            }
            if (found)
                list.push_back((int64_t)(&scan_bytes[i]));
        }
    }
}