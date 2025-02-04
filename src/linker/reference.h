#ifndef REFERENCE_H
#define REFERENCE_H

#include <fstream>
#include <optional>
#include <string>
#include <variant>

namespace Linker
{
	class Module;

	/**
	 * @brief Represents a reference stored in a linker script
	 *
	 * Such a reference may be resolved to a symbol or location.
	 */
	class Reference
	{
	public:
		std::optional<std::string> segment;
		std::variant<std::string, offset_t> offset;

		Reference(std::variant<std::string, offset_t> offset = offset_t(0))
			: segment(), offset(offset)
		{
		}

		Reference(std::string segment, std::variant<std::string, offset_t> offset = offset_t(0))
			: segment(segment), offset(offset)
		{
		}

		/**
		 * @brief Converts to location, resolving the symbol if it is a symbol
		 */
		Location ToLocation(Module& module) const;
	};

	std::ostream& operator<<(std::ostream& out, const Reference& ref);
}

#endif /* REFERENCE_H */
