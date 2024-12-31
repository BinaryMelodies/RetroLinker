#! /usr/bin/python3

import os
import sys
import subprocess
import pathlib

ENV = dict(os.environ)

class Process:
	def __init__(self, *steps):
		self.steps = list(steps)
		self.updated = set()

	def append(self, step):
		self.steps.append(step)

	def needs_to_run(self, inputs, outputs):
		for file in inputs:
			if file in self.updated:
				return True
		earliest = None
		for file in outputs:
			if not os.path.exists(file):
				return True
			time = os.path.getmtime(file)
			if earliest is None or earliest > time:
				earliest = time
		for file in inputs:
			time = os.path.getmtime(file)
			if time > earliest:
				return True
		return False

	def create_directories(self, path):
		dirs = os.path.split(path)[0]
		if dirs != '':
			os.makedirs(dirs, exist_ok = True)

	def execute_step(self, step):
		global ENV
		if step.force or self.needs_to_run(step.input, step.output):
			for file in step.output:
				self.updated.add(file)
				self.create_directories(file)
			print("[CMD] >>> " + ' '.join(step.arguments), file = sys.stderr)
			result = subprocess.run(step.arguments, env = ENV)
			if result.returncode != 0:
				print(f"[ERR] >>> Exited with return code {result.returncode}", file = sys.stderr)

	def execute(self):
		for step in self.steps:
			self.execute_step(step)

class ProcessStep:
	"""The actual step that the environment can execute."""
	def __init__(self, Input = (), Output = (), Arguments = (), Force = False):
		self.input = tuple(Input)
		self.output = tuple(Output)
		self.arguments = tuple(Arguments)
		self.force = Force

class Recipe:
	"""Describes the target independent process. Instantiating this class will also register it."""

	RECIPES = []
	def __init__(self, steps, register = True):
		self.steps = steps
		self.targets = []
		if register:
			Recipe.RECIPES.append(self)

	def generate_steps(self, **keywords):
		"""Creates the target specific steps actually required to execute the recipe."""

		steps = []
		remaining_steps = self.steps
		while len(remaining_steps) > 0:
			current_step, remaining_steps = remaining_steps[0].generate_arguments(remaining_steps[1:], **keywords)
			steps += current_step
		return steps

def ext_format(text, **keywords):
	argument = ""
	ix = 0
	ix1 = text.find('{', ix)
	while ix1 != -1:
		ix2 = text.find('}', ix1)
		assert ix2 != -1
		argument += text[ix:ix1]
		argument += eval(text[ix1 + 1:ix2], keywords)
		ix = ix2 + 1
		ix1 = text.find('{', ix)
	argument += text[ix:]
	return argument

class RecipeStep:
	"""Describes a target independent step to be executed. Typically, an environment command is executed, and the arguments can be parametrized."""
	class StepArgument:
		def __init__(self, text):
			self.text = text

		def process(self, *remaining_arguments, **keywords):
			return [ext_format(self.text, **keywords)], remaining_arguments

	def __init__(self, Input = (), Output = (), Arguments = (), Force = False):
		self.arguments = list(RecipeStep.StepArgument(argument) if type(argument) is str else argument for argument in Arguments)
		self.input = tuple(Input) if type(Input) in {list, tuple} else (Input,)
		self.output = tuple(Output) if type(Output) in {list, tuple} else (Output,)
		self.force = Force or Input == ()

	def get_inputs(self, **keywords):
		return self.input

	def get_outputs(self, **keywords):
		return self.output

	def generate_arguments(self, remaining_steps, **keywords):
		arguments = []
		remaining_arguments = self.arguments
		while len(remaining_arguments) > 0:
			current_arguments, remaining_arguments = remaining_arguments[0].process(*remaining_arguments[1:], **keywords)
			arguments += current_arguments
		return [ProcessStep(
			Input = [node for param in self.get_inputs(**keywords) for node in ext_format(param, **keywords).split(' ')],
			Output = [node for param in self.get_outputs(**keywords) for node in ext_format(param, **keywords).split(' ')],
			Arguments = [portion.strip() for argument in arguments for portion in argument.split(' ') if portion.strip() != ''],
			Force = self.force)], remaining_steps

class Assemble(RecipeStep):
	"""A step describing assembling, producing an object file."""
	def __init__(self, Input, Output, Executable = (), Arguments = [], Dependency = []):
		super(Assemble, self).__init__(Input = [Input] + Dependency, Output = Output, Arguments = ["{assembler}", Input, "-o", Output] + Arguments)
		#super(Assemble, self).__init__(Input = [Input] + Dependency, Output = Executable, Arguments = ["{assembler}", Input, "-o", Output] + Arguments)

class CompileC(RecipeStep):
	"""A step describing compiling a C file, producing an object file."""
	def __init__(self, Input, Output, Executable = (), Arguments = [], Dependency = []):
		#super(CompileC, self).__init__(Input = [Input] + Dependency, Output = Output, Arguments = ["{compiler}", Arguments[0], Input, "-o", Output] + Arguments[1:])
		super(CompileC, self).__init__(Input = [Input] + Dependency, Output = Output, Arguments = ["{compiler}{c_arguments}", Input, "-o", Output] + Arguments)
		#super(CompileC, self).__init__(Input = [Input] + Dependency, Output = Executable, Arguments = ["{compiler}", Input, "-o", Output] + Arguments)

class Link(RecipeStep):
	"""A step describing invoking the linker, producing an output (typically executable) file."""
	def __init__(self, Format, Input, Output, Arguments, Dependency = []):
		if type(Input) is tuple:
			Input = list(Input)
		elif type(Input) is not list:
			Input = [Input]
		super(Link, self).__init__(Input = Input + Dependency, Output = Output, Arguments = ["../../link", f"-F{Format}"] + Input + ["-o", Output] + Arguments)
		#super(Link, self).__init__(Input = Dependency, Output = Output, Arguments = ["../../link", f"-F{Format}"] + Input + ["-o", Output] + Arguments)

	def get_outputs(self, **keywords):
		return super(Link, self).get_outputs(**keywords) + keywords.get('additional_outputs', ())

class Copy(RecipeStep):
	def __init__(self, Source, Destination, Arguments = []):
		super(Copy, self).__init__(Input = Source, Output = Destination, Arguments = ["cp"] + Arguments + [Source, Destination])

class IfDefined:
	"""In place of an argument, provides conditional arguments."""
	def __init__(self, name):
		self.name = name

	def process(self, *remaining_arguments, **keywords):
		global EndIf
		arguments = []
		if keywords.get(self.name):
			while len(remaining_arguments) > 0 and remaining_arguments[0] is not EndIf:
				current_arguments, remaining_arguments = remaining_arguments[0].process(*remaining_arguments[1:], **keywords)
				arguments += current_arguments
		if EndIf in remaining_arguments:
			index = remaining_arguments.index(EndIf)
			remaining_arguments = remaining_arguments[index + 1:]
		else:
			remaining_arguments = ()
		return arguments, remaining_arguments

class ForAll:
	"""In place of an argument, provides a list of arguments."""
	def __init__(self, name, In = None):
		assert In is not None
		self.name = name
		self.collection = In

	def process(self, *remaining_arguments, **keywords):
		global EndFor
		arguments = []
		new_keywords = keywords.copy()
		for entry in keywords.get(self.collection, ()):
			new_keywords[self.name] = entry
			new_remaining_arguments = remaining_arguments
			while len(new_remaining_arguments) > 0 and new_remaining_arguments[0] is not EndFor:
				current_arguments, new_remaining_arguments = new_remaining_arguments[0].process(*new_remaining_arguments[1:], **new_keywords)
				arguments += current_arguments
		if EndFor in remaining_arguments:
			index = remaining_arguments.index(EndFor)
			remaining_arguments = remaining_arguments[index + 1:]
		else:
			remaining_arguments = ()
		return arguments, remaining_arguments

	def generate_arguments(self, remaining_steps, **keywords):
		global EndFor
		steps = []
		arguments = []
		new_keywords = keywords.copy()
		for entry in keywords.get(self.collection, ()):
			new_keywords[self.name] = entry
			new_remaining_steps = remaining_steps
			while len(new_remaining_steps) > 0 and new_remaining_steps[0] is not EndFor:
				current_step, new_remaining_steps = new_remaining_steps[0].generate_arguments(new_remaining_steps[1:], **new_keywords)
				steps += current_step
		if EndFor in remaining_steps:
			index = remaining_steps.index(EndFor)
			remaining_steps = remaining_steps[index + 1:]
		else:
			remaining_steps = ()
		return steps, remaining_steps

class EndIf:
	"""In place of an argument, marks the end of a conditional argument."""
	pass
EndIf = EndIf()

class EndFor:
	"""In place of an argument, marks the end of an iterated argument."""
	pass
EndFor = EndFor()

class DefineVersion:
	"""Provides variation in executable generation where the file format does not change significantly. Typically used for linking model or relocation suppression."""
	def __init__(self, Id, ModelName = None, LinkerName = None, AssemblerOptions = (), LinkerOptions = ()):
		"""
			Id - version is identified by this
			ModelName - passed to assembler as symbol MODEL_*
			LinkerName - passed to linker as -M*, default: Id
		"""
		self.id = Id
		self.model_name = ModelName
		self.linker_name = LinkerName if LinkerName is not None else Id
		self.assembler_options = tuple(AssemblerOptions)
		self.linker_options = tuple(LinkerOptions)

class DefineTarget:
	"""Registers a new target, for CPU/OS/binary format. Instantiating this class automatically registers the target for compilation."""
	SYSTEMS = {}
	TARGETS = {}
	def __init__(self, CPU, System, TargetName = None, IncludeName = None, Format = None, FormatName = None, LinkerName = None, Versions = (), PostProcess = None, **keywords):
		"""
			System, Format - target is identified by this pair
			TargetName - passed to assembler as symbol TARGET_*, default: System value
			IncludeName - depends on include/*.inc, default: System value
			FormatName - passed to assembler as symbol FORMAT_*, default: Format value
			LinkerName - the name of the format, as understood by the linker, passed as -F*, default: Format value or System value
		"""
		self.cpu = CPU
		self.system = System
		self.target_name = TargetName if TargetName is not None else System
		self.include_name = IncludeName if IncludeName is not None else System
		self.format = Format
		self.format_name = FormatName if FormatName is not None else Format
		self.linker_name = LinkerName if LinkerName is not None else Format if Format is not None else System
		self.versions = {version.id: version for version in Versions}
		if len(Versions) > 0 and "" not in self.versions:
			self.versions[""] = Versions[0]
		self.post_process = PostProcess
		self.keywords = keywords
		if 'extension' not in self.keywords:
			self.keywords['extension'] = ""
		if 'c_arguments' not in self.keywords:
			self.keywords['c_arguments'] = ""
		DefineTarget.TARGETS[self.system, self.format] = self

	def default_keywords(self, version = ""):
		global CPUS
		keywords = CPUS[self.cpu].copy()
		keywords.update(self.keywords)
		keywords['cpu_name'] = self.cpu
		keywords['target_name'] = self.target_name
		keywords['include_name'] = self.include_name
		if self.format_name is not None:
			keywords['format_name'] = self.format_name
		keywords['linker_format_name'] = self.linker_name
		if version != "" or version in self.versions:
			keywords['model_name'] = self.versions[version].model_name
			if self.versions[version].linker_name != "":
				keywords['linker_model_name'] = self.versions[version].linker_name
			keywords['assembler_options'] = self.versions[version].assembler_options
			keywords['linker_options'] = self.versions[version].linker_options
		return keywords

def PostProcess(System, PostProcess):
	DefineTarget.SYSTEMS[System] = PostProcess

class Target:
	def __init__(self, System, Format = None, Version = "", **arguments):
		self.system = System
		self.format = Format
		self.version = Version
		self.arguments = arguments
		Recipe.RECIPES[-1].targets.append(self)

	def lookup_target(self):
		return DefineTarget.TARGETS[self.system, self.format]

#### The Targets

CPUS = {
	'z80': {
		'assembler': 'z80-coff-as',
		'compiler': 'echo "Error: GCC does not support Z80" #',
	},
	'i86': {
		'assembler': 'i686-elf-as -msyntax=intel -mnaked-reg',
		'compiler': 'ia16-elf-gcc',
	},
	'i386': {
		'assembler': 'i686-elf-as -msyntax=intel -mnaked-reg',
		'compiler': 'i686-elf-gcc',
	},
	'm68k': {
		'assembler': 'm68k-elf-as -m68000 --register-prefix-optional --bitwise-or',
		'compiler': 'm68k-elf-gcc -m68000',
	},
	'arm': {
		'assembler': 'arm-none-eabi-as',
		'compiler': 'arm-none-eabi-gcc -march=armv3',
	},
	'pdp11': {
		'assembler': 'pdp11-aout-as',
		'compiler': 'pdp11-aout-gcc',
	},
	'z8k': {
		'assembler': 'z8k-coff-as',
		'compiler': 'z8k-coff-gcc',
	},
}

DefineTarget(
	CPU = "z80",
	System = "cpm80",
	Format = "com",
	extension = ".com")

DefineTarget(
	CPU = "z80",
	System = "cpm80",
	Format = "prl",
	extension = ".prl")

DefineTarget(
	CPU = "z80",
	System = "msxdos",
	IncludeName = "cpm80", # same include file
	Format = "com",
	extension = ".com")

DefineTarget(
	CPU = "i86",
	System = "msdos",
	Format = "com",
	Versions = [
		DefineVersion("", ModelName = "tiny", LinkerName = ""), # TODO: identical to tiny
		DefineVersion("tiny", ModelName = "tiny"),
		DefineVersion("small", ModelName = "small"),
		DefineVersion("compact", ModelName = "compact"),
	],
	extension = ".com")

DefineTarget(
	CPU = "i86",
	System = "msdos",
	Format = "mz",
	LinkerName = "exe",
	Versions = [
		DefineVersion("", ModelName = "small", LinkerName = ""), # TODO: identical to small
		DefineVersion("tiny", ModelName = "tiny"),
		DefineVersion("small", ModelName = "small"),
		DefineVersion("compact", ModelName = "compact"),
	],
	custom_entry = True,
	compiled_stack = True,
	extension = ".exe")

DefineTarget(
	CPU = "i86",
	System = "msdos1",
	IncludeName = "msdos",
	Format = "mz",
	LinkerName = "exe",
	Versions = [
		DefineVersion("", ModelName = "small", LinkerName = "", LinkerOptions = ["header_align=512"]), # TODO: identical to small
		DefineVersion("tiny", ModelName = "tiny", LinkerOptions = ["header_align=512"]),
		DefineVersion("small", ModelName = "small", LinkerOptions = ["header_align=512"]),
		DefineVersion("compact", ModelName = "compact", LinkerOptions = ["header_align=512"]),
	],
	custom_entry = True,
	compiled_stack = True,
	extension = ".exe")

DefineTarget(
	CPU = "i86",
	System = "msdos4",
	TargetName = "msdos",
	IncludeName = "msdos",
	Format = "ne",
	LinkerName = "dos4",
	Versions = [
		DefineVersion("small", ModelName = "small", LinkerName = ""),
		DefineVersion("large", ModelName = "compact", LinkerName = "large"),
	],
	c_target_options2 = "-mcmodel=small -mprotected-mode",
	#custom_entry = True, # TODO
	extension = ".exe")

DefineTarget(
	CPU = "i86",
	System = "dos16m",
	IncludeName = "msdos",
	Format = "bw",
	LinkerName = "dos16m",
	Versions = [
		DefineVersion("small", ModelName = "small", LinkerName = ""),
		DefineVersion("large", ModelName = "compact", LinkerName = "large"),
	],
	c_target_options2 = "-mcmodel=small -mprotected-mode",
	custom_entry = True,
	compiled_stack = True, # TODO: test?
	extension = ".exe",
	stub = ENV.get("DOS16MSTUB", "missing")) # TODO: requires a DOS/16M compatible stub

DefineTarget(
	CPU = "i86",
	System = "cpm86",
	Format = "cmd_tiny",
	Versions = [
		DefineVersion("noreloc", ModelName = "tiny", LinkerName = "", AssemblerOptions = ["no_reloc"], LinkerOptions = ["noreloc"]), # TODO: duplicate
		DefineVersion("tiny", ModelName = "tiny"),
		DefineVersion("tiny-noreloc", ModelName = "tiny", LinkerName = "tiny", AssemblerOptions = ["no_reloc"], LinkerOptions = ["noreloc"]),
		DefineVersion("small", ModelName = "small"),
		DefineVersion("small-noreloc", ModelName = "small", LinkerName = "small", AssemblerOptions = ["no_reloc"], LinkerOptions = ["noreloc"]),
		DefineVersion("compact", ModelName = "compact"),
		DefineVersion("compact-noreloc", ModelName = "compact", LinkerName = "compact", AssemblerOptions = ["no_reloc"], LinkerOptions = ["noreloc"]),
	],
	extension = ".cmd")

DefineTarget(
	CPU = "i86",
	System = "cpm86",
	Format = "cmd_small",
	LinkerName = "cmd",
	Versions = [
		DefineVersion("noreloc", ModelName = "small", LinkerName = "", AssemblerOptions = ["no_reloc"], LinkerOptions = ["noreloc"]), # TODO: duplicate
		DefineVersion("small", ModelName = "small"),
		DefineVersion("small-noreloc", ModelName = "small", LinkerName = "small", AssemblerOptions = ["no_reloc"], LinkerOptions = ["noreloc"]),
		DefineVersion("compact", ModelName = "compact"),
		DefineVersion("compact-noreloc", ModelName = "compact", LinkerName = "compact", AssemblerOptions = ["no_reloc"], LinkerOptions = ["noreloc"]),
		DefineVersion("tiny-noreloc", ModelName = "tiny", LinkerName = "", AssemblerOptions = ["no_reloc"], LinkerOptions = ["noreloc"]), # TODO: this is a bug, remove it
	],
	extension = ".cmd")

DefineTarget(
	CPU = "i86",
	System = "cpm86",
	Format = "cmd_compact",
	Versions = [
		DefineVersion("compact", ModelName = "compact"),
		DefineVersion("compact-noreloc", ModelName = "compact", LinkerName = "compact", AssemblerOptions = ["no_reloc"], LinkerOptions = ["noreloc"]),
	],
	extension = ".cmd")

DefineTarget(
	CPU = "i86",
	System = "elks",
	Format = "aout_com",
	LinkerName = "elks_comb",
	PostProcess = Recipe([
		RecipeStep(Arguments = ["chmod", "+x", "{binary_file}"], Input = "{binary_file}", Output = "{binary_file}"),
	], register = False))

DefineTarget(
	CPU = "i86",
	System = "elks",
	Format = "aout_sep",
	LinkerName = "elks_sep",
	PostProcess = Recipe([
		RecipeStep(Arguments = ["chmod", "+x", "{binary_file}"], Input = "{binary_file}", Output = "{binary_file}"),
	], register = False))

DefineTarget(
	CPU = "i86",
	System = "win16",
	c_target_options2 = "-mcmodel=small -mprotected-mode",
	custom_entry = True,
	extension = ".exe")

DefineTarget(
	CPU = "i86",
	System = "os2v1",
	IncludeName = "os2",
	c_target_options2 = "-mcmodel=small -mprotected-mode",
	#custom_entry = True, # TODO: ???
	extension = ".exe")

DefineTarget(
	CPU = "i386",
	System = "djgpp",
	IncludeName = "msdos",
	Format = "aout",
	LinkerName = "djgppv1",
	extension = ".exe",
	stub = ENV.get("GO32STUB", "missing")) # TODO: requires a GO32 compatible stub that can load a.out files

DefineTarget(
	CPU = "i386",
	System = "djgpp",
	IncludeName = "msdos",
	Format = "coff",
	LinkerName = "djgpp",
	extension = ".exe",
	stub = ENV.get("CWSDPMISTUB", "missing")) # TODO: requires a CWSDPMI compatible stub that can load COFF files

DefineTarget(
	CPU = "i386",
	System = "dos4g",
	IncludeName = "msdos",
	extension = ".exe",
	stub = ENV.get("DOS4GWSTUB", "missing")) # TODO: requires a DOS/4GW compatible stub

DefineTarget(
	CPU = "i386",
	System = "pharlap",
	IncludeName = "msdos",
	Format = "mp",
	LinkerName = "pharlap",
	custom_entry = True,
	compiled_stack = True,
	extension = ".exp")

DefineTarget(
	CPU = "i386",
	System = "pharlap",
	IncludeName = "msdos",
	Format = "mq",
	LinkerName = "pharlap_rex",
	custom_entry = True,
	compiled_stack = True,
	extension = ".rex")

DefineTarget(
	CPU = "i386",
	System = "pharlap",
	IncludeName = "msdos",
	Format = "p3",
	LinkerName = "pharlap_extended",
	custom_entry = True,
	compiled_stack = True,
	extension = ".exp")

DefineTarget(
	CPU = "i386",
	System = "pharlap",
	IncludeName = "msdos",
	Format = "p3_seg",
	LinkerName = "pharlap_segmented",
	custom_entry = True,
	compiled_stack = True,
	extension = ".exp")

DefineTarget(
	CPU = "i386",
	System = "pdos386",
	IncludeName = "msdos",
	extension = ".exe")

DefineTarget(
	CPU = "i386",
	System = "os2v2",
	IncludeName = "os2",
	#custom_entry = True, # TODO: ???
	extension = ".exe")

DefineTarget(
	CPU = "m68k",
	System = "cpm68k",
	Format = "68k_cont",
	LinkerName = "cpm68k",
	Versions = [
		DefineVersion("", LinkerOptions = ["noreloc"]),
		DefineVersion("default", LinkerName = ""), # TODO
		DefineVersion("reloc", LinkerName = "", AssemblerOptions = ["no_reloc=0"], LinkerOptions = []),
		DefineVersion("noreloc", LinkerName = "", AssemblerOptions = ["no_reloc"], LinkerOptions = []),
	],
	extension = ".68k")

DefineTarget(
	CPU = "m68k",
	System = "cpm68k",
	Format = "68k_noncont",
	LinkerName = "cpm68k_noncont",
	Versions = [
		DefineVersion("", LinkerOptions = ["noreloc"]),
		DefineVersion("default", LinkerName = ""), # TODO
		DefineVersion("reloc", LinkerName = "", AssemblerOptions = ["no_reloc=0"], LinkerOptions = []),
		DefineVersion("noreloc", LinkerName = "", AssemblerOptions = ["no_reloc"], LinkerOptions = []),
	],
	extension = ".68k")

DefineTarget( # TODO: this is a bug, remove
	CPU = "m68k",
	System = "cpm68k",
	Format = "68k_noncont-cont",
	FormatName = "68k_noncont",
	LinkerName = "cpm68k",
	Versions = [
		DefineVersion("default", LinkerName = ""),
	],
	extension = ".68k")

DefineTarget(
	CPU = "m68k",
	System = "gemdos",
	FormatName = "prg",
	LinkerName = "tos",
	Versions = [
		DefineVersion(""),
		DefineVersion("noreloc", LinkerName = "", AssemblerOptions = ["no_reloc"], LinkerOptions = ["noreloc"]),
		DefineVersion("reloc", LinkerName = "", AssemblerOptions = ["no_reloc", "suppress_reloc=0"], LinkerOptions = ["reloc"]),
	],
	extension = ".prg")

DefineTarget(
	CPU = "m68k",
	System = "human68k",
	Format = "rfile",
	Versions = [
		DefineVersion(""),
		DefineVersion("reloc", LinkerName = "", AssemblerOptions = ["suppress_reloc=0"], LinkerOptions = []), # TODO: misnomer
	],
	FormatName = "r",
	c_target_options = "-mpcrel",
	extension = ".r")

DefineTarget(
	CPU = "m68k",
	System = "human68k",
	Format = "zfile",
	FormatName = "z",
	c_target_options = "-mpcrel",
	extension = ".z")

DefineTarget(
	CPU = "m68k",
	System = "human68k",
	Format = "xfile",
	FormatName = "x",
	custom_entry = True,
	c_target_options = "-mpcrel",
	extension = ".x")

DefineTarget(
	CPU = "m68k",
	System = "amiga")

DefineTarget(
	CPU = "m68k",
	System = "macos",
	Versions = [
		DefineVersion("", ModelName = "default", LinkerName = ""),
		DefineVersion("tiny", ModelName = "tiny"),
	],
	c_target_options = "-mpcrel",
	custom_entry = True,
	additional_outputs = ('{output_file}.mbin', '%{output_file}', '.finf/{output_file}', '.rsrc/{output_file}'))

DefineTarget(
	CPU = "m68k",
	System = "sql",
	extension = "_bin") # can be anything

DefineTarget(
	CPU = "z8k",
	System = "cpm8k",
	Format = "z8k",
	LinkerName = "cpm8k",
	extension = ".z8k")

DefineTarget(
	CPU = "z8k",
	System = "cpm8k",
	Format = "z8k",
	FormatName = "z8k_nonshared",
	LinkerName = "cpm8k",
	extension = ".z8k")

DefineTarget(
	CPU = "z8k",
	System = "cpm8k",
	Format = "z8k_split",
	LinkerName = "cpm8k_split",
	extension = ".z8k")

DefineTarget(
	CPU = "z8k",
	System = "cpm8k",
	Format = "z8k_segmented",
	LinkerName = "cpm8k_segmented",
	c_arguments = " -mz8001",
	extension = ".z8k")

DefineTarget(
	CPU = "arm",
	System = "riscos",
	Format = "ff8",
	extension = ",ff8") # actually, not an extension, but a hexadecimal file type

DXDOS_BASE = 0x200 # This seems to be the default, with SP set to 0x1FE, presumably with a value pushed
DefineTarget(
	CPU = "pdp11",
	System = "dxdos",
	Format = "com",
	Versions = [
		DefineVersion("", LinkerOptions = [f"base_address={DXDOS_BASE}"]),
	],
	extension = ".com")

if __name__ == '__main__':
	exec(open('Bakefile').read())

	#### Cook recipes

	for rcp in Recipe.RECIPES:
		process = Process()
		systems = {}
		for trg_desc in rcp.targets:
			target = trg_desc.lookup_target()
			keywords = target.default_keywords(trg_desc.version).copy()
			keywords.update(trg_desc.arguments)
			keywords['binary_file'] = keywords['output_file'] + keywords['extension']
			for step in rcp.generate_steps(**keywords):
				process.append(step)
			if target.post_process is not None:
				for step in target.post_process.generate_steps(**keywords):
					process.append(step)
			if trg_desc.system in DefineTarget.SYSTEMS:
				if trg_desc.system not in systems:
					systems[trg_desc.system] = [DefineTarget.SYSTEMS[trg_desc.system], []]
				systems[trg_desc.system][1].append(keywords['binary_file'])

		for recipe, binary_files in systems.values():
			keywords = target.default_keywords(trg_desc.version).copy()
			keywords['binary_files'] = binary_files
			for step in recipe.generate_steps(**keywords):
				process.append(step)

		if len(sys.argv) <= 1 or sys.argv[1] == 'all':
			process.execute()
		elif sys.argv[1] == 'clean':
			files = {output for step in process.steps for output in step.output}
			dirs = set()
			for file in sorted(files):
				if os.path.isdir(file):
					dirs.add(file)
				else:
					if os.path.exists(file):
						print(f"[DEL] >>> {file}", file = sys.stderr)
						pathlib.Path(file).unlink()
					diry = os.path.split(file)[0]
					if diry != '':
						dirs.add(diry)
			while len(dirs) > 0:
				dirys = dirs
				dirs = set()
				for diry in dirys:
					if os.path.exists(diry):
						print(f"[RMD] >>> {diry}", file = sys.stderr)
						pathlib.Path(diry).rmdir()
					pdir = os.path.split(diry)[0]
					if pdir != '':
						dirs.add(pdir)
		else:
			print("Unknown option:", sys.argv[1], file = sys.stderr)

