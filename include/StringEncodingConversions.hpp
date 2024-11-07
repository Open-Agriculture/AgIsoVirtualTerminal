//================================================================================================
/// @file StringEncodingConversions.hpp
///
/// @brief Helper functions for converting ISO 11783's string encodings to UTF-8
/// @author Adrian Del Grosso
///
/// @copyright 2024 Adrian Del Grosso
//================================================================================================
#ifndef STRING_ENCODING_CONVERSIONS_HPP
#define STRING_ENCODING_CONVERSIONS_HPP

#include <string>

enum class SourceEncoding
{
	ISO8859_1,
	ISO8859_2,
	ISO8859_4,
	ISO8859_5,
	ISO8859_7,
	ISO8859_15
};

void convert_string_to_utf_8(SourceEncoding encoding, const std::string &input, std::string &output, bool autoWrappingEnabled);

#endif // STRING_ENCODING_CONVERSIONS_HPP
