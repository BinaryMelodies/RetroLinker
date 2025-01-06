#ifndef LINKER_H
#define LINKER_H

#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include "module.h"
#include "writer.h"
#include "../script/script.h"

namespace Linker
{
	/**
	 * @brief A helper class to collect sections into segments
	 */
	class LinkerManager
	{
	protected:
		/**
		 * @brief Holds the current address value when there is no current_segment
		 */
		offset_t current_address = 0;

		bool current_is_template = false;
		bool current_is_template_head = false;
		offset_t template_counter = 0;
		std::string current_template_name;
	public:
		/**
		 * @brief The base address of the current section
		 *
		 * By default, sections within a segment are assumed to have the same segment base.
		 * This variable stores the segment base, as valid for the currently processed section.
		 */
		offset_t current_base = 0;
		/**
		 * @brief Ordered sequence of segments
		 */
		std::vector<std::shared_ptr<Segment>> segment_vector;
		/**
		 * @brief Map of segments from their names
		 */
		std::map<std::string, std::shared_ptr<Segment>> segment_map;
		/**
		 * @brief Currently processed segment
		 */
		std::shared_ptr<Segment> current_segment;
		/**
		 * @brief Parameters that permit customizing the linker script
		 */
		std::map<std::string, Location> linker_parameters;
		/**
		 * @brief Contents of the linker script
		 */
		std::string linker_script;

		void ClearLinkerManager();

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
		std::unique_ptr<Script::List> GetScript(Linker::Module& module);

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
		virtual void OnNewSegment(std::shared_ptr<Segment> segment);

		/**
		 * @brief Terminates the current segment (if there is one), creates a new segment and attaches it to the image
		 * @param name The name of the new segment
		 */
		std::shared_ptr<Segment> AppendSegment(std::string name);

		/**
		 * @brief Attempts to fetch a segment, returns null if not found
		 */
		std::shared_ptr<Segment> FetchSegment(std::string name);

		/**
		 * @brief Adds a new section to the current segment, sets the base to the same as the segment
		 */
		void AppendSection(std::shared_ptr<Section> section);

		/**
		 * @brief Executes a parsed linker script on a module and collects segments
		 * The function OnNewSegment can be defined to handle each newly allocated segment.
		 */
		void ProcessScript(std::unique_ptr<Script::List>& directives, Module& module);
		void ProcessAction(std::unique_ptr<Script::Node>& action, Module& module);
		void PostProcessAction(std::unique_ptr<Script::Node>& action, Module& module);
		void ProcessCommand(std::unique_ptr<Script::Node>& command, Module& module);
		bool CheckPredicate(std::unique_ptr<Script::Node>& predicate, std::shared_ptr<Section> section, Module& module);
		offset_t EvaluateExpression(std::unique_ptr<Script::Node>& expression, Module& module);
	};

	bool starts_with(std::string str, std::string start);
	bool ends_with(std::string str, std::string end);
}

#endif /* LINKER_H */
