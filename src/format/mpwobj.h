#ifndef _MPWOBJ_H
#define _MPWOBJ_H

#include <map>
#include <memory>
#include <vector>
#include <set>
#include <string>
#include "../linker/format.h"
#include "../linker/image.h"
#include "../linker/reader.h"
#include "../linker/writer.h"
#include "../dumper/dumper.h"
#include "apple.h"

// TODO: attempt at parsing MPW object files

// based on Macintosh Programmer's Workshop 3.0 Reference, Appendix H, Object File Format

namespace MPW
{
	class ObjectFormat : public Linker::Format
	{
	public:
		class Record
		{
		protected:
			static const std::map<offset_t, std::string> record_type_names;
		public:
			enum record_type
			{
				Pad = 0,
				First = 1,
				Last = 2,
				Comment = 3,
				Dictionary = 4,
				Module = 5,
				EntryPoint = 6,
				Size = 7,
				Contents = 8,
				Reference = 9,
				ComputedReference = 10,
				Filename = 11,
				SourceStatement = 12,
				ModuleBegin = 13,
				ModuleEnd = 14,
				BlockBegin = 15,
				BlockEnd = 16,
				LocalIdentifier = 17,
				LocalLabel = 18,
				LocalType = 19,
			};

			offset_t record_offset;
			record_type type;

			Record(record_type type = record_type(0), offset_t record_offset = 0)
				: record_offset(record_offset), type(type)
			{
			}

			virtual ~Record() = default;
			static std::shared_ptr<Record> ReadFile(const std::shared_ptr<Linker::Reader>& rd);
			virtual void ReadBody(const std::shared_ptr<Linker::Reader>& rd);
			virtual offset_t ImageSize() const;
			virtual offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const;
			virtual void Dump(Dumper::Dumper& dump, offset_t record_index) const;
		};

		class FirstRecord : public Record
		{
		public:
			FirstRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}

			static constexpr uint8_t Nested = 0x01;
			uint8_t flags = 0;

			enum version_type : uint16_t
			{
				Nonsymbolic_2_0 = 1,
				Nonsymbolic_3_0 = 2,
				Symbolic = 3,
			};
			version_type version = version_type(0);

			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		class CommentRecord : public Record
		{
		public:
			CommentRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}

			std::shared_ptr<Linker::Contents> comments;

			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		class DictionaryRecord : public Record
		{
		public:
			DictionaryRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}
			static constexpr uint8_t DebugFlag = 0x80;
			uint8_t flags = 0;
			uint16_t first_id = 0;
			std::vector<std::string> names;

			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		class ModuleRecord : public Record
		{
		public:
			ModuleRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}
			static constexpr uint8_t DataFlag = 0x01;
			static constexpr uint8_t ExternFlag = 0x08;
			static constexpr uint8_t MainFlag = 0x10;
			static constexpr uint8_t ForceActiveFlag = 0x80;

			uint8_t flags = 0;
			uint16_t module_id = 0;
			union
			{
				uint16_t id;
				uint16_t size;
			} segment = { };

			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		class EntryPointRecord : public Record
		{
		public:
			EntryPointRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}
			static constexpr uint8_t DataFlag = 0x01;
			static constexpr uint8_t ExternFlag = 0x08;
			static constexpr uint8_t MainFlag = 0x10;

			uint8_t flags = 0;
			uint16_t entry_id = 0;
			uint32_t offset = 0;

			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		class SizeRecord : public Record
		{
		public:
			SizeRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}
			static constexpr uint8_t DataFlag = 0x01;

			uint8_t flags = 0;
			uint32_t module_size = 0;

			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		class ContentsRecord : public Record
		{
		public:
			ContentsRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}
			static constexpr uint8_t DataFlag = 0x01;
			static constexpr uint8_t PartialFlag = 0x08;
			static constexpr uint8_t RepeatFlag = 0x10;

			uint8_t flags = 0;
			uint32_t offset = 0;
			uint16_t repeat = 0;
			std::shared_ptr<Linker::Contents> contents;

			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		class ReferenceRecord : public Record
		{
		public:
			ReferenceRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}
			static constexpr uint8_t DataFlag = 0x01;
			static constexpr uint8_t LongFlag = 0x08;
			static constexpr uint8_t _16Flag = 0x10;
			static constexpr uint8_t _8Flag = 0x20;
			static constexpr uint8_t A5RelativeFlag = 0x80;

			uint8_t flags = 0;
			uint16_t id = 0;
			uint16_t id2 = 0; // only for ComputedReference
			uint16_t repeat = 0;
			std::vector<uint32_t> offsets;

			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		class FilenameRecord : public Record
		{
		public:
			FilenameRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}
			uint16_t file_id = 0;
			::Timestamp<Macintosh_clock> modification;

			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		class SourceStatementRecord : public Record
		{
		public:
			SourceStatementRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}
			static constexpr uint8_t CodeOffset16Flag = 0x40;
			static constexpr uint8_t FileOffset16Flag = 0x80;

			uint8_t flags = 0;
			uint16_t parent_id = 0;
			uint16_t file_id = 0;
			std::set<std::tuple<uint32_t, uint32_t>> file_code_offsets;

			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		class ModuleBeginRecord : public Record
		{
		public:
			ModuleBeginRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}
			static constexpr uint8_t IsDeclarationFlag = 0x04;
			static constexpr uint8_t FileOffset16Flag = 0x80;

			uint8_t flags = 0;
			uint16_t module_id = 0;
			uint16_t parent_id = 0;
			uint16_t file_id = 0;
			uint32_t file_offset = 0;
			enum module_kind_type : uint8_t
			{
				None = 0,
				Unit = 2,
				Procedure = 3,
				Function = 4,
				DataModule = 5,
			};
			module_kind_type module_kind = module_kind_type(0);

			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		class ModuleEndRecord : public Record
		{
		public:
			ModuleEndRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}
			static constexpr uint8_t IsDeclarationFlag = 0x04;

			uint8_t flags = 0;
			uint16_t module_id = 0;
			uint32_t file_offset = 0;

			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		class BlockBeginRecord : public Record
		{
		public:
			BlockBeginRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}
			static constexpr uint8_t IsDeclarationFlag = 0x04;
			static constexpr uint8_t FileOffset16Flag = 0x80;

			uint8_t flags = 0;
			uint16_t block_id = 0;
			uint16_t parent_id = 0;
			uint16_t file_id = 0;
			uint16_t file_offset = 0;
			uint16_t code_offset = 0;

			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		class BlockEndRecord : public Record
		{
		public:
			BlockEndRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}
			uint16_t block_id = 0;
			uint16_t file_offset = 0;
			uint16_t code_offset = 0;

			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		class LocalIdentifierRecord : public Record
		{
		public:
			LocalIdentifierRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}
			struct Identifier
			{
				uint16_t id = 0;
				uint16_t type = 0;
				uint8_t file_delta = 0;
				enum storage_class_type : uint8_t
				{
					Register = 0,
					A5Relative = 1,
					A6Relative = 2,
					A7Relative = 3,
					Absolute = 4,
					Constant = 5,
				};
				storage_class_type storage_class = storage_class_type(0);
				bool value = false;
				bool reference = false;
				enum offset_size_type : uint8_t
				{
					NoOffset = 0,
					Offset16 = 1,
					Offset32 = 2,
					VariableOffset = 3,
				};
				offset_size_type offset_size = offset_size_type(0);
				std::vector<uint8_t> offset_data;
			};

			static constexpr uint8_t FileOffset16Flag = 0x80;

			uint8_t flags = 0;
			uint16_t parent_id = 0;
			uint16_t file_id = 0;
			uint32_t file_offset = 0;
			std::vector<Identifier> identifiers;

			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		class LocalLabelRecord : public Record
		{
		public:
			LocalLabelRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}

			static constexpr uint8_t IsEntryFlag = 0x08;
			static constexpr uint8_t CodeOffset16Flag = 0x40;
			static constexpr uint8_t FileOffset16Flag = 0x80;

			uint8_t flags = 0;
			uint16_t parent_id = 0;
			uint16_t file_id = 0;
			std::map<std::tuple<uint32_t, uint32_t>, uint16_t> label_ids;

			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		class LocalTypeRecord : public Record
		{
		public:
			LocalTypeRecord(record_type type = record_type(0), offset_t record_offset = 0)
				: Record(type, record_offset)
			{
			}
			// TODO:
			void ReadBody(const std::shared_ptr<Linker::Reader>& rd) override;
			offset_t ImageSize() const override;
			offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
			void Dump(Dumper::Dumper& dump, offset_t record_index) const override;
		};

		std::vector<std::shared_ptr<Record>> records;

		void ReadFile(const std::shared_ptr<Linker::Reader>& rd) override;
		offset_t ImageSize() const override;
		offset_t WriteFile(const std::shared_ptr<Linker::Writer>& wr) const override;
		void Dump(Dumper::Dumper& dump) const override;
	};
}

#endif /* _MPWOBJ_H */
