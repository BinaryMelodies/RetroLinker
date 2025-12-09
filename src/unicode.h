#ifndef UNICODE_H
#define UNICODE_H

namespace UnicodeConversion
{
	enum Result
	{
		/**
		 * @brief Conversion succeeded
		 *
		 * Converted a single codepoint, advanced the input and output pointers
		 */
		Success,
		/**
		 * @brief Conversion succeeded, null character reached
		 *
		 * Converted a single codepoint, advanced the input and output pointers.
		 * If string processing terminates at null character, this is a signal to terminate conversion.
		 */
		NullCharacter,
		/**
		 * @brief Conversion encountered a surrogate character
		 *
		 * Converted a single codepoint, advanced the input and output pointers.
		 * The codepoint corresponds to a surrogate character, which is invalid in a Unicode text.
		 * If the target conversion is UTF-16, the generated character should be thrown away.
		 * This is never generated when converting from UTF-16.
		 */
		InvalidCharacter,
		/**
		 * @brief Conversion encountered a character whose codepoint value exceeds 0x0010FFFF
		 *
		 * Advanced the input codepoint, and if the output is not UTF-16, the output codepoint as well.
		 * This is an invalid codepoint, but UTF-32 and (extended) UTF-8 can represent it.
		 * If the output is UTF-16, the codepoint is ignored for the output.
		 * For correct Unicode behavior, the generated character should be thrown away.
		 */
		OutOfRangeCharacter,
		/**
		 * @brief The encoding does not represent a valid codepoint due to an invalid initial value
		 *
		 * The initial value in the input does not represent a valid start of an encoded codepoint.
		 * Advanced the input and output pointers.
		 * The generated output is the character that is closest in value to the incorrect encoding.
		 * The input pointer is advanced by one.
		 */
		InvalidValue,
		/**
		 * @brief The encoding does not represent a valid codepoint due to truncation
		 *
		 * Attempts to parse a partial codepoint encoding.
		 * Advanced the input and output pointers.
		 * The generated output is the character that is closest in value to the incorrect encoding.
		 * The input pointer is advanced to the following position after the last character that could be part of a valid but truncated encoding.
		 */
		InterruptedEncoding,
		/**
		 * @brief The encoding does not represent a valid codepoint due to the input being too short
		 *
		 * Attempts to parse a partial codepoint encoding.
		 * Advanced the input and output pointers.
		 * The generated output is the character that is closest in value to the incorrect encoding.
		 * The input pointer is advanced to the end of the input buffer.
		 */
		InputTruncated,
		/**
		 * @brief The output buffer cannot fit the codepoint
		 *
		 * The conversion has not taken place, and the input and output pointers have not advanced.
		 * An additional parameter should contain the required buffer size for the output.
		 * The function can be called again with a larger buffer.
		 */
		BufferOverflow,
	};

	static inline bool IsSuccess(Result result)
	{
		switch(result)
		{
		case Success:
		case NullCharacter:
			return true;
		default:
			return false;
		}
	}

	static inline bool InputAdvanced(Result result)
	{
		switch(result)
		{
		case BufferOverflow:
			return false;
		default:
			return true;
		}
	}

	static inline bool InputTerminated(Result result)
	{
		switch(result)
		{
		case NullCharacter:
		case InputTruncated:
			return true;
		default:
			return false;
		}
	}
}

/**
 * @brief Converts a single UTF-32 codepoint to UTF-8
 *
 * @param[in,out] input The pointer to the input UTF-32 string
 * @param[in,out] output The pointer to the output UTF-8 string
 * @param[in] max_output Maximum number of units that can be written into the output
 * @param[out] required_output The number of units actually required to encode the next codepoint
 * @return May result in Success, NullCharacter, InvalidCharacter, OutOfRangeCharacter, BufferOverflow
 */
UnicodeConversion::Result UTF32ToUTF8(char32_t const *& input, char *& output, size_t max_output, size_t& required_output);
/**
 * @brief Converts a single UTF-8 codepoint to UTF-32
 *
 * @param[in,out] input The pointer to the input UTF-8 string
 * @param[in,out] output The pointer to the output UTF-32 string
 * @param[in] max_output Maximum number of units that can be read from the input
 * @param[in] force_out_of_range_decoding When set to true, codepoints beyond 0x0010FFFF are also parsed, otherwise all bytes from 0xF8 to 0xFF are reported as InvalidValue
 * @return May result in Success, NullCharacter, InvalidCharacter, OutOfRangeCharacter, InvalidValue, InterruptedEncoding, InputTruncated
 */
UnicodeConversion::Result UTF8ToUTF32(char const *& input, char32_t *& output, size_t max_input, bool force_out_of_range_decoding = false);
/**
 * @brief Converts a single UTF-32 codepoint to UTF-16
 *
 * @param[in,out] input The pointer to the input UTF-32 string
 * @param[in,out] output The pointer to the output UTF-16 string
 * @param[in] max_output Maximum number of units that can be written into the output
 * @param[out] required_output The number of units actually required to encode the next codepoint
 * @return May result in Success, NullCharacter, InvalidCharacter, OutOfRangeCharacter (no output is generated), BufferOverflow
 */
UnicodeConversion::Result UTF32ToUTF16(char32_t const *& input, char16_t *& output, size_t max_output, size_t& required_output);
/**
 * @brief Converts a single UTF-16 codepoint to UTF-32
 *
 * @param[in,out] input The pointer to the input UTF-16 string
 * @param[in,out] output The pointer to the output UTF-32 string
 * @param[in] max_output Maximum number of units that can be read from the input
 * @return May result in Success, NullCharacter, InvalidValue, InterruptedEncoding, InputTruncated
 */
UnicodeConversion::Result UTF16ToUTF32(char16_t const *& input, char32_t *& output, size_t max_input);

UnicodeConversion::Result UTF16ToUTF8(char16_t const *& input, char *& output, size_t max_input, size_t max_output, size_t& required_output);
UnicodeConversion::Result UTF8ToUTF16(char const *& input, char16_t *& output, size_t max_input, size_t max_output, size_t& required_output);

#endif /* UNICODE_H */
