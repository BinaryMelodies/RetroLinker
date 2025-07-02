#ifndef TABLE_SECTION_H
#define TABLE_SECTION_H

#include "../common.h"
#include "reader.h"
#include "section.h"
#include "writer.h"

namespace Linker
{
	template <typename TableEntryType>
		class TableSection : public Section
	{
	public:
		::EndianType endian_type = ::UndefinedEndian;
		std::vector<TableEntryType> entries;

		TableSection(::EndianType endian_type, std::string name, int flags = Readable)
			: Section(name, flags), endian_type(endian_type)
		{
			assert(!(flags & ZeroFilled));
		}

		TableSection(std::string name, int flags = Readable)
			: Section(name, flags)
		{
			assert(!(flags & ZeroFilled));
		}

		offset_t Size() const override
		{
			return entries.size() * TableEntryType::EntrySize + data.size();
		}

		offset_t SetZeroFilled(bool is_zero_filled) override
		{
			assert(!is_zero_filled);
			return 0;
		}

		void ReadFile(std::istream& in) override
		{
			for(auto& entry : entries)
			{
				TableEntryType::ReadFile(in, entry, endian_type);
			}
			Section::ReadFile(in);
		}

		void ReadFile(Reader& rd) override
		{
			for(auto& entry : entries)
			{
				TableEntryType::ReadFile(rd, entry, endian_type);
			}
			Section::ReadFile(rd);
		}

		template <typename OutputType>
			offset_t WriteTable(OutputType& out, offset_t bytes, offset_t offset) const
		{
			if(offset / TableEntryType::EntrySize >= entries.size())
				return 0;

			bytes = std::min(bytes, entries.size() * TableEntryType::EntrySize - offset);

			offset_t output_byte_count = 0;
			offset_t offset_in_entry = offset % TableEntryType::EntrySize;
			if(offset_in_entry != 0)
			{
				if(offset + bytes >= TableEntryType::EntrySize)
				{
					// write first partial entry
					TableEntryType::WriteFile(out, entries[offset / TableEntryType::EntrySize],
						endian_type,
						TableEntryType::EntrySize,
						offset_in_entry);
					output_byte_count += TableEntryType::EntrySize - offset_in_entry;
					offset += TableEntryType::EntrySize - offset_in_entry;
				}
				else
				{
					// write only partial entry
					TableEntryType::WriteFile(out, entries[offset / TableEntryType::EntrySize],
						endian_type,
						bytes - offset_in_entry,
						offset_in_entry);
					output_byte_count += bytes;
					offset += bytes;
					bytes = 0;
				}
			}

			for(
				offset_t entry_index = offset / TableEntryType::EntrySize;
				entry_index < entries.size() && entry_index < (offset + bytes) / TableEntryType::EntrySize;
				entry_index++)
			{
				// write full entries
				TableEntryType::WriteFile(out, entries[entry_index], endian_type);
				output_byte_count += TableEntryType::EntrySize;
			}

			if(offset / TableEntryType::EntrySize < entries.size())
			{
				offset_t bytes_in_entry = bytes % TableEntryType::EntrySize;
				if(bytes_in_entry != 0)
				{
					// write last partial entry
					TableEntryType::WriteFile(out, entries[offset / TableEntryType::EntrySize],
						endian_type,
						bytes_in_entry,
						offset);
					output_byte_count += bytes;
				}
			}

			return output_byte_count;
		}

		using Section::WriteFile;

		offset_t WriteFile(std::ostream& out, offset_t bytes, offset_t offset = 0) const override
		{
			offset_t table_bytes = WriteTable(out, bytes, offset);
			return table_bytes + Section::WriteFile(out, bytes - table_bytes, offset + table_bytes);
		}

		offset_t WriteFile(Writer& wr, offset_t bytes, offset_t offset = 0) const override
		{
			offset_t table_bytes = WriteTable(wr, bytes, offset);
			return table_bytes + Buffer::WriteFile(wr, bytes - table_bytes, offset + table_bytes);
		}

		void Reset() override
		{
			Section::Reset();
			entries.clear();
		}
	};

	template <typename Int>
		struct Word
	{
	public:
		static constexpr offset_t EntrySize = sizeof(Int);

		static void ReadFile(std::istream& in, Word& word, ::EndianType endian_type)
		{
			Reader rd(endian_type, &in);
			ReadFile(rd, word);
		}

		static void ReadFile(Reader& rd, Word& word, ::EndianType endian_type = ::UndefinedEndian)
		{
			(void) endian_type; // ignored
			word.value = rd.ReadUnsigned(EntrySize); // TODO: ReadSigned for signed words
		}

		static void WriteFile(std::ostream& out, const Word& word, ::EndianType endian_type, offset_t bytes = EntrySize, offset_t offset = 0)
		{
			Writer wr(endian_type, &out);
			WriteFile(wr, word, endian_type, bytes, offset);
		}

		static void WriteFile(Writer& wr, const Word& word, ::EndianType endian_type = ::UndefinedEndian, offset_t bytes = EntrySize, offset_t offset = 0)
		{
			(void) endian_type; // ignored
			// TODO: not dealing with partial entries for now
			assert(bytes == EntrySize);
			assert(offset == 0);
			wr.WriteWord(EntrySize, word.value);
		}

		Int value;
	};
}

#endif /* TABLE_SECTION_H */
