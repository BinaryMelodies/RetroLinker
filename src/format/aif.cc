
#include "aif.h"

using namespace ARM;

void AIFFormat::Clear()
{
	// TODO
}

void AIFFormat::CalculateValues()
{
	// TODO
}

void AIFFormat::ReadFile(Linker::Reader& rd)
{
	rd.endiantype = endiantype;
	rd.Skip(4); // nop

	relocation_code = rd.ReadUnsigned(4);
	if((relocation_code >> 24) == ARM_BL_OP)
	{
		relocatable = true;
		relocation_code = ((relocation_code & 0x00FFFFFF) << 4) + 16;
	}
	else
	{
		if(relocation_code != ARM_NOP)
		{
			Linker::Error << "Error: invalid relocation code offset at second word" << std::endl;
			// TODO: print word
		}
		relocatable = false;
		relocation_code = 0;
	}

	zero_init_code = rd.ReadUnsigned(4);
	if((zero_init_code >> 24) == ARM_BL_OP)
	{
		has_zero_init = true;
		zero_init_code = ((zero_init_code & 0x00FFFFFF) << 4) + 24;
	}
	else
	{
		if(zero_init_code != ARM_NOP)
		{
			Linker::Error << "Error: invalid relocation code offset at second word" << std::endl;
			// TODO: print word
		}
		has_zero_init = false;
		zero_init_code = 0;
	}

	entry = rd.ReadUnsigned(4);
	if((entry >> 24) == ARM_BL_OP)
	{
		// BL instruction
		executable = true;
	}
	else if((entry >> 24) == 0)
	{
		executable = false;
	}
	else
	{
		Linker::Error << "Error: invalid entry at fourth word" << std::endl;
		// TODO: print word
		executable = true;
	}
	entry = (entry & 0x00FFFFFF) << 2;
	if(executable)
	{
		entry -= 108;
	}

	exit_instruction = rd.ReadUnsigned(4);

	text_size = rd.ReadUnsigned(4);
	data_size = rd.ReadUnsigned(4);
	debug_size = rd.ReadUnsigned(4);
	bss_size = rd.ReadUnsigned(4);
	image_debut_type = debug_type(rd.ReadUnsigned(4));
	image_base = rd.ReadUnsigned(4);
	workspace = rd.ReadUnsigned(4);
	address_mode = address_mode_type(rd.ReadUnsigned(4));
	data_base = rd.ReadUnsigned(4);

	/* TODO: rest of the header */

}

offset_t AIFFormat::WriteFile(Linker::Writer& wr)
{
	wr.endiantype = endiantype;
	wr.WriteWord(4, ARM_NOP);

	if(relocatable)
	{
		wr.WriteWord(4, ARM_BL + ((relocation_code - 16) >> 4));
	}
	else
	{
		wr.WriteWord(4, ARM_NOP);
	}

	if(has_zero_init)
	{
		wr.WriteWord(4, ARM_BL + ((zero_init_code - 24) >> 4));
	}
	else
	{
		wr.WriteWord(4, ARM_NOP);
	}

	if(executable)
	{
		wr.WriteWord(4, ARM_BL + ((entry + 108) >> 4));
	}
	else
	{
		wr.WriteWord(4, ARM_NOP);
	}

	wr.WriteWord(4, exit_instruction);

	wr.WriteWord(4, text_size);
	wr.WriteWord(4, data_size);
	wr.WriteWord(4, debug_size);
	wr.WriteWord(4, bss_size);
	wr.WriteWord(4, image_debut_type);
	wr.WriteWord(4, image_base);
	wr.WriteWord(4, workspace);
	wr.WriteWord(4, address_mode);
	wr.WriteWord(4, data_base);

	/* TODO: rest of the header */

	return offset_t(-1);
}

std::string AIFFormat::GetDefaultExtension(Linker::Module& module, std::string filename)
{
	return filename + ",ff8";
}

