#ifndef GSOS_H
#define GSOS_H

#include <optional>
#include <variant>
#include "../common.h"
#include "../dumper/dumper.h"
#include "../linker/module.h"
#include "../linker/segment.h"
#include "../linker/segment_manager.h"
#include "../linker/writer.h"

/* TODO: most of this code is not tested, it can parse and dump executable binaries */

namespace Apple
{
	/**
	 * @brief Apple GS/OS OMF file format
	 *
	 * The file format was originally invented for ORCA/M as an object format, later adopted for the Apple GS/OS operating system.
	 * It had multiple versions, including a few early versions, version 1, 2 and 2.1.
	 */
	class OMFFormat : public virtual Linker::SegmentManager
	{
	public:
		class Segment
		{
		public:
			/** @brief Offset of segment within file */
			offset_t segment_offset = 0;
			/** @brief (BYTECNT/BLKCNT) Total size of the segment, including the header, as stored in file */
			offset_t total_segment_size = 0;
			/** @brief (RESSPC) Additional zeroes to append to segment */
			offset_t bss_size = 0;
			/** @brief (LENGTH) Total size of segment when loaded into memory, including the additional zeroes */
			offset_t total_size = 0;
			enum segment_kind
			{
				/** @brief Code segment */
				SEG_CODE = 0x00,
				/** @brief Data segment */
				SEG_DATA = 0x01,
				/** @brief Jump table segment */
				SEG_JUMPTABLE = 0x02,
				/** @brief Pathname segment */
				SEG_PATHNAME = 0x04,
				/** @brief Library dictionary segment */
				SEG_LIBDICT = 0x08,
				/** @brief Initialization segment */
				SEG_INIT = 0x10,
				/** @brief Absolute bank segment (version 1 only) */
				SEG_ABSBANK = 0x11,
				/** @brief Direct page/stack segment */
				SEG_DIRPAGE = 0x12,
			};
			/** @brief (KIND) Segment type */
			segment_kind kind = SEG_CODE;
			/** @brief (KIND) Segment flags */
			uint16_t flags = 0;
			/** @brief (LABLEN) Label length, or 0 for variable length labels */
			uint8_t label_length = 0;
			/** @brief (NUMLEN) Number length, must be 4 */
			uint8_t number_length = 4;
			enum omf_version
			{
				OMF_VERSION_0   = 0x0000, // TODO: unsure
				OMF_VERSION_1   = 0x0100,
				OMF_VERSION_2   = 0x0200,
				OMF_VERSION_2_1 = 0x0201,
			};
			/** @brief (VERSION, REVISION) OMF file format version */
			omf_version version = OMF_VERSION_2;
			/** @brief (BANKSIZE) Maximum bank size of segment */
			offset_t bank_size = 0;
			/** @brief (ORG) Base address of segment */
			offset_t base_address = 0;
			/** @brief (ALIGN) Segment alignment */
			offset_t align = 0x100;
			/** @brief (NUMSEX) Endianness, must be 0 for ::LittleEndian */
			uint8_t endiantype = 0;
			/** @brief (LCBANK) Language card bank */
			uint8_t language_card_bank = 0;
			/** @brief (SEGNUM) Segment number (not present in version 0) */
			uint16_t segment_number = 0;
			/** @brief (ENTRY) Entry point (not present in version 0) */
			offset_t entry = 0;
			/** @brief (DISPNAME) Offset to linker_segment_name (LOADNAME), (not present in version 0) */
			offset_t segment_name_offset = 0x2C;
			/** @brief (DISPDATA) Offset to segment data, typically appearing after segment_name (not present in version 0) */
			offset_t segment_data_offset = 0;
			/** @brief (tempOrg) (only version 2.1, optional) */
			offset_t temp_org = 0;
			/** @brief (LOADNAME) Name of segment according to linker */
			std::string linker_segment_name;
			/** @brief (LOADNAME) Name of segment */
			std::string segment_name;

			/** @brief Segment flag: Bank relative (version 2 only) */
			static const uint16_t FLAG_BANK_RELATIVE = 0x0100;
			/** @brief Segment flag: Skip segment (version 2 only) */
			static const uint16_t FLAG_SKIP_SEGMENT = 0x0200;
			/** @brief Segment flag: Reload segment (version 2 only) */
			static const uint16_t FLAG_RELOAD = 0x0400;
			/** @brief Segment flag: Absolute bank segment (version 2 only) */
			static const uint16_t FLAG_ABSOLUTE_BANK = 0x0800;
			/** @brief Segment flag: Do not load in special memory (version 2 only) */
			static const uint16_t FLAG_NO_SPECIAL_MEMORY = 0x1000;
			/** @brief Segment flag: Position independent */
			static const uint16_t FLAG_POSITION_INDEPENDENT = 0x2000;
			/** @brief Segment flag: Private */
			static const uint16_t FLAG_PRIVATE = 0x4000;
			/** @brief Segment flag: Dynamic */
			static const uint16_t FLAG_DYNAMIC = 0x8000;

			::EndianType GetEndianType() const;

			/** @brief Reads a number the size of number_length */
			offset_t ReadUnsigned(Linker::Reader& rd) const;
			/** @brief Writes a number the size of number_length */
			void WriteWord(Linker::Writer& wr, offset_t value) const;

			/** @brief Reads a string the size of label_length, or variable length if it is 0 */
			std::string ReadLabel(Linker::Reader& rd) const;
			/** @brief Writes a string the size of label_length, or variable length if it is 0 */
			void WriteLabel(Linker::Writer& wr, std::string text) const;

			/** @brief Calculates the values needed to generate segment images
			 *
			 * @param segment_number The index of the current segment (1 based)
			 * @param current_offset Offset to current segment
			 * @return Offset to end of segment
			 */
			offset_t CalculateValues(uint16_t segment_number, offset_t current_offset);
			void ReadFile(Linker::Reader& rd);
			void WriteFile(Linker::Writer& wr) const;
			void Dump(Dumper::Dumper& dump, const OMFFormat& omf, unsigned segment_index) const;

			size_t ReadData(size_t bytes, offset_t offset, void * buffer) const;
			offset_t ReadUnsigned(size_t bytes, offset_t offset) const;

			class Expression
			{
			public:
				// TODO: untested

				/** @brief Represents an operation inside an expression in the object file
				 *
				 * The numerical values are identical to the opcode bytes in the binary file
				 */
				enum operation_type
				{
					End = 0x00,
					Addition = 0x01,
					Subtraction = 0x02,
					Multiplication = 0x03,
					Division = 0x04,
					IntegerRemainder = 0x05,
					Negation = 0x06,
					BitShift = 0x07,
					And = 0x08,
					Or = 0x09,
					EOr = 0x0A,
					Not = 0x0B,
					LessOrEqualTo = 0x0C,
					GreaterOrEqualTo = 0x0D,
					NotEqual = 0x0E,
					LessThan = 0x0F,
					GreaterThan = 0x10,
					EqualTo = 0x11,
					BitAnd = 0x12,
					BitOr = 0x13,
					BitEOr = 0x14,
					BitNot = 0x15,
					LocationCounterOperand = 0x80,
					ConstantOperand = 0x81,
					WeakLabelReferenceOperand = 0x82,
					LabelReferenceOperand = 0x83,
					LengthOfLabelReferenceOperand = 0x84,
					TypeOfLabelReferenceOperand = 0x85,
					CountOfLabelReferenceOperand = 0x86,
					RelativeOffsetOperand = 0x87,

					/** @brief A symbolic value to represent a stack underflow condition, it should not be printed back into a binary */
					StackUnderflow = -1,
				};

				/** @brief The type of operation, End means that the result is the last operand */
				operation_type operation = End;
				/** @brief The operands the operation takes */
				std::vector<std::unique_ptr<Expression>> operands;
				/** @brief A value corresponding to an operation, such as an integer constant or a label name */
				std::optional<std::variant<offset_t, std::string>> value;

				Expression(int operation)
					: operation(operation_type(operation))
				{
				}

				Expression(int operation, std::unique_ptr<Expression>& operand)
					: operation(operation_type(operation))
				{
					operands.emplace_back(std::move(operand));
				}

				Expression(int operation, std::unique_ptr<Expression>& operand1, std::unique_ptr<Expression>& operand2)
					: operation(operation_type(operation))
				{
					operands.emplace_back(std::move(operand1));
					operands.emplace_back(std::move(operand2));
				}

				Expression(int operation, offset_t value)
					: operation(operation_type(operation)), value(value)
				{
				}

				Expression(int operation, std::string value)
					: operation(operation_type(operation)), value(value)
				{
				}

				offset_t GetLength(const Segment& segment) const;
				void ReadFile(Segment& segment, Linker::Reader& rd);
				void WriteFile(const Segment& segment, Linker::Writer& wr) const;
			protected:
				/** @brief Removes elements from the top of the operands stack and copies them into target. On stack underflow, the missing elements are replaced with StackUnderflow expressions. */
				void PopElementsInto(size_t count, std::vector<std::unique_ptr<Expression>>& target);
				/** @brief Reads a single byte of operation (plus optional number and label) and modifies the current list of operands as an operation acting on a stack */
				uint8_t ReadSingleOperation(Segment& segment, Linker::Reader& rd);
			};

			/** @brief A single record inside the segment body, also represents an END record */
			class Record
			{
			public:
				enum record_type
				{
					// E: executable, O: object, L: library
					OPC_END = 0x00, // EOL
					OPC_CONST_BASE = 0x00,
					OPC_CONST_FIRST = 0x01, // O
					OPC_CONST_LAST = 0xDF, // O
					OPC_ALIGN = 0xE0, // O
					OPC_ORG = 0xE1, // O
					OPC_RELOC = 0xE2, // E
					OPC_INTERSEG = 0xE3, // E
					OPC_USING = 0xE4, // O
					OPC_STRONG = 0xE5, // O
					OPC_GLOBAL = 0xE6, // O
					OPC_GEQU = 0xE7, // O
					OPC_MEM = 0xE8, // O
					OPC_EXPR = 0xEB, // O
					OPC_ZEXPR = 0xEC, // O
					OPC_BEXPR = 0xED, // O
					OPC_RELEXPR = 0xEE, // O
					OPC_LOCAL = 0xEF, // O
					OPC_EQU = 0xF0, // O
					OPC_DS = 0xF1, // EOL
					OPC_LCONST = 0xF2, // EOL
					OPC_LEXPR = 0xF3, // O
					OPC_ENTRY = 0xF4, // L
					OPC_C_RELOC = 0xF5, // E
					OPC_C_INTERSEG = 0xF6, // E
					OPC_SUPER = 0xF7, // E (V2)
				};
				record_type type;

				Record(record_type type)
					: type(type)
				{
				}

				virtual ~Record() = default;
				/** @brief Returns the size of the record, as stored in the file
				 *
				 * @param segment The segment the record is part of
				 */
				virtual offset_t GetLength(const Segment& segment) const;
				/** @brief Returns the amount of memory the record occupies when loaded into memory. Most records have a length of 0
				 *
				 * @param segment The segment the record is part of
				 */
				virtual offset_t GetMemoryLength(const Segment& segment) const;
				virtual void ReadFile(Segment& segment, Linker::Reader& rd);
				virtual void WriteFile(const Segment& segment, Linker::Writer& wr) const;
				/** @brief Displays information pertaining to this record
				 *
				 * @param dump The dumper interface
				 * @param omf The entire file
				 * @param segment The segment the record is part of
				 * @param index 0-based index of the record within the segment
				 * @param file_offset Full file offset, including segment start offset, that this record starts at
				 * @param address The current memory pointer when this record is interpreted
				 */
				virtual void Dump(Dumper::Dumper& dump, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const;
				/** @brief Adds any further fields to the file region that encompasses this record */
				virtual void AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const;
				/** @brief If this is a relocation record, add the relocation signals to the block */
				virtual void AddSignals(Dumper::Block& block, offset_t current_segment_offset) const;
				/** @brief Attempts to read data from the in-memory image, if possible, it can be assumed that offset + bytes <= GetMemoryLength() */
				virtual void ReadData(size_t bytes, offset_t offset, void * buffer) const;
			};

			/** @brief Represents a CONST or LCONST record, containing a sequence of bytes */
			class DataRecord : public Record
			{
			public:
				std::shared_ptr<Linker::Image> image;

				DataRecord(record_type type, std::shared_ptr<Linker::Image> image)
					: Record(type), image(image)
				{
				}

				DataRecord(record_type type)
					: Record(type)
				{
				}

				offset_t GetLength(const Segment& segment) const override;
				offset_t GetMemoryLength(const Segment& segment) const override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(const Segment& segment, Linker::Writer& wr) const override;
				void Dump(Dumper::Dumper& dump, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const override;
				void ReadData(size_t bytes, offset_t offset, void * buffer) const override;
			};

			/** @brief Represents an ALIGN, ORG or DS record, containing an integer */
			class ValueRecord : public Record
			{
			public:
				offset_t value;

				ValueRecord(record_type type, offset_t value)
					: Record(type), value(value)
				{
				}

				offset_t GetLength(const Segment& segment) const override;
				offset_t GetMemoryLength(const Segment& segment) const override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(const Segment& segment, Linker::Writer& wr) const override;
				void AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const override;
				void ReadData(size_t bytes, offset_t offset, void * buffer) const override;
			};

			/** @brief Represents a USING or STRONG record, containing a string */
			class StringRecord : public Record
			{
			public:
				std::string value;

				StringRecord(record_type type, std::string value)
					: Record(type), value(value)
				{
				}

				offset_t GetLength(const Segment& segment) const override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(const Segment& segment, Linker::Writer& wr) const override;
			};

			/** @brief Represents a RELOC or cRELOC record, containing an intrasegment relocation */
			class RelocationRecord : public Record
			{
			public:
				uint8_t size = 0;
				int8_t shift = 0;
				offset_t source = 0;
				offset_t target = 0;

				RelocationRecord(record_type type)
					: Record(type)
				{
				}

				RelocationRecord(record_type type, uint8_t size, int8_t shift, offset_t source, offset_t target)
					: Record(type), size(size), shift(shift), source(source), target(target)
				{
				}

				offset_t GetLength(const Segment& segment) const override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(const Segment& segment, Linker::Writer& wr) const override;
				void AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const override;
				void AddSignals(Dumper::Block& block, offset_t current_segment_offset) const override;
			};

			/** @brief Represents an INTERSEG or cINTERSEG record, containing an intersegment relocation */
			class IntersegmentRelocationRecord : public RelocationRecord
			{
			public:
				uint16_t file_number = 0;
				uint16_t segment_number = 0;

				explicit IntersegmentRelocationRecord()
					: RelocationRecord(record_type(0))
				{
				}

				IntersegmentRelocationRecord(record_type type)
					: RelocationRecord(type)
				{
				}

				IntersegmentRelocationRecord(record_type type, uint8_t size, uint8_t shift, offset_t source, uint16_t file_number, uint16_t segment_number, offset_t target)
					: RelocationRecord(type, size, shift, source, target), file_number(file_number), segment_number(segment_number)
				{
				}

				offset_t GetLength(const Segment& segment) const override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(const Segment& segment, Linker::Writer& wr) const override;
				void AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const override;
			};

			/** @brief Represents a LOCAL or GLOBAL record */
			class LabelRecord : public Record
			{
			public:
				std::string name;
				uint16_t line_length = 0;
				enum operation_type
				{
					OP_ADDRESS_DC = 'A',
					OP_BOOL_DC = 'B',
					OP_CHAR_DC = 'C',
					OP_DOUBLE_DC = 'D',
					OP_FLOAT_DC = 'E',
					OP_EQU_GEQU = 'G',
					OP_HEX_DC = 'H',
					OP_INT_DC = 'I',
					OP_REF_ADDRESS_DC = 'K',
					OP_SOFT_REF_DC = 'L',
					OP_INSTRUCTION = 'M',
					OP_ASM_DIRECTIVE = 'N',
					OP_ORG = 'O',
					OP_ALIGN = 'P',
					OP_DS = 'S',
					OP_ARITHMETIC_SYMBOL = 'X',
					OP_BOOL_SYMBOL = 'Y',
					OP_CHAR_SYMBOL = 'Z'
				};
				operation_type operation = operation_type(0);
				uint16_t private_flag = 0;

				LabelRecord(record_type type)
					: Record(type)
				{
				}

				LabelRecord(record_type type, std::string name, uint16_t line_length, int operation, uint16_t private_flag)
					: Record(type), name(name), line_length(line_length), operation(operation_type(operation)), private_flag(private_flag)
				{
				}

				offset_t GetLength(const Segment& segment) const override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(const Segment& segment, Linker::Writer& wr) const override;
			};

			/** @brief Represents an EQU or GEQU record */
			class LabelExpressionRecord : public LabelRecord
			{
			public:
				std::unique_ptr<Expression> expression;

				LabelExpressionRecord(record_type type)
					: LabelRecord(type)
				{
				}

				LabelExpressionRecord(record_type type, std::string name, uint16_t line_length, int operation, uint16_t private_flag, std::unique_ptr<Expression> expression)
					: LabelRecord(type, name, line_length, operation, private_flag), expression(std::move(expression))
				{
				}

				offset_t GetLength(const Segment& segment) const override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(const Segment& segment, Linker::Writer& wr) const override;
			};

			/** @brief Represents a MEM record */
			class RangeRecord : public Record
			{
			public:
				offset_t start = 0;
				offset_t end = 0;

				RangeRecord(record_type type)
					: Record(type)
				{
				}

				RangeRecord(record_type type, offset_t start, offset_t end)
					: Record(type), start(start), end(end)
				{
				}

				offset_t GetLength(const Segment& segment) const override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(const Segment& segment, Linker::Writer& wr) const override;
			};

			/** @brief Represents an EXPR, ZEXPR, BEXPR or LEXPR record */
			class ExpressionRecord : public Record
			{
			public:
				uint8_t size = 0;
				std::unique_ptr<Expression> expression;

				ExpressionRecord(record_type type)
					: Record(type)
				{
				}

				ExpressionRecord(record_type type, uint8_t size, std::unique_ptr<Expression> expression)
					: Record(type), size(size), expression(std::move(expression))
				{
				}

				offset_t GetLength(const Segment& segment) const override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(const Segment& segment, Linker::Writer& wr) const override;
			};

			/** @brief Represents a RELEXPR record */
			class RelativeExpressionRecord : public ExpressionRecord
			{
			public:
				offset_t origin = 0;

				RelativeExpressionRecord(record_type type)
					: ExpressionRecord(type)
				{
				}

				RelativeExpressionRecord(record_type type, uint8_t size, offset_t origin, std::unique_ptr<Expression> expression)
					: ExpressionRecord(type, size, std::move(expression)), origin(origin)
				{
				}

				offset_t GetLength(const Segment& segment) const override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(const Segment& segment, Linker::Writer& wr) const override;
			};

			/** @brief Represents an ENTRY record */
			class EntryRecord : public Record
			{
			public:
				uint16_t segment_number;
				offset_t location;
				std::string name;

				EntryRecord(record_type type)
					: Record(type)
				{
				}

				EntryRecord(record_type type, uint16_t segment_number, offset_t location, std::string name)
					: Record(type), segment_number(segment_number), location(location), name(name)
				{
				}

				offset_t GetLength(const Segment& segment) const override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(const Segment& segment, Linker::Writer& wr) const override;
			};

			/** @brief Represents a SUPER record */
			class SuperCompactRecord : public Record
			{
			public:
				enum super_record_type
				{
					SUPER_RELOC2,
					SUPER_RELOC3,
					SUPER_INTERSEG1,
					SUPER_INTERSEG13 = SUPER_INTERSEG1 - 1 + 13,
					SUPER_INTERSEG25 = SUPER_INTERSEG1 - 1 + 25,
					SUPER_INTERSEG36 = SUPER_INTERSEG1 - 1 + 36,
				};

				super_record_type super_type = super_record_type(0);

				std::vector<uint16_t> offsets;

				SuperCompactRecord(record_type type, super_record_type super_type = super_record_type(0))
					: Record(type), super_type(super_type)
				{
				}

				offset_t GetLength(const Segment& segment) const override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(const Segment& segment, Linker::Writer& wr) const override;
			private:
				void WritePatchList(Linker::Writer& wr, const std::vector<uint8_t>& patches) const;

			public:
				void Dump(Dumper::Dumper& dump, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const override;
				void AddFields(Dumper::Dumper& dump, Dumper::Region& region, const OMFFormat& omf, const Segment& segment, unsigned index, offset_t file_offset, offset_t address) const override;
				void AddSignals(Dumper::Block& block, offset_t current_segment_offset) const override;

				/** @brief Expands the super compact relocation into a full relocation, without filling in the target fields */
				bool GetRelocation(IntersegmentRelocationRecord& relocation, unsigned index) const;
				/** @brief Expands the super compact relocation into a full relocation and fills in the target fields */
				bool GetRelocation(IntersegmentRelocationRecord& relocation, unsigned index, const Segment& segment) const;
			};

			std::vector<std::unique_ptr<Record>> records;

			std::unique_ptr<Expression> ReadExpression(Linker::Reader& rd);

			std::unique_ptr<Record> ReadRecord(Linker::Reader& rd);

			std::unique_ptr<Record> makeEND();
			std::unique_ptr<Record> makeCONST(std::shared_ptr<Linker::Image> image);
			std::unique_ptr<Record> makeCONST(size_t length);
			std::unique_ptr<Record> makeALIGN(offset_t align = 0);
			std::unique_ptr<Record> makeORG(offset_t value = 0);
			std::unique_ptr<Record> makeRELOC(uint8_t size, uint8_t shift, offset_t source, offset_t target);
			std::unique_ptr<Record> makeRELOC();
			std::unique_ptr<Record> makeINTERSEG(uint8_t size, uint8_t shift, offset_t source, uint16_t file_number, uint16_t segment_number, offset_t target);
			std::unique_ptr<Record> makeINTERSEG();
			std::unique_ptr<Record> makeUSING(std::string name = "");
			std::unique_ptr<Record> makeSTRONG(std::string name = "");
			std::unique_ptr<Record> makeGLOBAL();
			std::unique_ptr<Record> makeGLOBAL(std::string name, uint16_t line_length, int operation, uint16_t private_flag);
			std::unique_ptr<Record> makeGEQU();
			std::unique_ptr<Record> makeGEQU(std::string name, uint16_t line_length, int operation, uint16_t private_flag, std::unique_ptr<Expression> expression);
			std::unique_ptr<Record> makeMEM();
			std::unique_ptr<Record> makeMEM(offset_t start, offset_t end);
			std::unique_ptr<Record> makeEXPR();
			std::unique_ptr<Record> makeEXPR(uint8_t size, std::unique_ptr<Expression> expression);
			std::unique_ptr<Record> makeZEXPR();
			std::unique_ptr<Record> makeZEXPR(uint8_t size, std::unique_ptr<Expression> expression);
			std::unique_ptr<Record> makeBEXPR();
			std::unique_ptr<Record> makeBEXPR(uint8_t size, std::unique_ptr<Expression> expression);
			std::unique_ptr<Record> makeRELEXPR();
			std::unique_ptr<Record> makeRELEXPR(uint8_t size, offset_t origin, std::unique_ptr<Expression> expression);
			std::unique_ptr<Record> makeLOCAL();
			std::unique_ptr<Record> makeLOCAL(std::string name, uint16_t line_length, int operation, uint16_t private_flag);
			std::unique_ptr<Record> makeEQU();
			std::unique_ptr<Record> makeEQU(std::string name, uint16_t line_length, int operation, uint16_t private_flag, std::unique_ptr<Expression> expression);
			std::unique_ptr<Record> makeDS(offset_t count = 0);
			std::unique_ptr<Record> makeLCONST();
			std::unique_ptr<Record> makeLCONST(std::shared_ptr<Linker::Image> image);
			std::unique_ptr<Record> makeLEXPR();
			std::unique_ptr<Record> makeLEXPR(uint8_t size, std::unique_ptr<Expression> expression);
			std::unique_ptr<Record> makeENTRY();
			std::unique_ptr<Record> makeENTRY(uint16_t segment_number, offset_t location, std::string name);
			std::unique_ptr<Record> makecRELOC(uint8_t size, uint8_t shift, uint16_t source, uint16_t target);
			std::unique_ptr<Record> makecRELOC();
			std::unique_ptr<Record> makecINTERSEG(uint8_t size, uint8_t shift, uint16_t source, uint16_t segment_number, uint16_t target);
			std::unique_ptr<Record> makecINTERSEG();
			std::unique_ptr<Record> makeSUPER(SuperCompactRecord::super_record_type super_type = SuperCompactRecord::super_record_type(0));
		};

		std::vector<std::unique_ptr<Segment>> segments;

		void CalculateValues() override;
		void ReadFile(Linker::Reader& rd) override;
		using Linker::Format::WriteFile;
		offset_t WriteFile(Linker::Writer& wr) const override;
		offset_t ImageSize() const override;
		void Dump(Dumper::Dumper& dump) const override;
		/* TODO */
	};
}

#endif /* GSOS_H */
