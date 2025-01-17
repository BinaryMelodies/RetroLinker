
#include <set>
#include <sstream>
#include "linker.h"
#include "module.h"
#include "position.h"
#include "relocation.h"
#include "target.h"
#include "writer.h"

using namespace Linker;
using namespace Script;

EndianType DefaultEndianType = LittleEndian;

void LinkerManager::ClearLinkerManager()
{
	segment_map.clear();
	segment_vector.clear();
	linker_parameters.clear();
}

void LinkerManager::SetLinkScript(std::string script_file, std::map<std::string, std::string>& options)
{
	if(script_file != "")
	{
		Linker::Debug << "Debug: setting linker script to " << script_file << std::endl;
	}

	linker_script = script_file;

	for(auto it : options)
	{
		try
		{
			offset_t value = std::stoll(it.second, nullptr, 0);
			linker_parameters[it.first] = Linker::Location(value);
			Linker::Debug << "Debug: Setting linker parameter " << it.first << " to " << value << std::endl;
		}
		catch(std::invalid_argument& a)
		{
			Linker::Error << "Error: Unable to parse " << it.second << ", ignoring" << std::endl;
		}
	}
}

std::unique_ptr<Script::List> LinkerManager::GetScript(Linker::Module& module)
{
	bool file_error = false;
	std::unique_ptr<Script::List> list = Script::parse_file(linker_script.c_str(), file_error);
	if(file_error)
	{
		std::ostringstream message;
		message << "Fatal error: Unable to open linker script " << linker_script;
		Linker::FatalError(message.str());
	}
	else if(list == nullptr)
	{
		std::ostringstream message;
		message << "Fatal error: Unable to parse linker script " << linker_script;
		Linker::FatalError(message.str());
	}
	return list;
}

offset_t LinkerManager::GetCurrentAddress() const
{
	if(current_segment == nullptr)
	{
		return current_address;
	}
	else
	{
		return current_segment->GetEndAddress();
	}
}

void LinkerManager::SetCurrentAddress(offset_t address)
{
	if(current_segment == nullptr)
	{
		current_address = address;
	}
	else if(current_segment->sections.size() == 0)
	{
		current_segment->base_address = address;
	}
	else
	{
		current_segment->SetEndAddress(address);
	}
}

void LinkerManager::AlignCurrentAddress(offset_t align)
{
	SetCurrentAddress(::AlignTo(GetCurrentAddress(), align));
}

void LinkerManager::SetLatestBase(offset_t address)
{
	assert(current_segment != nullptr && current_segment->sections.size() != 0);
	std::shared_ptr<Section> section = current_segment->sections.back();
	section->bias = section->GetStartAddress() - address;
}

void LinkerManager::FinishCurrentSegment()
{
	if(current_segment != nullptr)
	{
		current_address = current_segment->GetEndAddress();
		OnNewSegment(current_segment);
		current_segment = nullptr;
	}
}

void LinkerManager::OnNewSegment(std::shared_ptr<Segment> segment)
{
	/* extend */
}

std::shared_ptr<Segment> LinkerManager::AppendSegment(std::string name)
{
	FinishCurrentSegment();
	current_segment = std::make_shared<Segment>(name, current_address);
	segment_vector.push_back(current_segment);
	segment_map[name] = current_segment;
	return current_segment;
}

std::shared_ptr<Segment> LinkerManager::FetchSegment(std::string name)
{
	auto it = segment_map.find(name);
	if(it == segment_map.end())
	{
		return nullptr;
	}
	else
	{
		return it->second;
	}
}

void LinkerManager::AppendSection(std::shared_ptr<Section> section)
{
	current_segment->Append(section); /* next_bias should not be used */
	SetLatestBase(current_base);
}

void LinkerManager::ProcessScript(std::unique_ptr<List>& directives, Module& module)
{
	for(auto& directive : directives->children)
	{
		switch(directive->type)
		{
		case Node::Sequence:
			for(auto& action : directive->list->children)
			{
				ProcessAction(action, module);
			}
			break;
		case Node::Assign:
			/* TODO */
			break;
		case Node::Segment:
			current_is_template = current_is_template_head = false;
			AppendSegment(*directive->value->Get<std::string>());
			for(auto& command : directive->at(0)->list->children)
			{
				ProcessCommand(command, module);
			}
			for(auto& action : directive->at(1)->list->children)
			{
				PostProcessAction(action, module);
			}
			break;
		case Node::SegmentTemplate:
			current_is_template = false;
			current_is_template_head = true;
			template_counter = 0;
			for(auto& section : module.Sections())
			{
				if(section->segment.use_count() != 0) // TODO
					continue;
				current_template_name = section->name;
				if(!CheckPredicate(directive->at(0), section, module))
					continue;
				current_is_template = true;
				AppendSegment(current_template_name);
				for(auto& command : directive->at(1)->list->children)
				{
					ProcessCommand(command, module);
				}
				for(auto& action : directive->at(2)->list->children)
				{
					PostProcessAction(action, module);
				}
				template_counter += 1;
			}
			break;
		default:
			Linker::FatalError("Internal error: invalid script");
		}
	}
	FinishCurrentSegment();
}

void Linker::LinkerManager::ProcessAction(std::unique_ptr<Node>& action, Module& module)
{
	switch(action->type)
	{
	case Node::SetCurrentAddress:
		SetCurrentAddress(EvaluateExpression(action->at(0), module));
		break;
	case Node::AlignAddress:
		AlignCurrentAddress(EvaluateExpression(action->at(0), module));
		break;
	case Node::SetNextBase:
		current_base = EvaluateExpression(action->at(0), module);
Linker::Debug << "Current base: " << int64_t(current_base) << std::endl;
		break;
	default:
		Linker::FatalError("Internal error: invalid script");
	}
}

void Linker::LinkerManager::PostProcessAction(std::unique_ptr<Node>& action, Module& module)
{
	switch(action->type)
	{
	case Node::SetCurrentAddress:
		segment_vector.back()->SetStartAddress(EvaluateExpression(action->at(0), module));
		break;
	case Node::AlignAddress:
		/* TODO */
		break;
	case Node::SetNextBase:
		Linker::FatalError("Fatal error: Script: Invalid use of `base` directive in postprocessing");
	default:
		Linker::FatalError("Internal error: invalid script");
	}
}

void Linker::LinkerManager::ProcessCommand(std::unique_ptr<Node>& command, Module& module)
{
	switch(command->type)
	{
	case Node::Sequence:
		for(auto& action : command->list->children)
		{
			ProcessAction(action, module);
		}
		break;
	case Node::Collect:
		for(auto& section : module.Sections())
		{
			if(section->segment.use_count() != 0) // TODO
				continue;
			if(!CheckPredicate(command->at(0), section, module))
				continue;
			for(auto& action : command->at(1)->list->children)
			{
				ProcessAction(action, module);
			}
			AppendSection(section);
		}
		break;
	case Node::Assign:
		Linker::FatalError("Internal error: unimplemented");
		break;
	default:
		Linker::FatalError("Internal error: invalid script");
	}
}

bool Linker::LinkerManager::CheckPredicate(std::unique_ptr<Node>& predicate, std::shared_ptr<Section> section, Module& module)
{
	switch(predicate->type)
	{
	case Node::Collect:
		if(!CheckPredicate(predicate->at(0), section, module))
			return false;
		for(auto& action : predicate->at(1)->list->children)
		{
			ProcessAction(action, module);
		}
		AppendSection(section);
		return false; /* already appended */
	case Node::MaximumSections:
		return template_counter < EvaluateExpression(predicate->at(1), module)
			&& EvaluateExpression(predicate->at(0), module);
	case Node::OrPredicate:
		return
			CheckPredicate(predicate->at(0), section, module)
			|| CheckPredicate(predicate->at(1), section, module);
	case Node::AndPredicate:
		return
			CheckPredicate(predicate->at(0), section, module)
			&& CheckPredicate(predicate->at(1), section, module);
	case Node::NotPredicate:
		return !CheckPredicate(predicate->at(0), section, module);
	case Node::MatchAny:
		if(current_is_template_head)
		{
			return true;
		}
		else if(current_is_template)
		{
			return current_template_name == section->name;
		}
		else
		{
			return true;
		}
	case Node::MatchName:
		return section->name == *predicate->value->Get<std::string>();
	case Node::MatchSuffix:
		{
			std::string suffix = *predicate->value->Get<std::string>();
			if(current_is_template_head)
			{
				if(ends_with(section->name, suffix))
				{
					current_template_name = section->name.substr(0, suffix.size());
					return true;
				}
				else
				{
					return false;
				}
			}
			else if(current_is_template)
			{
				return current_template_name + suffix == section->name;
			}
			else
			{
				return ends_with(section->name, suffix);
			}
		}
	case Node::IsReadable:
		return section->IsReadable();
	case Node::IsWritable:
		return section->IsWritable();
	case Node::IsExecutable:
		return section->IsExecable();
	case Node::IsMergeable:
		return section->IsMergeable();
	case Node::IsZeroFilled:
		return section->IsZeroFilled();
	case Node::IsFixedAddress:
		return section->IsFixed();
	case Node::IsResource:
		return (section->GetFlags() & Section::Resource) != 0;
	case Node::IsOptional:
		return (section->GetFlags() & Section::Optional) != 0;
	case Node::IsStack:
		return (section->GetFlags() & Section::Stack) != 0;
	case Node::IsHeap:
		return (section->GetFlags() & Section::Heap) != 0;
	case Node::IsCustomFlag:
		return (section->GetFlags() & EvaluateExpression(predicate->at(0), module)) != 0;
	default:
		Linker::FatalError("Internal error: invalid script");
	}
}

offset_t Linker::LinkerManager::EvaluateExpression(std::unique_ptr<Node>& expression, Module& module)
{
	switch(expression->type)
	{
	case Node::Integer:
		return *expression->value->Get<long>();
	case Node::Parameter:
		{
			auto it = linker_parameters.find(*expression->value->Get<std::string>());
			if(it == linker_parameters.end())
			{
				return 0;
			}
			else
			{
				return it->second.offset; /* TODO: use Location instead of offset_t */
			}
		}
		break;
	case Node::Identifier:
		/* TODO */
		return 0;
	case Node::BaseOf:
		/* TODO */
		return 0;
	case Node::StartOf:
		return segment_map[*expression->value->Get<std::string>()]->base_address;
	case Node::SizeOf:
		return segment_map[*expression->value->Get<std::string>()]->TotalSize();
	case Node::CurrentAddress:
		return GetCurrentAddress();
	case Node::AlignTo:
		return ::AlignTo(EvaluateExpression(expression->at(0), module),
			EvaluateExpression(expression->at(1), module));
	case Node::Maximum:
		{
			offset_t value = 0;
			for(auto& arg : expression->list->children)
			{
				offset_t new_value = EvaluateExpression(arg, module);
				if(new_value > value)
					value = new_value;
			}
			return value;
		}
	case Node::Minimum:
		{
			offset_t value = -1;
			for(auto& arg : expression->list->children)
			{
				offset_t new_value = EvaluateExpression(arg, module);
				if(new_value < value)
					value = new_value;
			}
			return value;
		}
	case Node::Neg:
		return -EvaluateExpression(expression->at(0), module);
	case Node::Not:
		return ~EvaluateExpression(expression->at(0), module);
	case Node::Shl:
		return EvaluateExpression(expression->at(0), module)
			<< EvaluateExpression(expression->at(1), module);
	case Node::Shr:
		return EvaluateExpression(expression->at(0), module)
			>> EvaluateExpression(expression->at(1), module);
	case Node::Add:
		return EvaluateExpression(expression->at(0), module)
			+ EvaluateExpression(expression->at(1), module);
	case Node::Sub:
		return EvaluateExpression(expression->at(0), module)
			- EvaluateExpression(expression->at(1), module);
	case Node::And:
		return EvaluateExpression(expression->at(0), module)
			& EvaluateExpression(expression->at(1), module);
	case Node::Xor:
		return EvaluateExpression(expression->at(0), module)
			^ EvaluateExpression(expression->at(1), module);
	case Node::Or:
		return EvaluateExpression(expression->at(0), module)
			| EvaluateExpression(expression->at(1), module);
	case Node::Location:
		Linker::FatalError("Script: Invalid use of location in expression context");
	default:
		Linker::FatalError("Internal error: invalid expression type");
	}
}

bool Linker::starts_with(std::string str, std::string start)
{
	return str.size() >= start.size() && str.substr(0, start.size()) == start;
}

bool Linker::ends_with(std::string str, std::string end)
{
	return str.size() >= end.size() && str.substr(str.size() - end.size()) == end;
}

