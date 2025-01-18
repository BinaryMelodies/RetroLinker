#ifndef GSOS_H
#define GSOS_H

#include "../common.h"
#include "../linker/linker.h"
#include "../linker/module.h"
#include "../linker/segment.h"
#include "../linker/writer.h"

/* TODO: untested */

namespace Apple
{
	/**
	 * @brief Apple GS/OS OMF file format
	 *
	 * The file format was originally invented for ORCA/M as an object format, later adopted for the Apple GS/OS operating system.
	 * It had multiple versions, including a few early versions, version 1, 2 and 2.1.
	 */
	class OMFFormat : public virtual Linker::LinkerManager
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

			/** @brief Reads a number the size of number_length */
			offset_t ReadUnsigned(Linker::Reader& rd);
			/** @brief Writes a string the size of number_length */
			void WriteWord(Linker::Writer& wr, offset_t value);

			/** @brief Reads a string the size of label_length, or variable length if it is 0 */
			std::string ReadLabel(Linker::Reader& rd);
			/** @brief Writes a string the size of label_length, or variable length if it is 0 */
			void WriteLabel(Linker::Writer& wr, std::string text);

			/** @brief Calculates the values needed to generate segment images
			 *
			 * @param segment_number The index of the current segment (1 based)
			 * @param current_offset Offset to current segment
			 * @return Offset to end of segment
			 */
			offset_t CalculateValues(uint16_t segment_number, offset_t current_offset);
			void ReadFile(Linker::Reader& rd);
			void WriteFile(Linker::Writer& wr);

			class Expression
			{
			public:
				/* TODO */
				virtual offset_t GetLength(Segment& segment) = 0;
				virtual void ReadFile(Segment& segment, Linker::Reader& rd) = 0;
				virtual void WriteFile(Segment& segment, Linker::Writer& wr) = 0;
			};

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

				virtual ~Record();
				virtual offset_t GetLength(Segment& segment);
				virtual void ReadFile(Segment& segment, Linker::Reader& rd);
				virtual void WriteFile(Segment& segment, Linker::Writer& wr);
			};

			class DataRecord : public Record
			{
			public:
				std::vector<uint8_t> data;

				DataRecord(record_type type, std::vector<uint8_t> data)
					: Record(type), data(data)
				{
				}

				DataRecord(record_type type, size_t length)
					: Record(type), data(length, 0)
				{
				}

				DataRecord(record_type type)
					: Record(type)
				{
				}

				offset_t GetLength(Segment& segment) override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(Segment& segment, Linker::Writer& wr) override;
			};

			class ValueRecord : public Record
			{
			public:
				offset_t value;

				ValueRecord(record_type type, offset_t value)
					: Record(type), value(value)
				{
				}

				offset_t GetLength(Segment& segment) override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(Segment& segment, Linker::Writer& wr) override;
			};

			class StringRecord : public Record
			{
			public:
				std::string value;

				StringRecord(record_type type, std::string value)
					: Record(type), value(value)
				{
				}

				offset_t GetLength(Segment& segment) override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(Segment& segment, Linker::Writer& wr) override;
			};

			class RelocationRecord : public Record
			{
			public:
				uint8_t size = 0;
				uint8_t shift = 0;
				offset_t source = 0;
				offset_t target = 0;

				RelocationRecord(record_type type)
					: Record(type)
				{
				}

				RelocationRecord(record_type type, uint8_t size, uint8_t shift, offset_t source, offset_t target)
					: Record(type), size(size), shift(shift), source(source), target(target)
				{
				}

				offset_t GetLength(Segment& segment) override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(Segment& segment, Linker::Writer& wr) override;
			};

			class IntersegmentRelocationRecord : public RelocationRecord
			{
			public:
				uint16_t file_number = 0;
				uint16_t segment_number = 0;

				IntersegmentRelocationRecord(record_type type)
					: RelocationRecord(type)
				{
				}

				IntersegmentRelocationRecord(record_type type, uint8_t size, uint8_t shift, offset_t source, uint16_t file_number, uint16_t segment_number, offset_t target)
					: RelocationRecord(type, size, shift, source, target), file_number(file_number), segment_number(segment_number)
				{
				}

				offset_t GetLength(Segment& segment) override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(Segment& segment, Linker::Writer& wr) override;
			};

			class LabelRecord : public Record
			{
			public:
				std::string name;
				uint16_t line_length = 0;
				enum operation_type
				{
					OP_ADDRESS_DC = 'A',
					// TODO: rest
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

				offset_t GetLength(Segment& segment) override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(Segment& segment, Linker::Writer& wr) override;
			};

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

				offset_t GetLength(Segment& segment) override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(Segment& segment, Linker::Writer& wr) override;
			};

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

				offset_t GetLength(Segment& segment) override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(Segment& segment, Linker::Writer& wr) override;
			};

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

				offset_t GetLength(Segment& segment) override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(Segment& segment, Linker::Writer& wr) override;
			};

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

				offset_t GetLength(Segment& segment) override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(Segment& segment, Linker::Writer& wr) override;
			};

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

				offset_t GetLength(Segment& segment) override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(Segment& segment, Linker::Writer& wr) override;
			};

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
				};

				super_record_type super_type = super_record_type(0);

				std::vector<uint16_t> offsets;

				SuperCompactRecord(record_type type, super_record_type super_type = super_record_type(0))
					: Record(type), super_type(super_type)
				{
				}

				offset_t GetLength(Segment& segment) override;
				void ReadFile(Segment& segment, Linker::Reader& rd) override;
				void WriteFile(Segment& segment, Linker::Writer& wr) override;
			private:
				void WritePatchList(Linker::Writer& wr, const std::vector<uint8_t>& patches);
			};

			std::unique_ptr<Expression> ReadExpression(Linker::Reader& rd);

			std::unique_ptr<Record> ReadRecord(Linker::Reader& rd);

			std::unique_ptr<Record> makeEND();
			std::unique_ptr<Record> makeCONST(std::vector<uint8_t> data);
			std::unique_ptr<Record> makeCONST(std::vector<uint8_t> data, size_t length);
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
			std::unique_ptr<Record> makeLCONST(std::vector<uint8_t> data);
			std::unique_ptr<Record> makeLCONST(std::vector<uint8_t> data, size_t length);
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
		void WriteFile(Linker::Writer& wr) override;
		/* TODO */
	};
}

#endif /* GSOS_H */
