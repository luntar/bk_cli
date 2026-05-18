/**
 * @file Command_Helpers.h
 * @brief Small helper functions for the embedded CLI.
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace bk {

enum class argument_type {
    unknown,
    u64,
    i64,
    u32,
    i32,
    u16,
    i16,
    f32,
    f64,
    u8,
    i8,
    str,
    u64_opt,
    i64_opt,
    u32_opt,
    i32_opt,
    u16_opt,
    i16_opt,
    f32_opt,
    f64_opt,
    u8_opt,
    i8_opt,
    str_opt
};

struct Command_Helpers {
    static std::size_t count_tokens(std::string_view sv);

    static uint64_t to_uint64(const std::string& str, uint64_t default_value = 0);
    static int64_t to_int64(const std::string& str, int64_t default_value = 0);
    static uint32_t to_uint32(const std::string& str, uint32_t default_value = 0);
    static int32_t to_int32(const std::string& str, int32_t default_value = 0);
    static uint16_t to_uint16(const std::string& str, uint16_t default_value = 0);
    static int16_t to_int16(const std::string& str, int16_t default_value = 0);
    static uint8_t to_uint8(const std::string& str, uint8_t default_value = 0);
    static int8_t to_int8(const std::string& str, int8_t default_value = 0);
    static char to_hex_char(const std::string& str, char default_value = 0);
    static char to_char(const std::string& str, char default_value = 0);
    static float to_float(const std::string& s, float default_val);
    static double to_double(const std::string& s, double default_val);
    static std::string to_string(const std::string& str, const std::string& default_value = {});

    static std::vector<std::string_view> tokenize(std::string_view sv);
    static std::string remove_first(const std::string& args);
    static bool is_optional(argument_type t);
    static std::string argument_type_to_string(argument_type t);
    static std::string escape_json_string(const std::string& input);
};

}
