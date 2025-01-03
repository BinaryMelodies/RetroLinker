#ifndef LINKER_H
#define LINKER_H

#include <fstream>
#include <optional>
#include <string>
#include <variant>
#include "module.h"
#include "writer.h"
#include "../script/script.h"

namespace Linker
{
	class Exception
	{
	public:
		std::string message;
		Exception(std::string message) : message(message)
		{
		}
	};

	/**
	 * @brief A helper class to collect sections into segments
	 */
	class LinkerManager
	{
	protected:
		/**
		 * @brief Holds the current address value when there is no current_segment
		 */
		offset_t current_address;

		bool current_is_template;
		bool current_is_template_head;
		offset_t template_counter;
		std::string current_template_name;
	public:
		/**
		 * @brief The base address of the current section
		 *
		 * By default, sections within a segment are assumed to have the same segment base.
		 * This variable stores the segment base, as valid for the currently processed section.
		 */
		offset_t current_base;
		/**
		 * @brief Ordered sequence of segments
		 */
		std::vector<Segment *> segment_vector;
		/**
		 * @brief Map of segments from their names
		 */
		std::map<std::string, Segment *> segment_map;
		/**
		 * @brief Currently processed segment
		 */
		Segment * current_segment;
		/**
		 * @brief Parameters that permit customizing the linker script
		 */
		std::map<std::string, Location> linker_parameters;
		/**
		 * @brief Contents of the linker script
		 */
		std::string linker_script;

		void InitializeLinkerManager();
		void ClearLinkerManager();

		LinkerManager()
		{
			InitializeLinkerManager();
		}

		~LinkerManager()
		{
			ClearLinkerManager();
		}

		/**
		 * @brief Sets up the linker script and linker parameters
		 */
		void SetLinkScript(std::string script_file, std::map<std::string, std::string>& options);

		/**
		 * @brief Sets a single linker parameter, if inside options
		 */
		bool SetLinkerParameter(std::map<std::string, std::string>& options, std::string key);

		/**
		 * @brief Sets a single linker parameter, if inside options
		 */
		bool SetLinkerParameter(std::map<std::string, std::string>& options, std::string key, std::string variable);

		/**
		 * @brief Compiles the linker script into an internal format
		 */
		Script::List * GetScript(Linker::Module& module);

		/**
		 * @brief Retrieves current address pointer
		 */
		offset_t GetCurrentAddress() const;

		/**
		 * @brief Moves the current address pointer further, and if the current segment already contains data, fill it up to the point
		 */
		void SetCurrentAddress(offset_t address);

		/**
		 * @brief Aligns current address to alignment, using SetCurrentAddress
		 */
		void AlignCurrentAddress(offset_t align);

		/**
		 * @brief Sets the base of the current section (the value from which offsets are counted from)
		 */
		void SetLatestBase(offset_t address);

		/**
		 * @brief Closes the current segment, sets current_segment to null
		 */
		void FinishCurrentSegment();

		/**
		 * @brief Callback function when allocating a new segment
		 * When the linker script runs, it creates segments consecutively. Overriding this method permits the output format to handle the allocated segment.
		 */
		virtual void OnNewSegment(Segment * segment);

		/**
		 * @brief Terminates the current segment (if there is one), creates a new segment and attaches it to the image
		 * @param name The name of the new segment
		 */
		Segment * AppendSegment(std::string name);

		/**
		 * @brief Attempts to fetch a segment, returns null if not found
		 */
		Segment * FetchSegment(std::string name);

		/**
		 * @brief Adds a new section to the current segment, sets the base to the same as the segment
		 */
		void AppendSection(Section * section);

		/**
		 * @brief Executes a parsed linker script on a module and collects segments
		 * The function OnNewSegment can be defined to handle each newly allocated segment.
		 */
		void ProcessScript(Script::List * directives, Module& module);
		void ProcessAction(Script::Node * action, Module& module);
		void PostProcessAction(Script::Node * action, Module& module);
		void ProcessCommand(Script::Node * command, Module& module);
		bool CheckPredicate(Script::Node * predicate, Section * section, Module& module);
		offset_t EvaluateExpression(Script::Node * expression, Module& module);
	};

	bool starts_with(std::string str, std::string start);
	bool ends_with(std::string str, std::string end);
}

#endif /* LINKER_H */
