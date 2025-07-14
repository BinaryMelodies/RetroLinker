#ifndef OMF_H
#define OMF_H

#include <array>
#include <variant>
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/format.h"
#include "../linker/reader.h"
#include "../linker/writer.h"

/* TODO: incomplete */

/* Intel Object Module format (input only) */

namespace OMF
{
	/** @brief Convenience class to calculate the checksum of a record while writing it */
	class ChecksumWriter
	{
	public:
		Linker::Writer * wr;
		uint8_t checksum = 0;

		ChecksumWriter(Linker::Writer& wr)
			: wr(&wr)
		{
		}

		void WriteWord(size_t bytes, uint64_t value)
		{
			wr->WriteWord(bytes, value);
			for(size_t offset = 0; offset < bytes; offset++, value >>= 8)
			{
				checksum -= value;
			}
		}

		size_t WriteData(const std::vector<uint8_t>& data)
		{
			size_t bytes = wr->WriteData(data);
			for(size_t offset = 0; offset < bytes; offset++)
			{
				checksum -= data[offset];
			}
			return bytes;
		}

		size_t WriteData(std::string text)
		{
			wr->WriteData(text);
			for(size_t offset = 0; offset < text.size(); offset++)
			{
				checksum -= text[offset];
			}
			return text.size();
		}

		void Skip(offset_t count)
		{
			wr->Skip(count);
		}
	};

	/**
	 * @brief Base class for a family of Intel Relocatable Object formats
	 */
	class OMFFormat : public virtual Linker::Format
	{
	public:
		virtual ~OMFFormat() = default;

		/** @brief Reads a string prefixed with a length byte */
		static std::string ReadString(Linker::Reader& rd, size_t max_bytes = size_t(-1));

		/** @brief Writes a string prefixed with a length byte */
		static void WriteString(ChecksumWriter& wr, std::string text);

		/** @brief An index referring to an element or definition in the file, typically stored as 1 or 2 bytes, used for OMF86 and OMF96 */
		typedef uint16_t index_t;

		/** @brief Parses a 1 or 2 byte index value */
		static index_t ReadIndex(Linker::Reader& rd);

		/** @brief Produces a 1 or 2 byte index value */
		static void WriteIndex(ChecksumWriter& wr, index_t index);

		/** @brief Determines if the index value requires 1 or 2 bytes to store */
		static size_t IndexSize(index_t index);

		/**
		 * @brief Base class representing OMF record types
		 *
		 * Any record type is expected to extend this class.
		 * If several record types have the same or similar layout, they can share the same class, since the record type can be determined using the record_type field.
		 */
		template <typename RecordTypeByte, typename FormatType, typename ModuleType>
			class Record
		{
		public:
			/** @brief Offset of record within the file */
			offset_t record_offset;
			/** @brief Length of record body file, excluding the type byte and 2-byte length field */
			uint16_t record_length;
			/** @brief A byte value identifying the type of record */
			RecordTypeByte record_type;

			Record(RecordTypeByte record_type = RecordTypeByte(0))
				: record_type(record_type)
			{
			}

			virtual ~Record() = default;

			offset_t RecordEnd() const
			{
				return record_offset + record_length + 2;
			}

			/** @brief Reads the record contents, except for the type, length and checksum */
			virtual void ReadRecordContents(FormatType * omf, ModuleType * mod, Linker::Reader& rd) = 0;

			/** @brief Calculates the required bytes to write the record, might be less than record_length */
			virtual uint16_t GetRecordSize(FormatType * omf, ModuleType * mod) const = 0;

			/** @brief Writes the record contents, except for the type, length and checksum */
			virtual void WriteRecordContents(FormatType * omf, ModuleType * mod, ChecksumWriter& wr) const = 0;

			/** @brief Writes the full record */
			virtual void WriteRecord(FormatType * omf, ModuleType * mod, Linker::Writer& wr) const
			{
				ChecksumWriter ckswr(wr);
				ckswr.WriteWord(1, record_type);
				ckswr.WriteWord(2, GetRecordSize(omf, mod));
				WriteRecordContents(omf, mod, ckswr);
				wr.WriteWord(1, ckswr.checksum);
			}

			/** @brief Updates all fields that will be used for writing an OMF module, should be called before output */
			virtual void CalculateValues(FormatType * omf, ModuleType * mod)
			{
				record_offset = omf->file_size;
				record_length = GetRecordSize(omf, mod);
				record_offset += record_length;
			}

			/** @brief Resolves any fields read from an OMF module, should be called after inpnut */
			virtual void ResolveReferences(FormatType * omf, ModuleType * mod)
			{
			}

			virtual void DumpAddFields(Dumper::Dumper& dump, Dumper::Region& region, const FormatType * omf, const ModuleType * mod, size_t record_index) const
			{
			}

			virtual void Dump(Dumper::Dumper& dump, const FormatType * omf, const ModuleType * mod, size_t record_index) const
			{
				Dumper::Region record_region("Record", record_offset, RecordEnd() + 1 - record_offset, 8);
				record_region.InsertField(0, "Index", Dumper::DecDisplay::Make(), offset_t(record_index + 1));
				record_region.AddField("Record type", Dumper::HexDisplay::Make(2), offset_t(record_type));
				FormatType::DumpAddFields(this, dump, record_region, omf, mod, record_index);
				DumpAddFields(dump, record_region, omf, mod, record_index);
				record_region.Display(dump);
			}

			/** @brief Records are 32-bit if the least significant bit of their record type is set (only meaningful for OMF86Format) */
			bool Is32Bit(FormatType * omf) const
			{
				return (record_type & 1) != 0;
			}

			/** @brief The number of bytes in an offset appearing inside the record, 2 for 16-bit records, 4 for 32-bit records (only meaningful for OMF86Format) */
			size_t GetOffsetSize(FormatType * omf) const
			{
				return Is32Bit(omf) ? 4 : 2;
			}
		};

		/** @brief An unparsed record type, if the format is not recognized */
		template <typename RecordTypeByte, typename FormatType, typename ModuleType>
			class UnknownRecord : public Record<RecordTypeByte, FormatType, ModuleType>
		{
		public:
			/** @brief Contents of the record, without the type, length and checksum */
			std::vector<uint8_t> data;

			UnknownRecord(RecordTypeByte record_type = RecordTypeByte(0))
				: Record<RecordTypeByte, FormatType, ModuleType>(record_type)
			{
			}

			void ReadRecordContents(FormatType * omf, ModuleType * mod, Linker::Reader& rd) override
			{
				data.resize(Record<RecordTypeByte, FormatType, ModuleType>::record_length - 1);
				rd.ReadData(data);
			}

			uint16_t GetRecordSize(FormatType * omf, ModuleType * mod) const override
			{
				return data.size() + 1;
			}

			void WriteRecordContents(FormatType * omf, ModuleType * mod, ChecksumWriter& wr) const override
			{
				wr.WriteData(data);
			}
		};

		/** @brief A record type that has no contents, aside from the type, length and checksum, used for BLKEND */
		template <typename RecordTypeByte, typename FormatType, typename ModuleType>
			class EmptyRecord : public Record<RecordTypeByte, FormatType, ModuleType>
		{
		public:
			EmptyRecord(RecordTypeByte record_type = RecordTypeByte(0))
				: Record<RecordTypeByte, FormatType, ModuleType>(record_type)
			{
			}

			void ReadRecordContents(FormatType * omf, ModuleType * mod, Linker::Reader& rd) override
			{
			}

			uint16_t GetRecordSize(FormatType * omf, ModuleType * mod) const override
			{
				return 1;
			}

			void WriteRecordContents(FormatType * omf, ModuleType * mod, ChecksumWriter& wr) const override
			{
			}
		};

		/** @brief A record representing data to be loaded into a segment
		 *
		 * This layout is shared by OMF80, OMF51 and OMF96.
		 */
		template <typename RecordTypeByte, typename FormatType, typename ModuleType>
			class ContentRecord : public Record<RecordTypeByte, FormatType, ModuleType>
		{
		public:
			uint8_t segment_id = 0;
			uint16_t offset = 0;
			std::vector<uint8_t> data;

			ContentRecord(RecordTypeByte record_type = RecordTypeByte(0))
				: Record<RecordTypeByte, FormatType, ModuleType>(record_type)
			{
			}

			void ReadRecordContents(FormatType * omf, ModuleType * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(FormatType * omf, ModuleType * mod) const override;
			void WriteRecordContents(FormatType * omf, ModuleType * mod, ChecksumWriter& wr) const override;
		};

		/** @brief A record representing line number information
		 *
		 * This layout is shared by OMF80 and OMF96.
		 * OMF86 has a similar format but with a 32-bit extension.
		 */
		template <typename RecordTypeByte, typename FormatType, typename ModuleType>
			class LineNumbersRecord : public Record<RecordTypeByte, FormatType, ModuleType>
		{
		public:
			class LineNumber
			{
			public:
				uint16_t line_number = 0;
				uint16_t offset = 0;

				static LineNumber Read(FormatType * omf, Linker::Reader& rd);
				void Write(FormatType * omf, ChecksumWriter& wr) const;
			};

			uint8_t segment_id = 0;
			std::vector<LineNumber> line_numbers;

			LineNumbersRecord(RecordTypeByte record_type = RecordTypeByte(0))
				: Record<RecordTypeByte, FormatType, ModuleType>(record_type)
			{
			}

			void ReadRecordContents(FormatType * omf, ModuleType * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(FormatType * omf, ModuleType * mod) const override;
			void WriteRecordContents(FormatType * omf, ModuleType * mod, ChecksumWriter& wr) const override;
		};

		/** @brief Library header record, used for LIBHED */
		template <typename RecordTypeByte, typename FormatType, typename ModuleType>
			class LibraryHeaderRecord : public Record<RecordTypeByte, FormatType, ModuleType>
		{
		public:
			/** @brief Number of modules contained */
			uint16_t module_count;
			/** @brief Offset to the first byte of the library module, as a block:byte pair */
			uint16_t block_number;
			/** @brief Offset to the first byte of the library module, as a block:byte pair */
			uint16_t byte_number;

			LibraryHeaderRecord(RecordTypeByte record_type = RecordTypeByte(0))
				: Record<RecordTypeByte, FormatType, ModuleType>(record_type)
			{
			}

			void ReadRecordContents(FormatType * omf, ModuleType * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(FormatType * omf, ModuleType * mod) const override;
			void WriteRecordContents(FormatType * omf, ModuleType * mod, ChecksumWriter& wr) const override;
		};

		/** @brief Library module names, used for LIBNAM */
		template <typename RecordTypeByte, typename FormatType, typename ModuleType>
			class LibraryModuleNamesRecord : public Record<RecordTypeByte, FormatType, ModuleType>
		{
		public:
			/** @brief List of module names */
			std::vector<std::string> names;

			LibraryModuleNamesRecord(RecordTypeByte record_type = RecordTypeByte(0))
				: Record<RecordTypeByte, FormatType, ModuleType>(record_type)
			{
			}

			void ReadRecordContents(FormatType * omf, ModuleType * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(FormatType * omf, ModuleType * mod) const override;
			void WriteRecordContents(FormatType * omf, ModuleType * mod, ChecksumWriter& wr) const override;
		};

		/** @brief Library module offsets, used for LIBLOC */
		template <typename RecordTypeByte, typename FormatType, typename ModuleType>
			class LibraryModuleLocationsRecord : public Record<RecordTypeByte, FormatType, ModuleType>
		{
		public:
			struct Location
			{
			public:
				/** @brief Offset to the first byte of the library module, as a block:byte pair */
				uint16_t block_number;
				/** @brief Offset to the first byte of the library module, as a block:byte pair */
				uint16_t byte_number;

				static Location Read(Linker::Reader& rd);
				void Write(ChecksumWriter& wr) const;
			};

			/** @brief List of starting locations, one for each library module */
			std::vector<Location> locations;

			LibraryModuleLocationsRecord(RecordTypeByte record_type = RecordTypeByte(0))
				: Record<RecordTypeByte, FormatType, ModuleType>(record_type)
			{
			}

			void ReadRecordContents(FormatType * omf, ModuleType * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(FormatType * omf, ModuleType * mod) const override;
			void WriteRecordContents(FormatType * omf, ModuleType * mod, ChecksumWriter& wr) const override;
		};

		/** @brief All public symbols of all modules, used for LIBDIC */
		template <typename RecordTypeByte, typename FormatType, typename ModuleType>
			class LibraryDictionaryRecord : public Record<RecordTypeByte, FormatType, ModuleType>
		{
		public:
			class Group
			{
			public:
				/** @brief List of public symbols for one library module */
				std::vector<std::string> names;

				void Read(Linker::Reader& rd);
				uint16_t Size() const;
				void Write(ChecksumWriter& wr) const;
			};

			/** @brief List of public symbols, grouped by library modules */
			std::vector<Group> groups;

			LibraryDictionaryRecord(RecordTypeByte record_type = RecordTypeByte(0))
				: Record<RecordTypeByte, FormatType, ModuleType>(record_type)
			{
			}

			void ReadRecordContents(FormatType * omf, ModuleType * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(FormatType * omf, ModuleType * mod) const override;
			void WriteRecordContents(FormatType * omf, ModuleType * mod, ChecksumWriter& wr) const override;
		};

		/** @brief Attempts to parse an OMF file, whether OMF80, OMF86, OMF51 or OMF96 */
		static std::shared_ptr<OMFFormat> ReadOMFFile(Linker::Reader& rd);

		/** @brief Used to calculate record offsets when generating OMF file */
		offset_t file_size = 0;
	};

	/**
	 * @brief Intel Relocatable Object Module format, used by various 16/32-bit DOS based compilers and linkers, including 16-bit Microsoft compilers, Borland and Watcom compilers
	 */
	class OMF86Format : public virtual OMFFormat, public virtual Linker::InputFormat
	{
	public:
		class Module;
		class SegmentDefinitionRecord;
		class GroupDefinitionRecord;

		/* * * General members * * */

		/** @brief Represents the various variants of the OMF file format, some of them are incompatible */
		enum omf_version_t
		{
			/** @brief The original Intel definition, version 4.0 (including any extensions that are compatible) */
			OMF_VERSION_INTEL_40 = 1,
			/** @brief Phar Lap's extensions, mostly concerning 32-bit (partly incompatible with later versions) */
			OMF_VERSION_PHARLAP,
			/** @brief Format generated by Microsoft's tools (partly incompatible with other versions) */
			OMF_VERSION_MICROSOFT,
			/** @brief Format generated by IBM's tools (partly incompatible with other versions) */
			OMF_VERSION_IBM,
			/** @brief Version 1.1 produced and consolidated by the Tool Interface Standard */
			OMF_VERSION_TIS_11,
		};

		/** @brief The variant of the OMF format, needed to handle certain fields */
		omf_version_t omf_version;

		OMF86Format(omf_version_t omf_version = OMF_VERSION_MICROSOFT)
			: omf_version(omf_version)
		{
		}

		/** @brief Flag binary or'ed to certain values to mark Intel interpretation */
		static constexpr unsigned int FlagIntel = 0x10;
		/** @brief Flag binary or'ed to certain values to mark Phar Lap interpretation */
		static constexpr unsigned int FlagPharLap = 0x20;
		/** @brief Flag binary or'ed to certain values to mark TIS interpretation */
		static constexpr unsigned int FlagTIS = 0x30;

		/** @brief Reference types for relocations, for targets and frames */
		enum
		{
			/** @brief References a segment defined in a module, for either targets or frames */
			MethodSegment = 0,
			/** @brief References a group defined in a module, for either targets or frames */
			MethodGroup = 1,
			/** @brief References a external symbol declared in a module, for either targets or frames */
			MethodExternal = 2,
			/** @brief References a physical paragraph address in a module, for either targets or frames */
			MethodFrame = 3,
			/** @brief References the frame of the relocation source, only for frames */
			MethodSource = 4,
			/** @brief References the frame of the relocation target, only for frames */
			MethodTarget = 5,
			/** @brief No frame is provided, intended for Intel 8089, only the Intel version supports it, only for frames */
			MethodAbsolute = 6,
		};

		/** @brief Represents a MethodFrame value */
		typedef uint16_t FrameNumber;

		/** @brief Represents a MethodSource value */
		struct UsesSource { };

		/** @brief Represents a MethodTarget value */
		struct UsesTarget { };

		/** @brief Represents a MethodAbsolute value */
		struct UsesAbsolute { };

		/** @brief A rich container for indexes referring to a LNAME/LLNAME declaration */
		class NameIndex
		{
		public:
			index_t index;
			std::string text;

			NameIndex(index_t index = 0)
				: index(index)
			{
			}

			void CalculateValues(OMF86Format * omf, Module * mod);
			void ResolveReferences(OMF86Format * omf, Module * mod);
		};

		/** @brief A rich container for indexes referring to a SEGDEF record */
		class SegmentIndex
		{
		public:
			index_t index;
			std::shared_ptr<SegmentDefinitionRecord> record;

			SegmentIndex(index_t index = 0)
				: index(index)
			{
			}

			void CalculateValues(OMF86Format * omf, Module * mod);
			void ResolveReferences(OMF86Format * omf, Module * mod);
		};

		/** @brief A rich container for indexes referring to a GRPDEF record */
		class GroupIndex
		{
		public:
			index_t index;
			std::shared_ptr<GroupDefinitionRecord> record;

			GroupIndex(index_t index = 0)
				: index(index)
			{
			}

			void CalculateValues(OMF86Format * omf, Module * mod);
			void ResolveReferences(OMF86Format * omf, Module * mod);
		};

		class TypeDefinitionRecord;
		class BlockDefinitionRecord;
		class OverlayDefinitionRecord;

		/** @brief A rich container for indexes referring to a TYPDEF record */
		class TypeIndex
		{
		public:
			index_t index;
			std::shared_ptr<TypeDefinitionRecord> record;

			TypeIndex(index_t index = 0)
				: index(index)
			{
			}

			void CalculateValues(OMF86Format * omf, Module * mod);
			void ResolveReferences(OMF86Format * omf, Module * mod);
		};

		/** @brief A rich container for indexes referring to a BLKDEF record */
		class BlockIndex
		{
		public:
			index_t index;
			std::shared_ptr<BlockDefinitionRecord> record;

			BlockIndex(index_t index = 0)
				: index(index)
			{
			}

			void CalculateValues(OMF86Format * omf, Module * mod);
			void ResolveReferences(OMF86Format * omf, Module * mod);
		};

		/** @brief A rich container for indexes referring to a OVLDEF record */
		class OverlayIndex
		{
		public:
			index_t index;
			std::shared_ptr<OverlayDefinitionRecord> record;

			OverlayIndex(index_t index = 0)
				: index(index)
			{
			}

			void CalculateValues(OMF86Format * omf, Module * mod);
			void ResolveReferences(OMF86Format * omf, Module * mod);
		};

		/** @brief An external name declaration, appearing in an EXTDEF, LEXTDEF, COMDEF, LCOMDEF or CEXTDEF record */
		class ExternalName
		{
		public:
			/** @brief Type of a common symbol */
			enum common_type_t : uint8_t
			{
				/** @brief Not a common symbol, appears in an EXTDEF, LEXTDEF or CEXTDEF record */
				External,

				/** @brief Borland segment index */
				SegmentIndexCommon,

				/** @brief Far variable */
				FarCommon = 0x61,

				/** @brief Near variable */
				NearCommon = 0x62,
			};

			/** @brief Signals if a symbol is local */
			bool local = false;
			/** @brief Set if name.index contains a valid value, for CEXTDEF entries */
			bool name_is_index = false;
			/** @brief Name of the symbol */
			NameIndex name;
			TypeIndex type;
			common_type_t common_type = External;

			union
			{
				uint8_t segment_index;
				struct
				{
					uint32_t length;
				} near;
				struct
				{
					uint32_t number;
					uint32_t element_size;
				} far;
			} value;

			ExternalName()
			{
				value.far.number = value.far.element_size = 0;
			}

			/** @brief Read a length value, for near or far elements */
			static uint32_t ReadValue(OMF86Format * omf, Linker::Reader& rd);
			/** @brief Determines the number of bytes needed for a length value, for near or far elements */
			static uint32_t ValueSize(OMF86Format * omf, uint32_t length);
			/** @brief Writes a length value, for near or far elements */
			static void WriteValue(OMF86Format * omf, ChecksumWriter& wr, uint32_t length);

			/** @brief Parse an EXTDEF or LEXTDEF entry */
			static ExternalName ReadExternalName(OMF86Format * omf, Linker::Reader& rd, bool local);
			/** @brief Parse a COMDEF or LCOMDEF entry */
			static ExternalName ReadCommonName(OMF86Format * omf, Linker::Reader& rd, bool local);
			/** @brief Parse a CEXTDEF entry */
			static ExternalName ReadComdatExternalName(OMF86Format * omf, Linker::Reader& rd);

			/** @brief Returns the number of bytes required to write this entry */
			uint16_t GetExternalNameSize(OMF86Format * omf) const;
			/** @brief Writes this entry to file */
			void WriteExternalName(OMF86Format * omf, ChecksumWriter& wr) const;

			void CalculateValues(OMF86Format * omf, Module * mod);
			void ResolveReferences(OMF86Format * omf, Module * mod);
		};

		/** @brief A rich container for indexes referring to symbols in an EXTDEF, LEXTDEF, COMDEF, LCOMDEF, CEXTDEF record */
		class ExternalIndex
		{
		public:
			index_t index;
			ExternalName name;

			ExternalIndex(index_t index = 0)
				: index(index)
			{
			}

			void CalculateValues(OMF86Format * omf, Module * mod);
			void ResolveReferences(OMF86Format * omf, Module * mod);
		};

		/** @brief Specification for the group and segment of some reference, and the frame if both are 0 */
		class BaseSpecification
		{
		public:
			/** @brief Type representing the group and segment of some reference */
			struct Location
			{
				GroupIndex group;
				SegmentIndex segment;
			};

			/** @brief The actual location of the reference */
			std::variant<Location, FrameNumber> location;

			void Read(OMF86Format * omf, Linker::Reader& rd);
			uint16_t Size(OMF86Format * omf) const;
			void Write(OMF86Format * omf, ChecksumWriter& wr) const;

			void CalculateValues(OMF86Format * omf, Module * mod);
			void ResolveReferences(OMF86Format * omf, Module * mod);
		};

		/** @brief Represents a symbol definition, used by SymbolDefinitionsRecord and DebugSymbolsRecord */
		class SymbolDefinition
		{
		public:
			/** @brief Set to true if the symbol is local */
			bool local;
			/** @brief The name of the symbol */
			std::string name;
			/** @brief The offset (value) of the symbol */
			uint32_t offset;
			/** @brief The type of the symbol, or 0 */
			TypeIndex type;

			static SymbolDefinition Read(OMF86Format * omf, Linker::Reader& rd, bool local, bool is32bit);
			uint16_t Size(OMF86Format * omf, bool is32bit) const;
			void Write(OMF86Format * omf, ChecksumWriter& wr, bool is32bit) const;

			void CalculateValues(OMF86Format * omf, Module * mod);
			void ResolveReferences(OMF86Format * omf, Module * mod);
		};

		/** @brief Represents a line number, used by LineNumbersRecord and SymbolLineNumbersRecord */
		class LineNumber
		{
		public:
			/** @brief The line number value */
			uint16_t number;
			/** @brief The offset in memory where the line starts */
			uint32_t offset;

			static LineNumber Read(OMF86Format * omf, Linker::Reader& rd, bool is32bit);
			void Write(OMF86Format * omf, ChecksumWriter& wr, bool is32bit) const;
		};

		/**
		 * @brief Represents a sequence of data, with optional repeated parts, used by RelocatableDataRecord, PhysicalDataRecord and LogicalDataRecord
		 *
		 * Data blocks are divided into two types: enumerated or iterated.
		 * Enumerated blocks are just a sequence of bytes.
		 * Iterated blocks consist of a collection of blocks that get repeated a number of times.
		 */
		class DataBlock
		{
		public:
			/** @brief Number of times the data should be appear. Should be 1 for enumerated data */
			uint16_t repeat_count;

			/** @brief If the contents are a sequence of blocks (iterated only) */
			typedef std::vector<std::shared_ptr<DataBlock>> Blocks;
			/** @brief If the contents are a sequence of bytes (either iterated or enumerated) */
			typedef std::vector<uint8_t> Data;

			/** @brief The actual contents of the block, whether iterated or enumerated */
			std::variant<Data, Blocks> content;

			/** @brief Parses the contents of an enumerated record */
			static std::shared_ptr<DataBlock> ReadEnumeratedDataBlock(OMF86Format * omf, Linker::Reader& rd, uint16_t data_length);
			/** @brief Gets the size of an encoded enumerated record in bytes */
			uint16_t GetEnumeratedDataBlockSize(OMF86Format * omf) const;
			/** @brief Writes the contents of an enumerated record into a file */
			void WriteEnumeratedDataBlock(OMF86Format * omf, ChecksumWriter& wr) const;

			/** @brief Parses the contents of an iterated record */
			static std::shared_ptr<DataBlock> ReadIteratedDataBlock(OMF86Format * omf, Linker::Reader& rd, bool is32bit);
			/** @brief Gets the size of an encoded iterated record in bytes */
			uint16_t GetIteratedDataBlockSize(OMF86Format * omf, bool is32bit) const;
			/** @brief Writes the contents of an iterated record into a file */
			void WriteIteratedDataBlock(OMF86Format * omf, ChecksumWriter& wr, bool is32bit) const;
		};

		/** @brief Represents a reference for a relocation */
		class Reference
		{
		public:
			/** @brief A number between 0 and 3 */
			typedef unsigned ThreadNumber;

			/** @brief The target for a reference, or a thread number */
			std::variant<ThreadNumber, SegmentIndex, GroupIndex, ExternalIndex, FrameNumber> target = ThreadNumber(0);
			/** @brief The frame for a reference, or a thread number */
			std::variant<ThreadNumber, SegmentIndex, GroupIndex, ExternalIndex, FrameNumber, UsesSource, UsesTarget, UsesAbsolute> frame = ThreadNumber(0);
			/** @brief Displacement to be added to the value */
			uint32_t displacement;

			void Read(OMF86Format * omf, Linker::Reader& rd, size_t displacement_size);
			uint16_t Size(OMF86Format * omf, bool is32bit) const;
			void Write(OMF86Format * omf, ChecksumWriter& wr, bool is32bit) const;

			void CalculateValues(OMF86Format * omf, Module * mod);
			void ResolveReferences(OMF86Format * omf, Module * mod);
		};

		/** @brief The recognized record types in an OMF86 file */
		enum record_type_t : uint8_t
		{
			RHEADR = 0x6E, // Intel 4.0
			REGINT = 0x70, // Intel 4.0
			REDATA = 0x72, // Intel 4.0
			RIDATA = 0x74, // Intel 4.0
			OVLDEF = 0x76, // Intel 4.0
			ENDREC = 0x78, // Intel 4.0
			BLKDEF = 0x7A, // Intel 4.0
			BLKEND = 0x7C, // Intel 4.0
			DEBSYM = 0x7E, // Intel 4.0
			THEADR = 0x80,
			LHEADR = 0x82,
			PEDATA = 0x84, // Intel 4.0
			PIDATA = 0x86, // Intel 4.0
			COMENT = 0x88,
			MODEND16 = 0x8A,
			MODEND = 0x8A,
			MODEND32 = 0x8B, // TIS 1.1
			EXTDEF = 0x8C,
			TYPDEF = 0x8E, // Intel 4.0
			PUBDEF16 = 0x90,
			PUBDEF = 0x90,
			PUBDEF32 = 0x91, // TIS 1.1
			LOCSYM = 0x92, // Intel 4.0
			LINNUM = 0x94, // Intel 4.0
			LNAMES = 0x96,
			SEGDEF = 0x98, // Intel 4.0
			GRPDEF = 0x9A, // Intel 4.0
			FIXUPP16 = 0x9C,
			FIXUPP = 0x9C,
			FIXUPP32 = 0x9D, // TIS 1.1
			LEDATA16 = 0xA0,
			LEDATA = 0xA0,
			LEDATA32 = 0xA1, // TIS 1.1
			LIDATA16 = 0xA2,
			LIDATA = 0xA2,
			LIDATA32 = 0xA3, // TIS 1.1
			LIBHED = 0xA4, // Intel 4.0
			LIBNAM = 0xA6, // Intel 4.0
			LIBLOC = 0xA8, // Intel 4.0
			LIBDIC = 0xAA, // Intel 4.0
			COMDEF = 0xB0, // TIS 1.1, since Microsoft LINK 3.5
			BAKPAT16 = 0xB2, // TIS 1.1, since Microsoft QuickC 1.0
			BAKPAT = 0xB2, // TIS 1.1
			BAKPAT32 = 0xB3, // TIS 1.1
			LEXTDEF16 = 0xB4, // TIS 1.1, since Microsoft C 5.0
			LEXTDEF = 0xB4, // TIS 1.1
			LEXTDEF32 = 0xB5, // TIS 1.1
			LPUBDEF16 = 0xB6, // TIS 1.1, since Microsoft C 5.0
			LPUBDEF = 0xB6, // TIS 1.1
			LPUBDEF32 = 0xB7, // TIS 1.1
			LCOMDEF = 0xB8, // TIS 1.1, since Microsoft C 5.0
			CEXTDEF = 0xBC, // TIS 1.1, since Microsoft C 7.0
			COMDAT16 = 0xC2, // TIS 1.1, since Microsoft C 7.0
			COMDAT = 0xC2, // TIS 1.1
			COMDAT32 = 0xC3, // TIS 1.1
			LINSYM16 = 0xC4, // TIS 1.1, since Microsoft C 7.0
			LINSYM = 0xC4, // TIS 1.1
			LINSYM32 = 0xC5, // TIS 1.1
			ALIAS = 0xC6, // TIS 1.1, since Microsoft LINK 5.13
			NBKPAT16 = 0xC8, // TIS 1.1, since Microsoft C 7.0
			NBKPAT = 0xC8, // TIS 1.1
			NBKPAT32 = 0xC9, // TIS 1.1
			LLNAMES = 0xCA, // TIS 1.1, since Microsoft C 7.0
			VERNUM = 0xCC, // TIS 1.1
			VENDEXT = 0xCE, // TIS 1.1
			LibraryHeader = 0xF0, // TIS 1.1
			LibraryEnd = 0xF1, // TIS 1.1
		};

		static const std::map<offset_t, std::string> RecordTypeNames;

		using Record = OMFFormat::Record<record_type_t, OMF86Format, Module>;
		using UnknownRecord = OMFFormat::UnknownRecord<record_type_t, OMF86Format, Module>;
		using EmptyRecord = OMFFormat::EmptyRecord<record_type_t, OMF86Format, Module>;
		using LibraryHeaderRecord = OMFFormat::LibraryHeaderRecord<record_type_t, OMF86Format, Module>;
		using LibraryModuleNamesRecord = OMFFormat::LibraryModuleNamesRecord<record_type_t, OMF86Format, Module>;
		using LibraryModuleLocationsRecord = OMFFormat::LibraryModuleLocationsRecord<record_type_t, OMF86Format, Module>;
		using LibraryDictionaryRecord = OMFFormat::LibraryDictionaryRecord<record_type_t, OMF86Format, Module>;

		/** @brief Parses and returns an instance of the next record */
		std::shared_ptr<Record> ReadRecord(Linker::Reader& rd);

		/** @brief A header record, used for THEADR and LHEADR */
		class ModuleHeaderRecord : public Record
		{
		public:
			/** @brief The name of the record */
			std::string name;

			ModuleHeaderRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		/** @brief A header record for an R-Module, used for RHEADR (not part of TIS OMF) */
		class RModuleHeaderRecord : public ModuleHeaderRecord
		{
		public:
			/** @brief The module object this record appears in */
			Module * mod;

			/** @brief Represents the type of the module */
			enum module_type_t : uint8_t
			{
				AbsoluteModule = 0,
				RelocatableModule = 1,
				PICModule = 2,
				LTLModule = 3,
			};

			/** @brief The type of the module */
			module_type_t module_type;
			/** @brief Number of segment definition records in module */
			uint16_t segment_record_count;
			/** @brief Number of group definition records in module */
			uint16_t group_record_count;
			/** @brief Number of overlay definition records in module */
			uint16_t overlay_record_count;
			/** @brief Offset to the first overlay definition record from the start of the file or 0 */
			uint32_t overlay_record_offset; // TODO: resolve record index

			/** @brief Total size of LTL segments */
			uint32_t static_size;
			/** @brief Maximum size needed for all LTL segments */
			uint32_t maximum_static_size;
			/** @brief Memory size to allocate on load */
			uint32_t dynamic_storage;
			/** @brief Maximum memory size to allocate on load */
			uint32_t maximum_dynamic_storage;

			RModuleHeaderRecord()
				: ModuleHeaderRecord()
			{
			}

			RModuleHeaderRecord(record_type_t record_type = record_type_t(0))
				: ModuleHeaderRecord(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		/** @brief A list of names to be referenced, used for LNAMES and LLNAMES records */
		class ListOfNamesRecord : public Record
		{
		public:
			/** @brief List of names (only needed for genering OMF files */
			std::vector<std::string> names;
			/** @brief Index of the first name in the module */
			index_t first_lname;
			/** @brief Number of names appearing in this record */
			uint16_t lname_count = 0;

			ListOfNamesRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief A record that defines a segment, used for SEGDEF records */
		class SegmentDefinitionRecord : public Record, public std::enable_shared_from_this<SegmentDefinitionRecord>
		{
		public:
			/** @brief The length of the segment (maximum 0x100000000) */
			uint64_t segment_length;
			/** @brief Alignment type for a segment */
			enum alignment_t : uint8_t
			{
				/** @brief Absolute segment, its position in memory is fixed */
				AlignAbsolute = 0,
				/** @brief Relocatable segment, byte aligned */
				Align1 = 1,
				/** @brief Relocatable segment, word (2-byte) aligned */
				Align2 = 2,
				/** @brief Relocatable segment, paragraph (16-byte) aligned */
				Align16 = 3,
				/** @brief Relocatable segment, page aligned, exact size depends on OMF version */
				AlignPage = 4,
				/** @brief Load-time locatable or, if not part of a group, paragraph (16-byte) aligned (Intel only) */
				AlignLTL16 = 6,

				/** @brief Relocatable segment, page (256-byte) aligned, as defined by Intel */
				Align256 = FlagIntel | AlignPage,
				/** @brief Relocatable segment, page (4096-byte) aligned, as used by IBM */
				Align4096 = FlagTIS | AlignPage,
				/** @brief Unnamed absolute portion of the memory address space, type 5 as defined by Intel */
				AlignUnnamed = FlagIntel | 5,
				/** @brief Relocatable segment, double word (4-byte) aligned, type 5 as defined by TIS */
				Align32 = FlagTIS | 5,
			};

			/** @brief Alignment of segment */
			alignment_t alignment;

			/** @brief Additional data used for absolute segments, type 0 or 5 if Intel version is used */
			typedef uint32_t Absolute;

			/** @brief Type representing a relocatable segment */
			struct Relocatable { };

			/** @brief Additional data for LTL segment, not part of TIS */
			class LoadTimeLocatable
			{
			public:
				bool group_member;
				uint16_t maximum_segment_length;
				uint16_t group_offset;
			};

			/** @brief Information on where the segment should be placed at runtime */
			std::variant<Relocatable, Absolute, LoadTimeLocatable> location = Relocatable{};

			/** @brief Describes how two segments of the same name and class name should be combined */
			enum combination_t : uint8_t
			{
				/** @brief Do not combine segments, also known as Private */
				Combination_Private = 0,
				/** @brief Like Combination_Merge_High, but also places the segment above all other segments (not used by TIS) */
				Combination_Merge_Highest = 1,
				/** @brief Appends the segments, also known as Public */
				Combination_Append = 2,
				Combination_Public = Combination_Append,
				/** @brief Merges the two segments at their lowest address, adds their sizes (Intel interpretation) */
				Combination_Join_Low = 4,
				Combination_Public4 = FlagTIS | 4,
				/** @brief Merges the two segments at their highest address, adds their sizes (Intel interpretation) */
				Combination_Join_High = FlagIntel | 5,
				/** @brief Same as Public, but for stack segments */
				Combination_Stack = FlagTIS | 5,
				/** @brief Merges the two segments at their lowest address, their size is the maximum (Intel interpretation) */
				Combination_Merge_Low = FlagIntel | 6,
				Combination_Common = FlagTIS | 6,
				/** @brief Merges the two segments at their highest address, their size is the maximum (Intel interpretation) */
				Combination_Merge_High = 7,
				Combination_Public7 = FlagTIS | 4,
			};

			/** @brief How segments of the same type should be combined */
			combination_t combination;

			/** @brief Set if the segment should not cross a 256 byte page boundary (only Intel) */
			bool page_resident = false; // Intel
			/** @brief Set if the USE32 directive is used for the segment. The field appears in different places for Phar Lap and TIS */
			bool use32 = false;

			/** @brief The name of the segment (not used for AlignUnnamed) */
			NameIndex segment_name;
			/** @brief The name of the segment's class (not used for AlignUnnamed) */
			NameIndex class_name;
			/** @brief The name of the overlay the segment appears in (not used for AlignUnnamed) */
			NameIndex overlay_name;

			/** @brief Represents an additional field used by Phar Lap */
			enum access_t : uint8_t
			{
				AccessReadOnly = 0,
				AccessExecuteOnly = 1,
				AccessExecuteRead = 2,
				AccessReadWrite = 3,
			};

			/** @brief Represents the value of a Phar Lap specific extension word */
			std::optional<access_t> access;

			SegmentDefinitionRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief A record that defines a segment group, used for GRPDEF records */
		class GroupDefinitionRecord : public Record, public std::enable_shared_from_this<GroupDefinitionRecord>
		{
		public:
			/** @brief Represents a component of the group */
			class Component
			{
			public:
				/** @brief Represents the data required for identifying a the segment by the name, class name and overlay name (Intel only) */
				class SegmentClassOverlayNames
				{
				public:
					NameIndex segment_name;
					NameIndex class_name;
					NameIndex overlay_name;
				};

				/** @brief Represents the data required for a load-time locatable group (Intel only) */
				class LoadTimeLocatable
				{
				public:
					uint32_t maximum_group_length;
					uint32_t group_length;
				};

				/** @brief Represents the data required for an absolute group (Intel only) */
				typedef uint32_t Absolute;

				/** @brief The type prefix for the address of the group (Intel only) */
				static constexpr uint8_t Absolute_type = 0xFA;
				/** @brief The type prefix for a load time locatable group (Intel only) */
				static constexpr uint8_t LoadTimeLocatable_type = 0xFB;
				/** @brief The type prefix for segment, class, overlay name triplets (Intel only) */
				static constexpr uint8_t SegmentClassOverlayNames_type = 0xFD;
				/** @brief The type prefix for the segment of an external symbol (Intel only) */
				static constexpr uint8_t ExternalIndex_type = 0xFE;
				/** @brief The type prefix for a segment (the only type defined by TIS) */
				static constexpr uint8_t SegmentIndex_type = 0xFF;

				/** @brief The contents of the component */
				std::variant<ExternalIndex, SegmentIndex, SegmentClassOverlayNames, LoadTimeLocatable, Absolute> component = ExternalIndex();

				static Component Read(OMF86Format * omf, Module * mod, Linker::Reader& rd);
				uint16_t Size(OMF86Format * omf, Module * mod) const;
				void Write(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const;

				void CalculateValues(OMF86Format * omf, Module * mod);
				void ResolveReferences(OMF86Format * omf, Module * mod);
			};

			/** @brief Name of the group */
			NameIndex name;
			/** @brief The components of the group, such as member segments or load address */
			std::vector<Component> components;

			GroupDefinitionRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/**
		 * @brief A record that defines a data type, used for TYPDEF records
		 *
		 * Intel defines types as a sequence of leaf descriptors, each element of a leaf encoding an integer, a type index, a string etc.
		 * In the OMF file they are stored as groups of 8, but what is relevant is that it is a sequence of items.
		 * The first entry is usually a number that represents what kind of type this is, and the following items describe the parameters to this type.
		 */
		class TypeDefinitionRecord : public Record, public std::enable_shared_from_this<TypeDefinitionRecord>
		{
		public:
			/** @brief A single item in a type definition */
			class LeafDescriptor
			{
			public:
				/** @brief Set to false if "easy", true if "nice" */
				bool nice;

				/** @brief Represents a null leaf */
				struct Null { };
				/** @brief Represents a final repeat leaf, meaning that the data in this type should be repeated indefinitely */
				struct Repeat { };

				/** @brief The actual contents of the leaf */
				std::variant<uint32_t, int32_t, std::string, TypeIndex, Null, Repeat> leaf = Null();

				/** @brief Type byte for a null leaf */
				static constexpr uint8_t NullLeaf = 0x80;
				/** @brief Type byte for a string leaf */
				static constexpr uint8_t StringLeaf = 0x82;
				/** @brief Type byte for an index leaf, usually a type index */
				static constexpr uint8_t IndexLeaf = 0x83;
				/** @brief Type byte for a repeat leaf leaf */
				static constexpr uint8_t RepeatLeaf = 0x85;
				/** @brief Type byte for an unsigned 16-bit leaf */
				static constexpr uint8_t NumericLeaf16 = 0x81;
				/** @brief Type byte for an unsigned 24-bit leaf */
				static constexpr uint8_t NumericLeaf24 = 0x84;
				/** @brief Type byte for an signed 8-bit leaf */
				static constexpr uint8_t SignedNumericLeaf8 = 0x86;
				/** @brief Type byte for an signed 16-bit leaf */
				static constexpr uint8_t SignedNumericLeaf16 = 0x87;
				/** @brief Type byte for an signed 32-bit leaf */
				static constexpr uint8_t SignedNumericLeaf32 = 0x88;

				static LeafDescriptor Read(OMF86Format * omf, Module * mod, Linker::Reader& rd);
				uint16_t Size(OMF86Format * omf, Module * mod) const;
				void Write(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const;

				void CalculateValues(OMF86Format * omf, Module * mod);
				void ResolveReferences(OMF86Format * omf, Module * mod);
			};

			/** @brief The name of this type */
			std::string name;
			/** @brief The parameters of this type */
			std::vector<LeafDescriptor> leafs;

			enum
			{
				Far = 0x61, // Far(Array(...)) - Microsoft
				Near = 0x62, // Near(Array | Structure | Scalar, length in bits) - Microsoft
				Interrupt = 0x63,
				File = 0x64,
				Packed = 0x65,
				Unpacked = 0x66,
				Set = 0x67,
				Chameleon = 0x69,
				Boolean = 0x6A,
				True = 0x6B,
				False = 0x6C,
				Char = 0x6D,
				Integer = 0x6E,
				Const = 0x6F,
				Label = 0x71, // Label(nil, Short | Long)
				Long = 0x72,
				Short = 0x73,
				Procedure = 0x74, // Procedure(nil, @type, Short | Long, parameter count, @List(...))
				Parameter = 0x75, // Parameter(@type)
				Dimension = 0x76,
				Array = 0x77, // Array(length, @type)
				Structure = 0x79, // Structure(length, component count, @List(...))
				Pointer = 0x7A,
				Scalar = 0x7B, // Scalar(length, UnsignedInteger | SignedInteger | Real | @Pointer)
				UnsignedInteger = 0x7C,
				SignedInteger = 0x7D,
				Real = 0x7E,
				List = 0x7F, // List(...)
			};

			TypeDefinitionRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief A record that defines a symbol, used by PUBDEF, LPUBDEF, LOCSYM */
		class SymbolsDefinitionRecord : public Record
		{
		public:
			/** @brief The first byte of the segment containing the symbol */
			BaseSpecification base;

			/** @brief A list of symbols */
			std::vector<SymbolDefinition> symbols;

			SymbolsDefinitionRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief A record defining several external or common symbols, used for EXTDEF, LEXTDEF, CEXTDEF, COMDEF and LCOMDEF */
		class ExternalNamesDefinitionRecord : public Record
		{
		public:
			/** @brief Index of first external or common symbol in module */
			ExternalIndex first_extdef;
			/** @brief Number of external or common symbols defined in this record */
			uint16_t extdef_count = 0;

			ExternalNamesDefinitionRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief Stores as debugging data the line number information, used for LINNUM */
		class LineNumbersRecord : public Record
		{
		public:
			/** @brief The first byte of the segment containing the line */
			BaseSpecification base;

			/** @brief List of lines */
			std::vector<LineNumber> lines;

			LineNumbersRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief A record that defines a block, used for BLKDEF records (Intel only) */
		class BlockDefinitionRecord : public Record, public std::enable_shared_from_this<BlockDefinitionRecord>
		{
		public:
			/** @brief The first byte of the segment containing the block */
			BaseSpecification base;
			/** @brief The name of the block or empty */
			std::string name;
			/** @brief Offset to the first byte of the block */
			uint16_t offset;
			/** @brief Length of the block */
			uint16_t length;
			/** @brief Type of the block, if it is a procedure */
			enum block_type_t : uint8_t
			{
				/** @brief Block is not a procedure */
				NotProcedure = 0x00,
				/** @brief Block is a near procedure */
				NearProcedure = 0x80,
				/** @brief Block is a far procedure */
				FarProcedure = 0xC0,
			};
			/** @brief Whether this block is a procedure, and whether it is a near or far procedure */
			block_type_t procedure;
			/** @brief Offset, relative to the frame pointer (BP), to the return address of the procedure in the calling frame on the stack */
			uint16_t return_address_offset; // only for procedures
			/** @brief Type of block or procedure, only used if name != "" */
			TypeIndex type;

			BlockDefinitionRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief Record to store debug symbols, used for DEBSYM (Intel only) */
		class DebugSymbolsRecord : public Record
		{
		public:
			enum frame_type_t : uint8_t
			{
				Location = 0x00,
				Frame16 = 0x80,
				Frame32 = 0xC0,
			};
			frame_type_t frame_type;

			static constexpr uint8_t SymbolBaseMethod = 0x00;
			static constexpr uint8_t ExternalMethod = 0x01;
			static constexpr uint8_t BlockMethod = 0x02;

			std::variant<BaseSpecification, ExternalIndex, BlockIndex> base = ExternalIndex();

			std::vector<SymbolDefinition> names;

			DebugSymbolsRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief Relocatable enumerated or iterated data, used for REDATA and RIDATA (Intel only) */
		class RelocatableDataRecord : public Record
		{
		public:
			/** @brief Reference to paragraph that contains this data */
			BaseSpecification base;
			/** @brief Offset within paragraph where data starts */
			uint16_t offset;
			/** @brief The data contents */
			std::shared_ptr<DataBlock> data;

			RelocatableDataRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief Physical enumerated or iterated data, used for PEDATA and PIDATA (Intel only) */
		class PhysicalDataRecord : public Record
		{
		public:
			/** @brief Physical address where data starts */
			uint32_t address;
			/** @brief The data contents */
			std::shared_ptr<DataBlock> data;

			PhysicalDataRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		/** @brief Logical enumerated or iterated data, used for LEDATA and LIDATA */
		class LogicalDataRecord : public Record
		{
		public:
			/** @brief Segment that contains this data */
			SegmentIndex segment;
			/** @brief Offset within segment where the data starts */
			uint32_t offset;
			/** @brief The data contents */
			std::shared_ptr<DataBlock> data;

			LogicalDataRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief A record containing relocation data, used for FIXUPP */
		class FixupRecord : public Record
		{
		public:
			/** @brief Threads are intermediary storage for relocation targets and frames, this class represents a thread assignment */
			class Thread
			{
			public:
				/** @brief The thread number which gets assigned to, a value from 0 to 3 */
				unsigned thread_number;
				/** @brief Set if this is a frame thread, it is a target thread otherwise */
				bool frame;
				/** @brief The frame or target reference */
				std::variant<SegmentIndex, GroupIndex, ExternalIndex, FrameNumber, UsesSource, UsesTarget, UsesAbsolute> reference = FrameNumber(0);

				static Thread Read(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint8_t leading_data_byte);
				uint16_t Size(OMF86Format * omf, Module * mod) const;
				void Write(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const;

				void CalculateValues(OMF86Format * omf, Module * mod);
				void ResolveReferences(OMF86Format * omf, Module * mod);
			};

			class Fixup
			{
			public:
				/** @brief Set if segment relative, self relative otherwise */
				bool segment_relative;

				/** @brief Types for the value that needs to be relocated */
				enum relocation_type_t
				{
					/** @brief Least significant 8 bits of value */
					RelocationLowByte = 0,
					/** @brief A 16-bit value or the 16-bit offset portion of a far address */
					RelocationOffset16 = 1,
					/** @brief The segment portion of a far address */
					RelocationSegment = 2,
					/** @brief A 32-bit far address, consisting of a 16-bit segment portion and a 16-bit offset portion */
					RelocationPointer32 = 3,
					/** @brief Most significant 8 bits of a 16-bit value */
					RelocationHighByte = 4,
					/** @brief A 16-bit loader-resolved value (TIS assignment) */
					RelocationOffset16_LoaderResolved = FlagTIS | 5,
					/** @brief A 32-bit value or the 32-bit offset portion of a far address (Phar Lap assignment) */
					RelocationOffset32_PharLap = FlagPharLap | 5,
					/** @brief A 48-bit far address, consisting of a 16-bit segment portion and a 32-bit offset portion (Phar Lap assignment) */
					RelocationPointer48_PharLap = 6,
					/** @brief A 32-bit value or the 32-bit offset portion of a far address (TIS assignment) */
					RelocationOffset32 = 9,
					/** @brief A 48-bit far address, consisting of a 16-bit segment portion and a 32-bit offset portion (TIS assignment) */
					RelocationPointer48 = 11,
					/** @brief A 32-bit loader-resolved value (TIS assignment) */
					RelocationOffset32_LoaderResolved = 13,
				};

				/** @brief The type of the relocation value */
				relocation_type_t type;
				/** @brief Offset to value to be relocated, relative to the latest data block */
				uint16_t offset;
				/** @brief Reference of this relocation */
				Reference ref;

				static Fixup Read(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint8_t leading_data_byte, bool is32bit);
				uint16_t Size(OMF86Format * omf, Module * mod, bool is32bit) const;
				void Write(OMF86Format * omf, Module * mod, ChecksumWriter& wr, bool is32bit) const;

				void CalculateValues(OMF86Format * omf, Module * mod);
				void ResolveReferences(OMF86Format * omf, Module * mod);
			};

			/** @brief Sequence of thread assignments and relocations */
			std::vector<std::variant<Thread, Fixup>> fixup_data;

			FixupRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief A record that defines an overlay, used for OVLDEF records */
		class OverlayDefinitionRecord : public Record, public std::enable_shared_from_this<OverlayDefinitionRecord>
		{
		public:
			/** @brief Name of the overlay */
			std::string name;
			/** @brief Offset to the first overlay data record within the file */
			uint32_t location;
			/** @brief Index of the first overlay data record within the file */
			size_t first_data_record_index;
			/** @brief Must be loaded at the same location as the shared overlay */
			std::optional<OverlayIndex> shared_overlay;
			/** @brief Must be loaded adjacent to the adjacent overlay */
			std::optional<OverlayIndex> adjacent_overlay;

			OverlayDefinitionRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief Record to signal termination of an overloay or block, used for ENDBLK (Intel only) */
		class EndRecord : public Record
		{
		public:
			/** @brief Type of block that is terminated by an EndRecord */
			enum block_type_t : uint8_t
			{
				Overlay = 0,
				Block = 1,
			};
			/** @brief Type of block this record terminates */
			block_type_t block_type;

			EndRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		/** @brief Initialization values for registers, used for REGINT (Intel only) */
		class RegisterInitializationRecord : public Record
		{
		public:
			/** @brief A single initialization entry */
			class Register
			{
			public:
				/** @brief Register type that can be initialized */
				enum register_t : uint8_t
				{
					/** @brief The CS and IP register pair, it signifies the starting address as a far pointer */
					CS_IP,
					/** @brief The SS and SP register pair, it signifies the initial stack as a far pointer */
					SS_SP,
					/** @brief The DS segment register, the default data segment */
					DS,
					/** @brief The ES segment register */
					ES,
				};

				/** @brief Used to initialize the register */
				class InitialValue
				{
				public:
					/** @brief The segment base */
					BaseSpecification base;
					/** @brief An offset, needed for CS:IP and SS:SP */
					uint16_t offset;
				};
				/** @brief The register to initialize */
				register_t reg_id;
				/** @brief The initial value of the register */
				std::variant<Reference, InitialValue> value = Reference();

				static Register Read(OMF86Format * omf, Module * mod, Linker::Reader& rd);
				uint16_t Size(OMF86Format * omf, Module * mod) const;
				void Write(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const;

				void CalculateValues(OMF86Format * omf, Module * mod);
				void ResolveReferences(OMF86Format * omf, Module * mod);
			};

			/** @brief List of registers to be initialized */
			std::vector<Register> registers;

			RegisterInitializationRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief Terminates a module, used for MODEND */
		class ModuleEndRecord : public Record
		{
		public:
			/** @brief Set if this is a main module */
			bool main_module;
			/** @brief Optional starting address, either a reference, or a segment:offset pair */
			std::optional<std::variant<Reference, std::tuple<uint16_t, uint16_t>>> start_address;

			ModuleEndRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		/** @brief A specialized relocation, used for BAKPAT (included in TIS) */
		class BackpatchRecord : public Record
		{
		public:
			/** @brief Represents a single fixup */
			class OffsetValuePair
			{
			public:
				/** @brief The offset at which the fixup should happen */
				uint32_t offset;
				/** @brief The value to be added to the fixup location */
				uint32_t value;
			};

			/** @brief Segment to which the relocations belong */
			SegmentIndex segment;
			/** @brief Relocation type */
			enum location_type_t : uint8_t
			{
				Location8bit = 0,
				Location16bit = 1,
				Location32bit = 2,
				Location32bit_IBM = 9,
			};
			/** @brief The type of the relocations */
			uint8_t type;
			/** @brief The data for the relocations */
			std::vector<OffsetValuePair> offset_value_pairs;

			BackpatchRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief A specialized relocation for named common segments, used for NBKPAT (included in TIS) */
		class NamedBackpatchRecord : public Record
		{
		public:
			using OffsetValuePair = BackpatchRecord::OffsetValuePair;
			using location_type_t = BackpatchRecord::location_type_t;

			/** @brief The type of the relocations */
			uint8_t type;
			/** @brief The name of the common segment to which the relocations belong to */
			NameIndex name;
			std::vector<OffsetValuePair> offset_value_pairs;

			NamedBackpatchRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief Initialization data for common segments, used for COMDAT (included in TIS) */
		class InitializedCommunalDataRecord : public Record
		{
		public:
			/** @brief Set if this record is the continuation of a previous record, cleared if it is a new record */
			bool continued;
			/** @brief Set if the format of the data is iterated, cleared for enumerated */
			bool iterated;
			/** @brief Local common segment */
			bool local;
			/** @brief Set if the data is to be placed in a code segment */
			bool code_segment;

			// TODO: document further

			enum selection_criterion_t : uint8_t
			{
				SelectNoMatch = 0x00,
				SelectPickAny = 0x10,
				SelectSameSize = 0x20,
				SelectExactMatch = 0x30,
			};
			static constexpr uint8_t SelectionCriterionMask = 0xF0;
			selection_criterion_t selection_criterion;

			enum allocation_type_t : uint8_t
			{
				AllocateExplicit = 0x00,
				AllocateFarCode = 0x01,
				AllocateFarData = 0x02,
				AllocateCode32 = 0x03,
				AllocateData32 = 0x04,
			};
			static constexpr uint8_t AllocationTypeMask = 0x0F;
			allocation_type_t allocation_type;

			SegmentDefinitionRecord::alignment_t alignment;
			static constexpr SegmentDefinitionRecord::alignment_t UseSegment = SegmentDefinitionRecord::alignment_t(0);

			uint32_t offset;
			TypeIndex type;
			BaseSpecification base;
			NameIndex name;
			std::shared_ptr<DataBlock> data;

			InitializedCommunalDataRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief Line numbers for common segments, used by LINSYM (included in TIS) */
		class SymbolLineNumbersRecord : public Record
		{
		public:
			/** @brief Set if this record is the continuation of a previous record, cleared if it is a new record */
			bool continued;
			/** @brief The name of the common segment to which the line numbers belong to */
			NameIndex name;
			/** @brief The line numbers */
			std::vector<LineNumber> line_numbers;

			SymbolLineNumbersRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		/** @brief Record defining a replacement name for symbols, used for ALIAS (included in TIS) */
		class AliasDefinitionRecord : public Record
		{
		public:
			/** @brief Represents a single substitution */
			class AliasDefinition
			{
			public:
				/** @brief Name used in references */
				std::string alias_name;
				/** @brief Name it should be substituted with */
				std::string substitute_name;

				static AliasDefinition Read(OMF86Format * omf, Module * mod, Linker::Reader& rd);
				uint16_t Size(OMF86Format * omf, Module * mod) const;
				void Write(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const;
			};
			/** @brief List of replacements */
			std::vector<AliasDefinition> alias_definitions;

			AliasDefinitionRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		/** @brief The version of the OMF format, used for VERNUM (included in TIS) */
		class OMFVersionNumberRecord : public Record
		{
		public:
			/** @brief Textual representation of the version number */
			std::string version;

			OMFVersionNumberRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		/** @brief Vendor specific extension to the OMF format, used for VENDEXT (included in TIS) */
		class VendorExtensionRecord : public Record
		{
		public:
			/** @brief Assigned vendor identifier */
			uint16_t vendor_number;
			/** @brief Extension data */
			std::vector<uint8_t> extension;

			VendorExtensionRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		// TODO: document

		class CommentRecord : public Record
		{
		public:
			bool no_purge;
			bool no_list;
			enum comment_class_t : uint8_t
			{
				Translator = 0x00,
				IntelCopyright = 0x01,
				LibrarySpecifier = 0x81,
				MSDOSVersion = 0x9C,
				MemoryModel = 0x9D,
				DOSSEG = 0x9E,
				DefaultLibrarySearchName = 0x9F,
				OMFExtension = 0xA0,
				NewOMFExtension = 0xA1,
				LinkPassSeparator = 0xA2,
				LIBMOD = 0xA3,
				EXESTR = 0xA4,
				INCERR = 0xA6,
				NOPAD = 0xA7,
				WKEXT = 0xA8,
				LZEXT = 0xA9,
				Comment = 0xDA,
				Compiler = 0xDB,
				Date = 0xDC,
				TimeStamp = 0xDD,
				User = 0xDF,
				DependencyFile = 0xE9,
				CommandLine = 0xFF,
			};
			comment_class_t comment_class;

			CommentRecord()
				: Record(COMENT)
			{
			}

			CommentRecord(comment_class_t comment_class = comment_class_t(0))
				: Record(COMENT), comment_class(comment_class)
			{
			}

			virtual void ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length) = 0;
			virtual uint16_t GetCommentSize(OMF86Format * omf, Module * mod) const = 0;
			virtual void WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const = 0;

			static std::shared_ptr<CommentRecord> ReadCommentRecord(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t record_length);
			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			class GenericCommentRecord;
			class EmptyCommentRecord;
			class TextCommentRecord;
		};

		class CommentRecord::GenericCommentRecord : public CommentRecord
		{
		public:
			std::vector<uint8_t> data;

			GenericCommentRecord(comment_class_t comment_class = comment_class_t(0))
				: CommentRecord(comment_class)
			{
			}

			void ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length) override;
			uint16_t GetCommentSize(OMF86Format * omf, Module * mod) const override;
			void WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		class CommentRecord::EmptyCommentRecord : public CommentRecord
		{
		public:
			EmptyCommentRecord(comment_class_t comment_class = comment_class_t(0))
				: CommentRecord(comment_class)
			{
			}

			void ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length) override;
			uint16_t GetCommentSize(OMF86Format * omf, Module * mod) const override;
			void WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		class CommentRecord::TextCommentRecord : public CommentRecord
		{
		public:
			std::string name;

			TextCommentRecord(comment_class_t comment_class = comment_class_t(0))
				: CommentRecord(comment_class)
			{
			}

			void ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length) override;
			uint16_t GetCommentSize(OMF86Format * omf, Module * mod) const override;
			void WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		class NoSegmentPaddingRecord : public CommentRecord
		{
		public:
			std::vector<SegmentIndex> segments;

			NoSegmentPaddingRecord()
				: CommentRecord(NOPAD)
			{
			}

			void ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length) override;
			uint16_t GetCommentSize(OMF86Format * omf, Module * mod) const override;
			void WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		class ExternalAssociationRecord : public CommentRecord
		{
		public:
			class ExternalAssociation
			{
			public:
				ExternalIndex definition;
				ExternalIndex default_resolution;

				static ExternalAssociation Read(OMF86Format * omf, Module * mod, Linker::Reader& rd);
				uint16_t Size(OMF86Format * omf, Module * mod) const;
				void Write(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const;

				void CalculateValues(OMF86Format * omf, Module * mod);
				void ResolveReferences(OMF86Format * omf, Module * mod);
			};

			std::vector<ExternalAssociation> associations;

			ExternalAssociationRecord(comment_class_t comment_class = comment_class_t(0))
				: CommentRecord(comment_class)
			{
			}

			void ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length) override;
			uint16_t GetCommentSize(OMF86Format * omf, Module * mod) const override;
			void WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF86Format * omf, Module * mod) override;
			void ResolveReferences(OMF86Format * omf, Module * mod) override;
		};

		class OMFExtensionRecord : public CommentRecord
		{
		public:
			enum extension_type_t : uint8_t
			{
				IMPDEF = 0x01,
				EXPDEF = 0x02,
				INCDEF = 0x03,
				ProtectedModeLibrary = 0x04,
				LNKDIR = 0x05,
				BigEndian = 0x06,
				PRECOMP = 0x07,
			};
			extension_type_t subtype;

			OMFExtensionRecord(extension_type_t subtype = extension_type_t(0))
				: CommentRecord(OMFExtension), subtype(subtype)
			{
			}

			class GenericOMFExtensionRecord;
			class EmptyOMFExtensionRecord;
		};

		class OMFExtensionRecord::GenericOMFExtensionRecord : public OMFExtensionRecord
		{
		public:
			std::vector<uint8_t> data;

			GenericOMFExtensionRecord(extension_type_t extension_type = extension_type_t(0))
				: OMFExtensionRecord(extension_type)
			{
			}

			void ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length) override;
			uint16_t GetCommentSize(OMF86Format * omf, Module * mod) const override;
			void WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		class OMFExtensionRecord::EmptyOMFExtensionRecord : public OMFExtensionRecord
		{
		public:
			EmptyOMFExtensionRecord(extension_type_t extension_type = extension_type_t(0))
				: OMFExtensionRecord(extension_type)
			{
			}

			void ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length) override;
			uint16_t GetCommentSize(OMF86Format * omf, Module * mod) const override;
			void WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		class ImportDefinitionRecord : public OMFExtensionRecord
		{
		public:
			std::string internal_name;
			std::string module_name;
			std::variant<std::string, uint16_t> entry_ident;

			ImportDefinitionRecord()
				: OMFExtensionRecord(IMPDEF)
			{
			}

			void ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length) override;
			uint16_t GetCommentSize(OMF86Format * omf, Module * mod) const override;
			void WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		class ExportDefinitionRecord : public OMFExtensionRecord
		{
		public:
			bool resident_name;
			bool no_data;
			uint8_t parameter_count;
			std::string exported_name;
			std::string internal_name;
			std::optional<uint16_t> ordinal;

			ExportDefinitionRecord()
				: OMFExtensionRecord(EXPDEF)
			{
			}

			void ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length) override;
			uint16_t GetCommentSize(OMF86Format * omf, Module * mod) const override;
			void WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		class IncrementalCompilationRecord : public OMFExtensionRecord
		{
		public:
			uint16_t extdef_delta;
			uint16_t linnum_delta;
			uint16_t padding_byte_count;

			IncrementalCompilationRecord()
				: OMFExtensionRecord(INCDEF)
			{
			}

			void ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length) override;
			uint16_t GetCommentSize(OMF86Format * omf, Module * mod) const override;
			void WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		class LinkerDirectivesRecord : public OMFExtensionRecord
		{
		public:
			bool new_executable;
			bool omit_codeview_publics;
			bool run_mpc;
			uint8_t pseudocode_version;
			uint8_t codeview_version;

			static constexpr uint8_t FlagNewExecutable = 0x01;
			static constexpr uint8_t FlagOmitCodeViewPublics = 0x02;
			static constexpr uint8_t FlagRunMPC = 0x04;

			LinkerDirectivesRecord()
				: OMFExtensionRecord(LNKDIR)
			{
			}

			void ReadComment(OMF86Format * omf, Module * mod, Linker::Reader& rd, uint16_t comment_length) override;
			uint16_t GetCommentSize(OMF86Format * omf, Module * mod) const override;
			void WriteComment(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		class TISLibraryHeaderRecord : public Record
		{
		public:
			uint32_t dictionary_offset;
			uint16_t dictionary_size;
			bool case_sensitive;

			TISLibraryHeaderRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void WriteRecord(OMF86Format * omf, Module * mod, Linker::Writer& wr) const override;
		};

		class TISLibraryEndRecord : public Record
		{
		public:
			TISLibraryEndRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF86Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF86Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF86Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void WriteRecord(OMF86Format * omf, Module * mod, Linker::Writer& wr) const override;
		};

		/** @brief Represents a single module inside the OMF file */
		class Module
		{
		public:
			/** @brief The index of the first record inside the file, stored in the OMF86Format objects */
			size_t first_record = 0;
			/** @brief The number of records belonging to this module */
			size_t record_count = 0;

			/** @brief List of names declared in the module */
			std::vector<std::string> lnames;
			/** @brief List of segments declared in the module */
			std::vector<std::shared_ptr<SegmentDefinitionRecord>> segdefs;
			/** @brief List of groups declared in the module */
			std::vector<std::shared_ptr<GroupDefinitionRecord>> grpdefs;
			/** @brief List of types declared in the module */
			std::vector<std::shared_ptr<TypeDefinitionRecord>> typdefs;
			/** @brief List of blocks declared in the module */
			std::vector<std::shared_ptr<BlockDefinitionRecord>> blkdefs;
			/** @brief List of overlays declared in the module */
			std::vector<std::shared_ptr<OverlayDefinitionRecord>> ovldefs;
			/** @brief List of external symbols declared in the module */
			std::vector<ExternalName> extdefs; // also includes comdefs and cextdefs
			// TODO: CalculateValue/etc. for ExternalName
		};

		/** @brief The ordered collection of records contained in the file */
		std::vector<std::shared_ptr<Record>> records;

		/** @brief List of modules appearing in an OMF file, typically only one for an object file */
		std::vector<Module> modules;

		// TIS library fields
		uint16_t page_size;

		/** @brief Parses an OMF86 file */
		static std::shared_ptr<OMF86Format> ReadOMFFile(Linker::Reader& rd);

		static void DumpAddFields(const Record * record, Dumper::Dumper& dump, Dumper::Region& region, const OMF86Format * omf, const Module * mod, size_t record_index);

		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
		using Linker::InputFormat::GenerateModule;
		void GenerateModule(Linker::Module& module) const override;
		/* TODO */
	};

	/**
	 * @brief Intel Relocatable Object Module for the Intel 8080
	 */
	class OMF80Format : public virtual OMFFormat, public virtual Linker::InputFormat
	{
	public:
		class Module;

		enum record_type_t : uint8_t
		{
			ModuleHeader = 0x02,
			ModuleEnd = 0x04,
			Content = 0x06,
			LineNumbers = 0x08,
			EndOfFile = 0x0E,
			ModuleAncestor = 0x10,
			LocalSymbols = 0x12,
			PublicDefinitions = 0x16,
			ExternalDefinitions = 0x18,
			ExternalReferences = 0x20,
			Relocations = 0x22,
			InterSegmentReferences = 0x24,
			LibraryModuleLocations = 0x26,
			LibraryModuleNames = 0x28,
			LibraryDictionary = 0x2A,
			LibraryHeader = 0x2C,
			NamedCommonDefinitions = 0x2E,
		};

		static const std::map<offset_t, std::string> RecordTypeNames;

		using Record = OMFFormat::Record<record_type_t, OMF80Format, Module>;
		using UnknownRecord = OMFFormat::UnknownRecord<record_type_t, OMF80Format, Module>;
		using EmptyRecord = OMFFormat::EmptyRecord<record_type_t, OMF80Format, Module>;
		using ContentRecord = OMFFormat::ContentRecord<record_type_t, OMF80Format, Module>;
		using LineNumbersRecord = OMFFormat::LineNumbersRecord<record_type_t, OMF80Format, Module>;
		using LibraryHeaderRecord = OMFFormat::LibraryHeaderRecord<record_type_t, OMF80Format, Module>;
		using LibraryModuleNamesRecord = OMFFormat::LibraryModuleNamesRecord<record_type_t, OMF80Format, Module>;
		using LibraryModuleLocationsRecord = OMFFormat::LibraryModuleLocationsRecord<record_type_t, OMF80Format, Module>;
		using LibraryDictionaryRecord = OMFFormat::LibraryDictionaryRecord<record_type_t, OMF80Format, Module>;

		/** @brief Parses and returns an instance of the next record */
		std::shared_ptr<Record> ReadRecord(Linker::Reader& rd);

		/**
		 * @brief Represents one of the 256 segments
		 *
		 * Segment number 5 is reserved by Intel, 6 to 255 are common segments
		 */
		typedef uint8_t segment_id_t;

		/** @brief The absolute segment, all offsets are absolute memory locations */
		static constexpr segment_id_t AbsoluteSegment = 0;
		/** @brief Used for instructions and generally anything loaded into the ROM */
		static constexpr segment_id_t CodeSegment = 1;
		/** @brief Used for data and generally anything loaded into the RAM */
		static constexpr segment_id_t DataSegment = 2;
		/** @brief Used to specify size of segment */
		static constexpr segment_id_t StackSegment = 3;
		/** @brief Used to specify the highest memory address available */
		static constexpr segment_id_t MemorySegment = 4;
		/** @brief The first segment number corresponding to a named common segment */
		static constexpr segment_id_t FirstNamedCommonSegment = 6;
		/** @brief The last segment number corresponding to a named common segment */
		static constexpr segment_id_t LastNamedCommonSegment = 254;
		/** @brief The unnamed common segment */
		static constexpr segment_id_t UnnamedCommonSegment = 255;

		// TODO: document

		enum alignment_t : uint8_t
		{
			// page is 256 bytes
			AlignFitsPage = 1,
			AlignPage = 2,
			AlignByte = 3,
		};

		class ExternalNameIndex
		{
		public:
			uint16_t index;
			std::string external_name;

			void CalculateValues(OMF80Format * omf, Module * mod);
			void ResolveReferences(OMF80Format * omf, Module * mod);
		};

		class ModuleHeaderRecord : public Record
		{
		public:
			class SegmentDefinition
			{
			public:
				segment_id_t segment_id = 0;
				uint16_t length = 0;
				alignment_t alignment = AlignByte;

				static SegmentDefinition Read(OMF80Format * omf, Linker::Reader& rd);
				void Write(OMF80Format * omf, ChecksumWriter& wr) const;
			};

			std::string name;
			std::vector<SegmentDefinition> segment_definitions;

			ModuleHeaderRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF80Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF80Format * omf, Module * mod) override;
			void ResolveReferences(OMF80Format * omf, Module * mod) override;
		};

		class ModuleEndRecord : public Record
		{
		public:
			bool main;
			segment_id_t start_segment_id;
			uint16_t start_offset;
			std::vector<uint8_t> info;

			ModuleEndRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF80Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF80Format * omf, Module * mod) override;
			void ResolveReferences(OMF80Format * omf, Module * mod) override;
		};

		class NamedCommonDefinitionsRecord : public Record
		{
		public:
			class NamedCommonDefinition
			{
			public:
				segment_id_t segment_id;
				std::string common_name;

				static NamedCommonDefinition ReadNamedCommonDefinition(OMF80Format * omf, Linker::Reader& rd);
				uint16_t GetNamedCommonDefinitionSize(OMF80Format * omf) const;
				void WriteNamedCommonDefinition(OMF80Format * omf, ChecksumWriter& wr) const;
			};

			std::vector<NamedCommonDefinition> named_common_definitions;

			NamedCommonDefinitionsRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF80Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF80Format * omf, Module * mod) override;
			void ResolveReferences(OMF80Format * omf, Module * mod) override;
		};

		class ExternalDefinitionsRecord : public Record
		{
		public:
			Module * mod;

			std::vector<std::string> external_names; // only used for generation
			uint16_t first_external_name;
			uint16_t external_name_count;

			ExternalDefinitionsRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF80Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF80Format * omf, Module * mod) override;
			void ResolveReferences(OMF80Format * omf, Module * mod) override;
		};

		class SymbolDefinitionsRecord : public Record
		{
		public:
			class SymbolDefinition
			{
			public:
				uint16_t offset;
				std::string name;

				static SymbolDefinition Read(OMF80Format * omf, Module * mod, Linker::Reader& rd);
				uint16_t Size(OMF80Format * omf, Module * mod) const;
				void Write(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const;
			};

			uint8_t segment_id;
			std::vector<SymbolDefinition> public_definitions;

			SymbolDefinitionsRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF80Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF80Format * omf, Module * mod) override;
			void ResolveReferences(OMF80Format * omf, Module * mod) override;
		};

		enum relocation_type_t : uint8_t
		{
			RelocationLowByte = 1,
			RelocationHighByte = 2,
			RelocationWord = 3,
		};

		class RelocationsRecord : public Record
		{
		public:
			relocation_type_t relocation_type;
			std::vector<uint16_t> offsets;

			RelocationsRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF80Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF80Format * omf, Module * mod) override;
			void ResolveReferences(OMF80Format * omf, Module * mod) override;
		};

		class InterSegmentReferencesRecord : public RelocationsRecord
		{
		public:
			segment_id_t segment_id;

			InterSegmentReferencesRecord(record_type_t record_type = record_type_t(0))
				: RelocationsRecord(record_type)
			{
			}

			void ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF80Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF80Format * omf, Module * mod) override;
			void ResolveReferences(OMF80Format * omf, Module * mod) override;
		};

		class ExternalReferencesRecord : public Record
		{
		public:
			class ExternalReference
			{
			public:
				ExternalNameIndex name_index;
				uint16_t offset;
			};

			relocation_type_t relocation_type;
			std::vector<ExternalReference> external_references;

			ExternalReferencesRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF80Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF80Format * omf, Module * mod) override;
			void ResolveReferences(OMF80Format * omf, Module * mod) override;
		};

		class ModuleAncestorRecord : public Record
		{
		public:
			std::string name;

			ModuleAncestorRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF80Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF80Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF80Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF80Format * omf, Module * mod) override;
			void ResolveReferences(OMF80Format * omf, Module * mod) override;
		};

		class SegmentDefinition : public ModuleHeaderRecord::SegmentDefinition
		{
		public:
			segment_id_t segment_id = 0;
			uint16_t length = 0;
			alignment_t alignment = AlignByte;
			std::string common_name;

			SegmentDefinition()
			{
			}

			SegmentDefinition(const ModuleHeaderRecord::SegmentDefinition& segment_definition)
				: segment_id(segment_definition.segment_id), length(segment_definition.length), alignment(segment_definition.alignment)
			{
			}
		};

		class Module
		{
		public:
			/** @brief The index of the first record inside the file, stored in the OMF86Format objects */
			size_t first_record = 0;
			/** @brief The number of records belonging to this module */
			size_t record_count = 0;

			std::map<segment_id_t, SegmentDefinition> segment_definitions;
			std::vector<std::string> external_names;
		};

		/** @brief The ordered collection of records contained in the file */
		std::vector<std::shared_ptr<Record>> records;

		/** @brief List of modules appearing in an OMF file, typically only one for an object file */
		std::vector<Module> modules;

		/** @brief Parses an OMF80 file */
		static std::shared_ptr<OMF80Format> ReadOMFFile(Linker::Reader& rd);

		static void DumpAddFields(const Record * record, Dumper::Dumper& dump, Dumper::Region& region, const OMF80Format * omf, const Module * mod, size_t record_index);

		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
		using Linker::InputFormat::GenerateModule;
		void GenerateModule(Linker::Module& module) const override;
		/* TODO */
	};

	/**
	 * @brief Intel Relocatable Object Module for the Intel MCS-51
	 */
	class OMF51Format : public virtual OMFFormat, public virtual Linker::InputFormat
	{
	public:
		class Module;

		enum record_type_t : uint8_t
		{
			ModuleHeader = OMF80Format::ModuleHeader,
			ModuleEnd = OMF80Format::ModuleEnd,
			Content = OMF80Format::Content,
			Fixups = 0x08,
			SegmentDefinitions = 0x0E,
			ScopeDefinition = 0x10,
			DebugItems = 0x12,
			PublicDefinitions = OMF80Format::PublicDefinitions,
			ExternalDefinitions = OMF80Format::ExternalDefinitions,
			LibraryModuleLocations = OMF80Format::LibraryModuleLocations,
			LibraryModuleNames = OMF80Format::LibraryModuleNames,
			LibraryDictionary = OMF80Format::LibraryDictionary,
			LibraryHeader = OMF80Format::LibraryHeader,
		};

		static const std::map<offset_t, std::string> RecordTypeNames;

		using Record = OMFFormat::Record<record_type_t, OMF51Format, Module>;
		using UnknownRecord = OMFFormat::UnknownRecord<record_type_t, OMF51Format, Module>;
		using EmptyRecord = OMFFormat::EmptyRecord<record_type_t, OMF51Format, Module>;
		using ContentRecord = OMFFormat::ContentRecord<record_type_t, OMF51Format, Module>;
		using LibraryHeaderRecord = OMFFormat::LibraryHeaderRecord<record_type_t, OMF51Format, Module>;
		using LibraryModuleNamesRecord = OMFFormat::LibraryModuleNamesRecord<record_type_t, OMF51Format, Module>;
		using LibraryModuleLocationsRecord = OMFFormat::LibraryModuleLocationsRecord<record_type_t, OMF51Format, Module>;
		using LibraryDictionaryRecord = OMFFormat::LibraryDictionaryRecord<record_type_t, OMF51Format, Module>;

		/** @brief Parses and returns an instance of the next record */
		std::shared_ptr<Record> ReadRecord(Linker::Reader& rd);

		// TODO: document record types

		static constexpr uint8_t SegmentAbsolute = 0;

		enum segment_type_t : uint8_t
		{
			/** @brief 64 KiB read-only memory */
			SegmentCode = 0,
			/** @brief 64 KiB read-write memory */
			SegmentXData = 1,
			/** @brief 256 B internal RAM that is accessible via direct addressing, first 128 B shared with SegmentIData */
			SegmentData = 2,
			/** @brief 256 B internal RAM that is accessible via indirect addressing, first 128 B shared with SegmentData */
			SegmentIData = 3,
			/** @brief 256 bit address space, part of SegmentData */
			SegmentBit = 4,
			/** @brief Not a segment, but a symbol with this segment type is treated as a value */
			SegmentNone = 5,
		};

		enum alignment_t : uint8_t
		{
			/** @brief Non-relocatable */
			AlignAbsolute = 0,
			/** @brief Byte or bit alignment for SegmentBit segments */
			AlignUnit = 1,
			/** @brief Must be allocated within the bit addressable part of SegmentData */
			AlignBitAddressable = 2,
			/** @brief Must fit a 256 byte page */
			AlignFitsPage = 3,
			/** @brief Must fit a 256 byte block */
			AlignFitsBlock = 4,
			/** @brief Align to a 256 byte page boundary */
			AlignPage = 5,
		};

		class SegmentInfo
		{
		public:
			bool overlayable;
			uint8_t register_bank;
			segment_type_t segment_type;

			static constexpr uint8_t MaskSegmentType = 0x07;
			static constexpr uint8_t MaskRegisterBank = 0x18;
			static constexpr uint8_t ShiftRegisterBank = 3;
			static constexpr uint8_t FlagOverlayable = 0x20;
			static constexpr uint8_t FlagSegmentEmpty = 0x80;

			void ReadSegmentInfo(OMF51Format * omf, Module * mod, uint8_t segment_info);
			uint8_t WriteSegmentInfo(OMF51Format * omf, Module * mod) const;
		};

		class SegmentDefinition
		{
		public:
			uint8_t segment_id;
			SegmentInfo info;
			alignment_t alignment;
			uint16_t base;
			uint32_t size;
			std::string name;

			static SegmentDefinition Read(OMF51Format * omf, Module * mod, Linker::Reader& rd);
			uint16_t Size(OMF51Format * omf, Module * mod) const;
			void Write(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const;
		};

		class SymbolInfo
		{
		public:
			bool indirectly_callable;
			bool variable;
			std::optional<unsigned> register_bank;
			segment_type_t usage;

			static constexpr uint8_t MaskSegmentType = 0x07;
			static constexpr uint8_t MaskRegisterBank = 0x18;
			static constexpr uint8_t ShiftRegisterBank = 3;
			static constexpr uint8_t FlagRegisterBank = 0x20;
			static constexpr uint8_t FlagVariable = 0x40;
			static constexpr uint8_t FlagIndirectlyCallable = 0x80;

			void Read(OMF51Format * omf, Module * mod, uint8_t segment_info);
			uint8_t Write(OMF51Format * omf, Module * mod) const;
		};

		class SymbolDefinition
		{
		public:
			uint8_t segment_id;
			SymbolInfo info;
			uint16_t offset;
			std::string name;

			static SymbolDefinition Read(OMF51Format * omf, Module * mod, Linker::Reader& rd);
			uint16_t Size(OMF51Format * omf, Module * mod) const;
			void Write(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const;
		};

		class ExternalDefinition
		{
		public:
			uint8_t block_id = 2;
			uint8_t external_id;
			SymbolInfo info;
			std::string name;

			static ExternalDefinition Read(OMF51Format * omf, Module * mod, Linker::Reader& rd);
			uint16_t Size(OMF51Format * omf, Module * mod) const;
			void Write(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const;
		};

		class ModuleHeaderRecord : public Record
		{
		public:
			std::string name;
			enum translator_id_t : uint8_t
			{
				Assembler = 0xFD,
				PLMCompiler = 0xFE,
				Linker = 0xFF,
			};
			translator_id_t translator_id;

			ModuleHeaderRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF51Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF51Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		class ModuleEndRecord : public Record
		{
		public:
			std::string name;
			std::array<bool, 4> banks;

			ModuleEndRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF51Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF51Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		class SegmentDefinitionsRecord : public Record
		{
		public:
			std::vector<SegmentDefinition> segment_definitions;

			SegmentDefinitionsRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF51Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF51Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF51Format * omf, Module * mod) override;
			void ResolveReferences(OMF51Format * omf, Module * mod) override;
		};

		class PublicSymbolsRecord : public Record
		{
		public:
			std::vector<SymbolDefinition> symbol_definitions;

			PublicSymbolsRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF51Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF51Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF51Format * omf, Module * mod) override;
			void ResolveReferences(OMF51Format * omf, Module * mod) override;
		};

		class ExternalDefinitionsRecord : public Record
		{
		public:
			std::vector<ExternalDefinition> external_definitions;

			ExternalDefinitionsRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF51Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF51Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF51Format * omf, Module * mod) override;
			void ResolveReferences(OMF51Format * omf, Module * mod) override;
		};

		class ScopeDefinitionRecord : public Record
		{
		public:
			enum block_type_t : uint8_t
			{
				ModuleBlock = 0,
				DoBlock = 1,
				ProcedureBlock = 2,
				EndModuleBlock = 3,
				EndDoBlock = 4,
				EndProcedureBlock = 5,
			};

			block_type_t block_type;
			std::string name;

			ScopeDefinitionRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF51Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF51Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF51Format * omf, Module * mod) override;
			void ResolveReferences(OMF51Format * omf, Module * mod) override;
		};

		class DebugItemsRecord : public Record
		{
		public:
			class Symbol
			{
			public:
				uint8_t segment_id;
				SymbolInfo info;
				uint16_t offset;
				std::string name;

				static Symbol Read(OMF51Format * omf, Module * mod, Linker::Reader& rd);
				uint16_t Size(OMF51Format * omf, Module * mod) const;
				void Write(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const;
			};

			class LocalSymbols
			{
			public:
				std::vector<Symbol> symbols;
			};

			class PublicSymbols
			{
			public:
				std::vector<Symbol> symbols;
			};

			class SegmentSymbol
			{
			public:
				uint8_t segment_id;
				SegmentInfo info;
				uint16_t offset;
				std::string name;

				static SegmentSymbol Read(OMF51Format * omf, Module * mod, Linker::Reader& rd);
				uint16_t Size(OMF51Format * omf, Module * mod) const;
				void Write(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const;
			};

			class SegmentSymbols
			{
			public:
				std::vector<SegmentSymbol> symbols;
			};

			class LineNumber
			{
			public:
				uint8_t segment_id;
				uint16_t offset;
				uint16_t line_number;

				static LineNumber Read(OMF51Format * omf, Module * mod, Linker::Reader& rd);
				void Write(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const;
			};

			class LineNumbers
			{
			public:
				std::vector<LineNumber> symbols;
			};

			static constexpr uint8_t Type_LocalSymbols = 0;
			static constexpr uint8_t Type_PublicSymbols = 1;
			static constexpr uint8_t Type_SegmentSymbols = 2;
			static constexpr uint8_t Type_LineNumbers = 3;

			std::variant<LocalSymbols, PublicSymbols, SegmentSymbols, LineNumbers> contents;

			DebugItemsRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF51Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF51Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF51Format * omf, Module * mod) override;
			void ResolveReferences(OMF51Format * omf, Module * mod) override;
		};

		class FixupRecord : public Record
		{
		public:
			enum relocation_type_t : uint8_t
			{
				RelocationLow8 = 0,
				RelocationByte8 = 1,
				RelocationRelative8 = 2,
				RelocationHigh8 = 3,
				RelocationWord16 = 4,
				RelocationAbsolute11 = 5,
				RelocationBit7 = 6,
				RelocationConv7 = 7,
			};

			enum reference_type_t : uint8_t
			{
				/** @brief The starting address of the segment */
				ReferenceSegmentBase = 0,
				/** @brief The starting address of the portion of the segment in the file */
				ReferenceSegmentStart = 1,
				/** @brief An external reference */
				ReferenceExternal = 2,
			};

			class Fixup
			{
			public:
				uint16_t location;
				relocation_type_t relocation;
				reference_type_t reference;
				uint8_t id;
				uint16_t offset;

				static Fixup Read(OMF51Format * omf, Module * mod, Linker::Reader& rd);
				void Write(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const;
			};

			std::vector<Fixup> fixups;

			FixupRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF51Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF51Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF51Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF51Format * omf, Module * mod) override;
			void ResolveReferences(OMF51Format * omf, Module * mod) override;
		};

		class Module
		{
		public:
			/** @brief The index of the first record inside the file, stored in the OMF86Format objects */
			size_t first_record = 0;
			/** @brief The number of records belonging to this module */
			size_t record_count = 0;

			std::map<uint8_t, SegmentDefinition> segment_definitions;
			std::map<uint8_t, ExternalDefinition> external_definitions;
		};

		/** @brief The ordered collection of records contained in the file */
		std::vector<std::shared_ptr<Record>> records;

		/** @brief List of modules appearing in an OMF file, typically only one for an object file */
		std::vector<Module> modules;

		/** @brief Parses an OMF51 file */
		static std::shared_ptr<OMF51Format> ReadOMFFile(Linker::Reader& rd);

		static void DumpAddFields(const Record * record, Dumper::Dumper& dump, Dumper::Region& region, const OMF51Format * omf, const Module * mod, size_t record_index);

		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
		using Linker::InputFormat::GenerateModule;
		void GenerateModule(Linker::Module& module) const override;
		/* TODO */
	};

	/**
	 * @brief Intel Relocatable Object Module for the Intel MCS-96
	 */
	class OMF96Format : public virtual OMFFormat, public virtual Linker::InputFormat
	{
	public:
		class Module;

		typedef uint16_t index_t;

		enum record_type_t : uint8_t
		{
			ModuleHeader = OMF80Format::ModuleHeader,
			ModuleEnd = OMF80Format::ModuleEnd,
			Content = OMF80Format::Content,
			LineNumbers = OMF80Format::LineNumbers,
			BlockDefinition = 0x0A,
			BlockEnd = 0x0C,
			EndOfFile = OMF80Format::EndOfFile,
			ModuleAncestor = OMF80Format::ModuleAncestor,
			LocalSymbols = OMF80Format::LocalSymbols,
			TypeDefinition = 0x14,
			PublicDefinitions = OMF80Format::PublicDefinitions,
			ExternalDefinitions = OMF80Format::ExternalDefinitions,
			SegmentDefinitions = 0x20, // different from OMF51 number
			Relocations = OMF80Format::Relocations,
			LibraryModuleLocations = OMF80Format::LibraryModuleLocations,
			LibraryModuleNames = OMF80Format::LibraryModuleNames,
			LibraryDictionary = OMF80Format::LibraryDictionary,
			LibraryHeader = 0x2E, // different from OMF80 number
		};

		static const std::map<offset_t, std::string> RecordTypeNames;

		using Record = OMFFormat::Record<record_type_t, OMF96Format, Module>;
		using UnknownRecord = OMFFormat::UnknownRecord<record_type_t, OMF96Format, Module>;
		using EmptyRecord = OMFFormat::EmptyRecord<record_type_t, OMF96Format, Module>;
		using ContentRecord = OMFFormat::ContentRecord<record_type_t, OMF96Format, Module>;
		using LineNumbersRecord = OMFFormat::LineNumbersRecord<record_type_t, OMF96Format, Module>;
		using LibraryHeaderRecord = OMFFormat::LibraryHeaderRecord<record_type_t, OMF96Format, Module>;
		using LibraryModuleNamesRecord = OMFFormat::LibraryModuleNamesRecord<record_type_t, OMF96Format, Module>;
		using LibraryModuleLocationsRecord = OMFFormat::LibraryModuleLocationsRecord<record_type_t, OMF96Format, Module>;
		using LibraryDictionaryRecord = OMFFormat::LibraryDictionaryRecord<record_type_t, OMF96Format, Module>;

		/** @brief Parses and returns an instance of the next record */
		std::shared_ptr<Record> ReadRecord(Linker::Reader& rd);

		typedef uint8_t segment_id_t;

		static constexpr segment_id_t CodeSegment = 0;
		static constexpr segment_id_t DataSegment = 1;
		static constexpr segment_id_t RegisterSegment = 2;
		static constexpr segment_id_t OverlaySegment = 3;
		static constexpr segment_id_t StackSegment = 4;
		static constexpr segment_id_t DynamicSymbol = 5;
		static constexpr segment_id_t NullSegment = 6;
		static constexpr segment_id_t SegmentTypeMask = 7;

		static constexpr segment_id_t FlagBasedVariable = 0x40;

		static constexpr segment_id_t FlagRelocatableSegment = 0x80;

		enum alignment_t
		{
			AlignByte = 0,
			AlignWord = 1,
			AlignLong = 2,
		};

		class SegmentDefinition
		{
		public:
			segment_id_t segment_id;
		private:
			union
			{
				alignment_t alignment; // for RelocatableSegment
				uint16_t base_address;
			};
		public:
			uint16_t size;

			bool IsRelocatable() const
			{
				return segment_id & FlagRelocatableSegment;
			}

			bool IsAbsolute() const
			{
				return !IsRelocatable();
			}

			alignment_t GetAlignment() const
			{
				return IsRelocatable() ? alignment : alignment_t(-1);
			}

			uint16_t GetBaseAddress() const
			{
				return IsAbsolute() ? base_address : 0;
			}

			void MakeRelocatable(alignment_t rel_alignment)
			{
				segment_id |= FlagRelocatableSegment;
				alignment = rel_alignment;
			}

			void MakeAbsolute(uint16_t address)
			{
				segment_id &= ~FlagRelocatableSegment;
				base_address = address;
			}

			static SegmentDefinition Read(OMF96Format * omf, Module * mod, Linker::Reader& rd);
			uint16_t Size(OMF96Format * omf, Module * mod) const;
			void Write(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const;
		};

		class TypeDefinitionRecord;

		class TypeIndex
		{
		public:
			index_t index;
			std::shared_ptr<TypeDefinitionRecord> record;

			TypeIndex(index_t index = 0)
				: index(index)
			{
			}

			void CalculateValues(OMF86Format * omf, Module * mod);
			void ResolveReferences(OMF86Format * omf, Module * mod);
		};

		class ExternalDefinition
		{
		public:
			std::string name;
			TypeIndex type;

			static ExternalDefinition Read(OMF96Format * omf, Module * mod, Linker::Reader& rd);
			uint16_t Size(OMF96Format * omf, Module * mod) const;
			void Write(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const;

			void CalculateValues(OMF96Format * omf, Module * mod);
			void ResolveReferences(OMF96Format * omf, Module * mod);
		};

		class SymbolDefinition
		{
		public:
			uint16_t offset;
			std::string name;
			TypeIndex type;

			static SymbolDefinition Read(OMF96Format * omf, Module * mod, Linker::Reader& rd);
			uint16_t Size(OMF96Format * omf, Module * mod) const;
			void Write(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const;

			void CalculateValues(OMF96Format * omf, Module * mod);
			void ResolveReferences(OMF96Format * omf, Module * mod);
		};

		class ModuleHeaderRecord : public Record
		{
		public:
			std::string name;
			uint8_t translator_id;
			std::string date_time;

			static constexpr uint8_t Version14 = 0xE0;
			static constexpr uint8_t Version20 = 0xE4;
			static constexpr uint8_t Translator = 0x00;
			static constexpr uint8_t Linker = 0x01;

			ModuleHeaderRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF96Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		class ModuleEndRecord : public Record
		{
		public:
			bool main;
			bool valid;

			static constexpr uint8_t MainModule = 0x01;
			static constexpr uint8_t OtherModule = 0x00;

			static constexpr uint8_t ValidModule = 0x00;
			static constexpr uint8_t ErroneousModule = 0x01;

			ModuleEndRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF96Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const override;
		};

		class SegmentDefinitionsRecord : public Record
		{
		public:
			std::vector<SegmentDefinition> segment_definitions;

			SegmentDefinitionsRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF96Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF96Format * omf, Module * mod) override;
			void ResolveReferences(OMF96Format * omf, Module * mod) override;
		};

		class TypeDefinitionRecord : public Record, public std::enable_shared_from_this<TypeDefinitionRecord>
		{
		public:
			class LeafDescriptor
			{
			public:
				bool nice; // easy if false

				/** @brief Represents a null leaf */
				struct Null { };
				/** @brief Represents a final repeat leaf, meaning that the data in this type should be repeated indefinitely */
				struct Repeat { };
				// TODO: document
				struct EndOfBranch { };

				std::variant<Null, int32_t, std::string, TypeIndex, Repeat, EndOfBranch> leaf = Null();

				/** @brief Type byte for a null leaf */
				static constexpr uint8_t NullLeaf = 0x64;
				/** @brief Type byte for an signed 16-bit leaf */
				static constexpr uint8_t SignedNumericLeaf16 = 0x65;
				/** @brief Type byte for an signed 32-bit leaf */
				static constexpr uint8_t SignedNumericLeaf32 = 0x66;
				/** @brief Type byte for a string leaf */
				static constexpr uint8_t StringLeaf = 0x67;
				/** @brief Type byte for an index leaf, usually a type index */
				static constexpr uint8_t IndexLeaf = 0x68;
				/** @brief Type byte for a repeat leaf leaf */
				static constexpr uint8_t RepeatLeaf = 0x69;
				/** @brief Type byte for an end of branch leaf */
				static constexpr uint8_t EndOfBranchLeaf = 0x6A;

				static LeafDescriptor Read(OMF96Format * omf, Module * mod, Linker::Reader& rd);
				uint16_t Size(OMF96Format * omf, Module * mod) const;
				void Write(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const;

				void CalculateValues(OMF96Format * omf, Module * mod);
				void ResolveReferences(OMF96Format * omf, Module * mod);
			};

			/** @brief The parameters of this type */
			std::vector<LeafDescriptor> leafs;

			TypeDefinitionRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF96Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF96Format * omf, Module * mod) override;
			void ResolveReferences(OMF96Format * omf, Module * mod) override;
		};

		class SymbolDefinitionsRecord : public Record
		{
		public:
			segment_id_t segment_id;
			std::vector<SymbolDefinition> symbol_definitions;

			SymbolDefinitionsRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF96Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF96Format * omf, Module * mod) override;
			void ResolveReferences(OMF96Format * omf, Module * mod) override;
		};

		class ExternalDefinitionsRecord : public Record
		{
		public:
			segment_id_t segment_id;
			std::vector<ExternalDefinition> external_definitions;

			ExternalDefinitionsRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF96Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF96Format * omf, Module * mod) override;
			void ResolveReferences(OMF96Format * omf, Module * mod) override;
		};

		class RelocationRecord : public Record
		{
		public:
			class Relocation
			{
			public:
				typedef segment_id_t LocalReference;
				typedef uint16_t ExternalReference;

				enum reference_type_t
				{
					RegisterSimple = 0,
					RegisterAutoIncrement = 1,
					ShiftCountImmediate = 2,
					ShiftCountRegister = 3,
					ByteConstant = 4,
					ShortJump = 6,
					MediumJump = 7,
					MediumCall = 8,
					LongJumpCall = 9,
					LongDirect = 10,
				};

				static constexpr uint8_t FlagAddendInCode = 0x40;
				static constexpr uint8_t FlagExternal = 0x80;

				uint16_t offset;
				std::variant<LocalReference, ExternalReference> reference = LocalReference(0);
				std::optional<uint16_t> addend;
				reference_type_t reference_type;
				alignment_t alignment;

				static Relocation Read(OMF96Format * omf, Module * mod, Linker::Reader& rd);
				uint16_t Size(OMF96Format * omf, Module * mod) const;
				void Write(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const;
			};

			std::vector<Relocation> relocations;

			RelocationRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF96Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF96Format * omf, Module * mod) override;
			void ResolveReferences(OMF96Format * omf, Module * mod) override;
		};

		class ModuleAncestorRecord : public Record
		{
		public:
			std::string name;
			std::optional<SegmentDefinition> segment_definition;

			ModuleAncestorRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			void ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF96Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF96Format * omf, Module * mod) override;
			void ResolveReferences(OMF96Format * omf, Module * mod) override;
		};

		class BlockDefinitionRecord : public Record
		{
		public:
			class BlockNameAndType
			{
			public:
				std::string name;
				TypeIndex type;
			};

			typedef segment_id_t LocalReference;
			typedef uint16_t ExternalReference;

			class ProcedureInformation
			{
			public:
				std::variant<LocalReference, ExternalReference> frame_pointer = LocalReference(0);
				uint16_t return_offset;
				uint8_t prologue_size;
			};

			segment_id_t segment_id;
			uint16_t offset;
			uint16_t size;
			std::optional<BlockNameAndType> name_and_type;
			std::optional<ProcedureInformation> procedure_info;

			BlockDefinitionRecord(record_type_t record_type = record_type_t(0))
				: Record(record_type)
			{
			}

			static constexpr uint8_t FlagProcedure = 0x40;
			static constexpr uint8_t FlagExternal = 0x80;

			void ReadRecordContents(OMF96Format * omf, Module * mod, Linker::Reader& rd) override;
			uint16_t GetRecordSize(OMF96Format * omf, Module * mod) const override;
			void WriteRecordContents(OMF96Format * omf, Module * mod, ChecksumWriter& wr) const override;

			void CalculateValues(OMF96Format * omf, Module * mod) override;
			void ResolveReferences(OMF96Format * omf, Module * mod) override;
		};

		class Module
		{
		public:
			/** @brief The index of the first record inside the file, stored in the OMF86Format objects */
			size_t first_record = 0;
			/** @brief The number of records belonging to this module */
			size_t record_count = 0;

			std::map<segment_id_t, SegmentDefinition> segment_definitions;
			std::vector<std::shared_ptr<TypeDefinitionRecord>> type_definitions;
		};

		/** @brief The ordered collection of records contained in the file */
		std::vector<std::shared_ptr<Record>> records;

		/** @brief List of modules appearing in an OMF file, typically only one for an object file */
		std::vector<Module> modules;

		/** @brief Parses an OMF96 file */
		static std::shared_ptr<OMF96Format> ReadOMFFile(Linker::Reader& rd);

		static void DumpAddFields(const Record * record, Dumper::Dumper& dump, Dumper::Region& region, const OMF96Format * omf, const Module * mod, size_t record_index);

		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
		using Linker::InputFormat::GenerateModule;
		void GenerateModule(Linker::Module& module) const override;
		/* TODO */
	};

	class OMFFormatContainer : public virtual Linker::InputFormat
	{
	public:
		std::shared_ptr<OMFFormat> contents;

		void ReadFile(Linker::Reader& rd) override;

		using Linker::InputFormat::GenerateModule;
		void GenerateModule(Linker::Module& module) const override;

		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;

		offset_t ImageSize() const override;

		void Dump(Dumper::Dumper& dump) const override;
	};
}

#endif /* OMF_H */
