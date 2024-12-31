#! /usr/bin/python3

import os
import sys
import struct
import codecs

def control_decode(intext):
	text = ''
	for c in intext:
		if ord(c) < 0x20:
			text += chr(0x2400 + ord(c))
		elif ord(c) == 0x7F:
			text += '␡'
		else:
			text += c
	return text

def control_decoder(codec):
	def decode(binary : bytes):
		binary = bytes(binary)
		return control_decode(binary.decode(codec, 'replace')), len(binary)
	return decode

CP437 = "␀☺☻♥♦♣♠•◘○◙♂♀♪♫☼►◄↕‼¶§▬↨↑↓→←∟↔▲▼"

def cp437_decode(binary : bytes):
	if type(binary) is not bytes:
		binary = bytes(binary)
	text = ''
	for c in binary.decode('cp437'):
		if ord(c) == 0xA0:
			text += '�'
		elif ord(c) == 0x7F:
			text += '⌂'
		elif ord(c) < 0x20:
			text += CP437[ord(c)]
		else:
			text += c
	return text, len(binary)

#ST = "␀⇧⇩⇨⇦❎︎🮾🮿✓🕒︎🔔︎♪␌␍��🯰🯱🯲🯳🯴🯵🯶🯷🯸🯹ə␛���� !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~⌂ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥ßƒáíóúñÑªº¿⌐¬½¼¡«»ãõØøœŒÀÃÕ¨´†¶©®™ĳĲאבגדהוזחטיכלמנסעפצקרשתןךםףץ§∧∞αβΓπΣσµτΦΘΩδ∮ϕ∈∩≡±≥≤⌠⌡÷≈°•·√ⁿ²³¯"

ST = "␀⇧⇩⇨⇦���✓��♪␌␍������������ə␛���� !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~⌂ÇüéâäàåçêëèïîìÄÅÉæÆôöòûùÿÖÜ¢£¥ßƒáíóúñÑªº¿⌐¬½¼¡«»ãõØøœŒÀÃÕ¨´†¶©®™ĳĲאבגדהוזחטיכלמנסעפצקרשתןךםףץ§∧∞αβΓπΣσµτΦΘΩδ∮ϕ∈∩≡±≥≤⌠⌡÷≈°•·√ⁿ²³¯"

def st_decode(binary : bytes):
	if type(binary) is not bytes:
		binary = bytes(binary)
	return ''.join(ST[b] for b in binary), len(binary)

def custom_decoder_search(encoding_name):
	return {
		'cp437_full': codecs.CodecInfo(None, cp437_decode, name = 'cp437-full'),
		'st_full': codecs.CodecInfo(None, st_decode, name = 'st-full'),
		'ascii_graphic': codecs.CodecInfo(None, control_decoder('ascii'), name = 'ascii-graphic'),
		'macroman_graphic': codecs.CodecInfo(None, control_decoder('macroman'), name = 'mac-roman-graphic'),
	}.get(encoding_name)

codecs.register(custom_decoder_search)

class Reader:
	LittleEndian = 1
	BigEndian = 2
	PDP11Endian = 3
	AntiPDP11Endian = 4 # Honeywell Series 16 was claimed to be this
	def __init__(self, fp, endian = LittleEndian):
		self.fp = fp
		self.endian = endian

	def Tell(self):
		return self.fp.tell()

	def Seek(self, offset):
		self.fp.seek(offset, 0)

	def Skip(self, offset):
		self.fp.seek(offset, 1)

	def SeekEnd(self, offset = 0):
		self.fp.seek(offset, 2)

	def Read(self, count = None):
		if count is None:
			return self.fp.read(1)[0]
		else:
			return self.fp.read(count)

	def ReadToZero(self):
		b = b''
		c = self.Read(1)
		while c[0] != 0:
			b += c
			c = self.Read(1)
		return b

	def GetFormat(self, count, signed = False, endian = None):
		if endian is None:
			endian = self.endian
		fmt = {1: 'B', 2: 'H', 4: 'I', 8: 'Q'}.get(count)
		if fmt is None:
			return None
		if signed:
			fmt = fmt.lower()

		if count == 1:
			return '=' + fmt
		elif endian == Reader.LittleEndian \
		or endian == Reader.PDP11Endian and count <= 2:
			return '<' + fmt
		elif endian == Reader.BigEndian:
			return '>' + fmt
		else:
			return None

	def ParseWord(self, data, signed = False, endian = None):
		if endian is None:
			endian = self.endian
		fmt = self.GetFormat(len(data), signed = signed, endian = endian)
		if fmt is not None:
			return struct.unpack(fmt, data)[0]
		else:
			value = 0
			for i in range(len(data)):
				if endian == Reader.LittleEndian or len(data) <= 2 and endian == Reader.PDP11Endian:
					value |= data[i] << (i << 3)
				elif endian == Reader.BigEndian:
					value |= data[i] << ((len(data) - i - 1) << 3)
				elif endian == Reader.PDP11Endian:
					value |= data[i] << ((len(data) - (i ^ 1) - 1) << 3)
			if value & (1 << ((len(data) << 3) - 1)):
				value |= -1 << (len(data) << 3)
			else:
				value &= ~(-1 << (len(data) << 3))
			return value

	def ReadWord(self, count, signed = False, endian = None):
		if endian is None:
			endian = self.endian
		data = self.Read(count)
		if len(data) < count:
			print(f"Warning: expected 0x{count:X} bytes, stream ended at 0x{self.Tell():X}", file = sys.stderr)
			data += bytes(count - len(data))
		return self.ParseWord(data, signed = signed, endian = endian)

class Determiner:
	FMT_COUNT = 0
	FMT_COUNT += 1; FMT_MZ = FMT_COUNT
	FMT_COUNT += 1; FMT_NE = FMT_COUNT
	FMT_COUNT += 1; FMT_LE = FMT_COUNT
	FMT_COUNT += 1; FMT_AOut = FMT_COUNT
	FMT_COUNT += 1; FMT_COFF = FMT_COUNT
	FMT_COUNT += 1; FMT_MINIX = FMT_COUNT
	FMT_COUNT += 1; FMT_68K_CONTIGUOUS = FMT_COUNT
	FMT_COUNT += 1; FMT_68K_NONCONTIGUOUS = FMT_COUNT
	FMT_COUNT += 1; FMT_68K_CRUNCHED = FMT_COUNT
	FMT_COUNT += 1; FMT_HU = FMT_COUNT
	FMT_COUNT += 1; FMT_MP_MQ = FMT_COUNT
	FMT_COUNT += 1; FMT_P2_P3 = FMT_COUNT
	FMT_COUNT += 1; FMT_BW = FMT_COUNT
	FMT_COUNT += 1; FMT_UZI280 = FMT_COUNT
	FMT_COUNT += 1; FMT_PE = FMT_COUNT
	FMT_COUNT += 1; FMT_ELF = FMT_COUNT
	FMT_COUNT += 1; FMT_Hunk = FMT_COUNT
	FMT_COUNT += 1; FMT_MacRsrc = FMT_COUNT
	FMT_COUNT += 1; FMT_Apple = FMT_COUNT
	FMT_COUNT += 1; FMT_Adam = FMT_COUNT
	FMT_COUNT += 1; FMT_D3X = FMT_COUNT
	FMT_COUNT += 1; FMT_DX64 = FMT_COUNT
	FMT_COUNT += 1; FMT_CPM8000 = FMT_COUNT

	def __init__(self, rd):
		self.rd = rd
		self.rd.endian = Reader.LittleEndian

	def GetMagic(self):
		magic = self.rd.Read(2)
		if magic in {b'MZ', b'ZM'}:
			return Determiner.FMT_MZ
		elif magic in {b'NE', b'DX'}: # DOS/16M NE
			return Determiner.FMT_NE
		elif magic in {b'LE', b'LX'}:
			return Determiner.FMT_LE
		elif magic in {b'\x07\x01', b'\x08\x01', b'\x0B\x01', b'\xCC\x00'}: # OMAGIC, NMAGIC, ZMAGIC, QMAGIC (little endian)
			return Determiner.FMT_AOut
		elif magic in {b'\x4C\x01', b'\x01\x50'}: # COFF-i386, COFF-m68k
			return Determiner.FMT_COFF
		elif magic == b'\x01\x03':
			return Determiner.FMT_MINIX # MINIX a.out
		elif magic == b'\x60\x1A':
			return Determiner.FMT_68K_CONTIGUOUS
		elif magic == b'\x60\x1B':
			return Determiner.FMT_68K_NONCONTIGUOUS
		elif magic == b'\x60\x1C':
			return Determiner.FMT_68K_CRUNCHED
		elif magic == b'HU':
			return Determiner.FMT_HU
		elif magic in {b'MP', b'MQ'}:
			return Determiner.FMT_MP_MQ
		elif magic in {b'P2', b'P3'}:
			return Determiner.FMT_P2_P3
		elif magic == b'BW':
			return Determiner.FMT_BW
		elif magic == b'\xFF\x00':
			return Determiner.FMT_UZI280 # UZI-280
		elif magic in {b'\xEE\x00', b'\xEE\x01', b'\xEE\x02', b'\xEE\x03', b'\xEE\x07', b'\xEE\x0B'}:
			return Determiner.FMT_CPM8000
		else:
			magic += self.rd.Read(2)
			if magic in {b'PE\0\0', b'PL\0\0'}: # Phar Lap PE
				return Determiner.FMT_PE
			elif magic == b'\x7FELF':
				return Determiner.FMT_ELF
			elif magic in {b'\0\0\x03\xF3', b'\0\0\x03\xE7'}: # executable, library
				return Determiner.FMT_Hunk
			elif magic in {b'\0\x05\x16\x00', b'\0\x05\x16\x07'}:
				return Determiner.FMT_Apple
			elif magic in {b'Adam', b'Dll '}:
				return Determiner.FMT_Adam
			elif magic in {b'D3X1'}:
				return Determiner.FMT_D3X
			elif magic in {b'LV\0\0', 'Flat'}:
				return Determiner.FMT_DX64
		# TODO: PRL, CP/M Plus

	def ReadMagic(self):
		self.rd.Seek(0)
		magic = self.GetMagic()
		if magic == Determiner.FMT_MZ:
			# Fetch offset from long word at 0x3C
			self.rd.Seek(0x3C)
			offset = self.rd.ReadWord(4, endian = Reader.LittleEndian)
			self.rd.Seek(offset)
			if offset != 0:
				magic = self.GetMagic()
				if magic is not None:
					return magic
			# Seek end of MZ image
			self.rd.Seek(0x02)
			offset = self.rd.ReadWord(2)
			offset = (self.rd.ReadWord(2) << 9) - ((-offset) & 0x1FF)
			self.rd.Seek(offset)
			magic = self.GetMagic()
			if magic is not None:
				return magic
			return Determiner.FMT_MZ
		else:
			return magic

class FileReader:
	def __init__(self, rd):
		self.rd = rd

	def fetch_segment_data(self, count):
		# To be overridden by subclasses
		return self.rd.Read(count)

	def get_rows(self, arg, offset = 0, row_length = 16):
		row_start = (offset // 16) * 16
		current_offset = offset % 16
		if type(arg) is int:
			row_end = offset + arg
		elif type(arg) is bytes:
			row_end = offset + len(arg)

		for row in range(row_start, row_end, 16):
			#data = self.fetch_segment_data(min(16, row_end - row) - current_offset)
			count = 16 - current_offset
			if type(arg) is int:
				data = self.rd.Read(min(count, row_end - row))
			elif type(arg) is bytes:
				data = arg[row - row_start:min(row - row_start + count, len(arg))]
			current_offset = 0
			yield row, data

	def signal_reloc(self, offset, size, hexcodes, text, row_length = 16):
		if offset + size > 0:
			start = max(0, offset)
			length = min(size, row_length - offset)
			text = text[:start] + "\33[4m" + text[start:offset + length] + "\33[m" + text[offset + length:]

			start = 3 * start
			length = 3 * length - 1
			hexcodes = hexcodes[:start] + "\33[4m" + hexcodes[start:offset * 3 + length] + "\33[m" + hexcodes[offset * 3 + length:]
		return hexcodes, text

	def process_data(self, options, max_reloc_size, parameter, data_offset = 0,
			get_reloc_size = None, text_encoding = 'ascii_graphic'):
		current_offset = data_offset
		for row, data in self.get_rows(parameter, offset = data_offset): # TODO: make this into a parameter as well
			hexcodes = " ".join(f"{b:02X}" for b in data)
			text = data.decode(options.get('encoding', text_encoding))
			if current_offset != 0:
				hexcodes = ' ' * (3 * current_offset) + hexcodes
				text = ' ' * current_offset + text
				current_offset = 0
			if len(hexcodes) < (16 * 3 - 1):
				hexcodes += ' ' * ((16 * 3 - 1) - len(hexcodes))

			if get_reloc_size is not None and (options.get('relshow', False) or options.get('showall', False)):
				for offset in range(16 - 1, -max_reloc_size, -1):
					size = get_reloc_size(row + offset)
					if size is not None:
						hexcodes, text = self.signal_reloc(offset, size, hexcodes, text)

			yield row, hexcodes, text

class CPM86Reader(FileReader):
	def __init__(self, rd):
		super(CPM86Reader, self).__init__(rd)
		self.rd.endian = Reader.LittleEndian

	def ReadImage(self, image_offset = 0, **options):
		if image_offset != 0:
			print(f"- Image offset: 0x{image_offset:08X}")
		self.rd.Seek(image_offset)
		segments = []
		for i in range(8):
			segment_type = self.rd.ReadWord(1)
			if segment_type == 0:
				break
			segment_size = self.rd.ReadWord(2) << 4
			segment_base = self.rd.ReadWord(2) << 4
			segment_min  = self.rd.ReadWord(2) << 4
			segment_max  = self.rd.ReadWord(2) << 4
			segments.append((segment_type, segment_size, segment_base, segment_min, segment_max))

		self.rd.Seek(image_offset + 0x48)
		libraries_type = self.rd.ReadWord(1)
		if libraries_type == 0xFF:
			libraries_size = self.rd.ReadWord(2) << 4
			libraries_base = self.rd.ReadWord(2) << 4
			libraries_min  = self.rd.ReadWord(2) << 4
			libraries_max  = self.rd.ReadWord(2) << 4

		self.rd.Seek(image_offset + 0x60)
		library_name = self.rd.Read(8)
		is_library = library_name[0] != 0 # TODO?
		library_name = library_name.decode(options.get('encoding', 'cp437_full'))
		library_version = self.rd.ReadWord(2)
		library_version = (library_version, self.rd.ReadWord(2))
		library_flags = self.rd.ReadWord(4)

		self.rd.Seek(image_offset + 0x7B)
		rsx_table_offset = self.rd.ReadWord(2) << 7
		fixup_offset = self.rd.ReadWord(2) << 7
		flags = self.rd.ReadWord(1)

		print("= Group table")
		segment_offset = 0x80
		if libraries_type == 0xFF:
			libraries_offset = segment_offset
			segment_offset += libraries_size
		segment_offsets = []
		for index, (segment_type, segment_size, segment_base, segment_min, segment_max) in enumerate(segments):
			segment_type_name = {
				0x1: 'code',
				0x2: 'data',
				0x3: 'extra',
				0x4: 'stack',
				0x5: 'auxiliary 1',
				0x6: 'auxiliary 2',
				0x7: 'auxiliary 3',
				0x8: 'auxiliary 4/fixups',
				0x9: 'shared code',
			}.get(segment_type & 0x0F, 'undefined')
			print(f"Segment #{index + 1}:")
			print(f"- Type: {segment_type_name} (0x{segment_type:02X})")
			if segment_base != 0:
				print(f"- Address: 0x{segment_base:06X}")
			print(f"- Offset: 0x{segment_offset:06X} (0{image_offset + segment_offset:08X} in file)")
			print(f"- Length: 0x{segment_size:06X}")
			if segment_min != segment_size:
				print(f"- Minimum: 0x{segment_min:06X}")
			if segment_max != 0:
				print(f"- Maximum: 0x{segment_max:06X}")
			segment_offsets.append(segment_offset)
			segment_offset += segment_size

		if libraries_type == 0xFF:
			print(f"Libraries:")
			if libraries_base != 0:
				print(f"- Address: 0x{libraries_base:06X}")
			print(f"- Offset: 0x{libraries_offset:06X} (0{image_offset + libraries_offset:08X} in file)")
			print(f"- Length: 0x{libraries_size:06X}")
			if libraries_min != libraries_size:
				print(f"- Minimum: 0x{libraries_min:06X}")
			if libraries_max != 0 and libraries_max != libraries_size:
				print(f"- Maximum: 0x{libraries_max:06X}")
			segment_offset += segment_size

		if rsx_table_offset != 0:
			print(f"RSX index offset: 0x{rsx_table_offset:06X} (0{image_offset + rsx_table_offset:08X} in file)")
		if fixup_offset != 0:
			print(f"Fixup offset: 0x{fixup_offset:06X} (0{image_offset + fixup_offset:08X} in file)")
			if not flags & 0x80:
				print("Warning: no actual fixups take place")
		print(f"Flags: 0x{flags:02X}", end = '')
		if flags & 0x08: # TODO: unsure, double check
			print(", direct video access", end = '')
		if flags & 0x10:
			print(", RSX", end = '')
		if flags & 0x20:
			print(", needs 8087", end = '')
		if flags & 0x40:
			print(", uses or emulates 8087", end = '')
		if flags & 0x80:
			print(", do fixups", end = '')
		print()

		if is_library:
			print(f"Library: {library_name} {'.'.join(map(str, library_version))}, flags: 0x{library_flags:08X}", end = '')
			# TODO: flags?
			print()

		imported_library_names = []
		imported_library_fixup_counts = []
		if libraries_type == 0xFF:
			self.rd.Seek(image_offset + libraries_offset)
			library_count = self.rd.ReadWord(2)
			actual_size = (2 + library_count * 18 + 0xF) & ~0xF
			if actual_size != libraries_size:
				if actual_size < libraries_size:
					print("Error: actual STRL group is too short", file = sys.stderr)
				if actual_size > libraries_size:
					print("Warning: actual STRL group is too long", file = sys.stderr)
				print(f"Actual library size: 0x{actual_size:06X}")
			for count in range(library_count):
				imported_library_name = self.rd.Read(8).decode(options.get('encoding', 'cp437_full'))
				imported_library_version = self.rd.ReadWord(2)
				imported_library_version = (imported_library_version, self.rd.ReadWord(2))
				imported_library_flags = self.rd.ReadWord(4)
				imported_library_fixups = self.rd.ReadWord(2)
				print(f"Imported library #{count + 1}: {imported_library_name} {'.'.join(map(str, imported_library_version))}, flags: 0x{imported_library_flags:08X}", end = '')
				# TODO: flags?
				print(f", fixups: #{imported_library_fixups}")
				imported_library_names.append(imported_library_name)
				imported_library_fixup_counts.append(imported_library_fixups)

		relocs = [set() for i in range(len(segments))]
		if options.get('rel', False) or options.get('showall', False):
			if flags & 0x80:
				print("= Relocations")
				self.rd.Seek(image_offset + fixup_offset)
				index = 0
				while True:
					target_group = self.rd.ReadWord(1)
					if target_group == 0:
						break
					group = target_group >> 4
					target_group &= 0xF
					if group == 0 or group > len(segments):
						print(f"Error: invalid group {group}", file = sys.stderr)
					if target_group == 0 or target_group > len(segments):
						print(f"Error: invalid group {target_group}", file = sys.stderr)
					segment = self.rd.ReadWord(2)
					offset = self.rd.ReadWord(1)
					position = (segment << 4) + offset
					print(f"Relocation #{index + 1} (0x{fixup_offset + 4 * index:04X}) to group #{target_group} at #{group}:0x{position:06X}")
					if position in relocs[group - 1]:
						print(f"Warning: duplicate relocation 0x{position:06X}", file = sys.stderr)
					relocs[group - 1].add(position)
					index += 1
				if len(imported_library_fixup_counts) > 0:
					self.rd.Skip(3)
					for imported_library_name, imported_library_fixups in zip(imported_library_names, imported_library_fixup_counts):
						print(f"- Fixups for library {imported_library_name}")
						for index2 in range(imported_library_fixups):
							target_group = self.rd.ReadWord(1)
							if target_group == 0:
								break
							group = target_group >> 4
							target_group &= 0xF
							if group == 0 or group > len(segments):
								print(f"Error: invalid group {group}", file = sys.stderr)
							segment = self.rd.ReadWord(2)
							offset = self.rd.ReadWord(1)
							position = (segment << 4) + offset
							print(f"Relocation #{index + index2 + 2} (0x{fixup_offset + 4 * index:04X}) at #{group}:0x{position:06X}")
							if position in relocs[group - 1]:
								print(f"Warning: duplicate relocation 0x{position:06X}", file = sys.stderr)
							relocs[group - 1].add(position)
						index += imported_library_fixups

		if options.get('data', False) or options.get('showall', False):
			for segment_number, segment_offset in enumerate(segment_offsets):
				print(f"= Segment data #{segment_number + 1}")
				print("[FILE    ] SEGMENT \tDATA")
				self.rd.Seek(segment_offset)
				for row, hexcodes, text in self.process_data(options, 2, segments[segment_number][1],
						get_reloc_size = lambda position: 2 if position in relocs[segment_number] else None, text_encoding = 'cp437_full'):
					print(f"[{image_offset + segment_offset + row:08X}] {row:08X}\t{hexcodes}\t{text}")

		if rsx_table_offset != 0:
			print(f"= RSX table")
			self.rd.Seek(image_offset + rsx_table_offset)
			index = 0
			rsx_offsets = []
			rsx_names = []
			while True:
				rsx_offset = self.rd.ReadWord(2)
				if rsx_offset == 0xFFFF:
					break
				rsx_offset <<= 7
				rsx_name = self.rd.Read(8).decode(options.get('encoding', 'cp437_full'))
				rsx_offsets.append(rsx_offset)
				rsx_names.append(rsx_name)
				print(f"RSX #{len(rsx_offsets)}:")
				print(f"- Name: {rsx_name!r}")
				print(f"- Offset: 0x{rsx_offset:06X} (0x{image_offset + rsx_offset:08X} in file)")
				self.rd.Skip(6)

			for index, (rsx_offset, rsx_name) in enumerate(zip(rsx_offsets, rsx_names)):
				print(f"== RSX #{index + 1}: {rsx_name!r}")
				self.ReadImage(rsx_offset, **options)

	def ReadFile(self, **options):
		self.rd.SeekEnd()
		size = self.rd.Tell()
		print("==== CP/M-86 format ====")
		self.ReadImage(0, **options)

class MZReader(FileReader):
	def __init__(self, rd):
		super(MZReader, self).__init__(rd)
		self.rd.endian = Reader.LittleEndian

	def ReadFile(self, **options):
		print("==== MS-DOS MZ .EXE format ====")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(0)
		magic = self.rd.Read(2)
		if magic not in {b'MZ', b'ZM'}:
			print(f"Error: invalid magic {magic} at 0x{self.rd.Tell():X}", file = sys.stderr)
		print(f"Magic number: {magic}")
		file_size = self.rd.ReadWord(2)
		if file_size > 0x1FF:
			print(f"Error: invalid last block count 0x{file_size:X} at 0x{self.rd.Tell():X}", file = sys.stderr)
		file_size = (self.rd.ReadWord(2) << 9) - ((-file_size) & 0x1FF)
		print(f"File size: 0x{file_size:08X}")
		if file_size > size:
			print(f"Error: reported file size larger than actual file size: 0x{file_size:08X} > 0x{size:08X}", file = sys.stderr)
		if file_size != size:
			print(f"Actual size: 0x{size:08X}")
			if file_size < size:
				print(f"Trailing data: 0x{size - file_size:08X}")
		reloc_count = self.rd.ReadWord(2)
		header_size = self.rd.ReadWord(2) << 4
		print(f"Header length: 0x{header_size:08X}")
		min_memory = self.rd.ReadWord(2) << 4
		print(f"Minimum extra memory: 0x{min_memory:08X}")
		max_memory = self.rd.ReadWord(2) << 4
		if min_memory > max_memory:
			print(f"Warning: minimum required additional memory exceeds maximum memory: 0x{min_memory:08X} > 0x{max_memory:08X}", file = sys.stderr)
		print(f"Maximum extra memory: 0x{max_memory:08X}")
		print(f"Image offset: 0x{header_size:08X}")
		if file_size < header_size:
			print(f"Error: header is longer than entire file: 0x{file_size:08X} < 0x{header_size:08X}", file = sys.stderr)
		elif file_size == header_size:
			print(f"Error: header takes up entire file: 0x{file_size:08X} < 0x{header_size:08X}", file = sys.stderr)
		print(f"Image length: 0x{file_size - header_size:08X}")
		print(f"Total length: 0x{file_size - header_size + min_memory:08X}")
		ss = self.rd.ReadWord(2)
		sp = self.rd.ReadWord(2)
		print(f"SS:SP = 0x{ss:04X}:0x{sp:04X}")
		checksum = self.rd.ReadWord(2)
		if checksum != 0:
			print(f"Checksum: 0x{checksum:04X}")
			# TODO: check checksum
		ip = self.rd.ReadWord(2)
		cs = self.rd.ReadWord(2)
		print(f"CS:IP = 0x{cs:04X}:0x{ip:04X}")
		reloc_offset = self.rd.ReadWord(2)
		if reloc_offset != 0 or reloc_count != 0:
			print(f"Relocations offset: 0x{reloc_offset:04X}")
		if reloc_count != 0:
			print(f"Relocations length: 0x{reloc_count * 4:04X}")
			print(f"Relocations count: {reloc_count}")
			if reloc_offset < 0x1C:
				print(f"Warning: Relocations start in standard header fields at 0x{reloc_offset:04X}", file = sys.stderr)
			if reloc_offset > header_size:
				print(f"Warning: Relocations begin outside header at 0x{reloc_offset:04X}, after 0x{header_size:04X}", file = sys.stderr)
			elif reloc_offset + 4 * reloc_count > header_size:
				print(f"Warning: Relocations cross header boundary from 0x{reloc_offset:04X} to 0x{reloc_offset + 4 * reloc_count:04X}, after 0x{header_size:04X}", file = sys.stderr)
		overlay_number = self.rd.ReadWord(2)
		if overlay_number != 0:
			print(f"Overlay number: {overlay_number}")
		relocs = set()
		if options.get('rel', False) or options.get('showall', False):
			if reloc_count != 0:
				self.rd.Seek(reloc_offset)
				for index in range(reloc_count):
					offset = self.rd.ReadWord(2)
					segment = self.rd.ReadWord(2)
					position = (segment << 4) + offset
					print(f"Relocation #{index + 1} (0x{reloc_offset + 4 * index:04X}) at 0x{segment:04X}:0x{offset:04X} (0x{position:06X})")
					if position in relocs:
						print(f"Warning: duplicate relocation 0x{position:06X}", file = sys.stderr)
					relocs.add(position)
		if options.get('data', False) or options.get('showall', False):
			self.rd.Seek(header_size)
			print("[FILE    ] SEGMENT \tDATA")
			for row, hexcodes, text in self.process_data(options, 2, file_size - header_size,
					get_reloc_size = lambda position: 2 if position in relocs else None, text_encoding = 'cp437_full'):
				print(f"[{header_size + row:08X}] {row:08X}\t{hexcodes}\t{text}")

class NEReader(FileReader):
	def __init__(self, rd):
		super(NEReader, self).__init__(rd)
		self.rd.endian = Reader.LittleEndian

	def FetchName(self, offset):
		pos = self.rd.Tell()
		self.rd.Seek(offset)
		name_length = self.rd.ReadWord(1)
		name = self.rd.Read(name_length)
		if len(name) < name_length:
			name += bytes(name_length - len(name))
		self.rd.Seek(pos)
		return name

	def FetchModuleName(self, offset, index):
		pos = self.rd.Tell()
		self.rd.Seek(index)
		name_offset = self.rd.ReadWord(2)
		name = self.FetchName(offset + name_offset)
		self.rd.Seek(pos)
		return name

	def ReadFile(self, **options):
		print("==== NE .EXE format ====")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(0)
		magic = self.rd.Read(2)
		if magic in {b'NE', b'DX'}:
			print("Stubless image")
			new_header_offset = 0
		else:
			if magic not in {b'MZ', b'ZM'}:
				print(f"Warning: invalid stub magic {magic}", file = sys.stderr)
			self.rd.Seek(0x18)
			reloc_offset = self.rd.ReadWord(2)
			if reloc_offset < 0x40:
				print(f"Warning: stub relocation offset at 0x18 is supposed to be at least 0x0040, received: 0x{reloc_offset:04X}", file = sys.stderr)
			self.rd.Seek(0x3C)
			new_header_offset = self.rd.ReadWord(4)
			self.rd.Seek(new_header_offset)
			magic = self.rd.Read(2)
			if magic not in {b'NE', b'DX'}:
				print(f"Error: invalid magic {magic} at 0x{self.rd.Tell():X}", file = sys.stderr)
		if new_header_offset != 0:
			print(f"- Image offset: 0x{new_header_offset:08X}")
		print(f"Magic number: {magic}")
		linker_version = self.rd.ReadWord(1)
		linker_version = (linker_version, self.rd.ReadWord(1))
		print(f"Linker version: {'.'.join(map(str, linker_version))}")
		entry_table_offset = self.rd.ReadWord(2)
		entry_table_length = self.rd.ReadWord(2)
		checksum = self.rd.ReadWord(4)
		print(f"File checksum: 0x{checksum:04X}") # TODO: maybe?
		program_flags = self.rd.ReadWord(1)
		application_flags = self.rd.ReadWord(1)
		dll = (application_flags & 0x80) != 0
		auto_data_segment = self.rd.ReadWord(2)
		print(f"Automatic data segment: 0x{auto_data_segment:04X}")
		heap_size = self.rd.ReadWord(2)
		print(f"Heap size: 0x{heap_size:04X}")
		stack_size = self.rd.ReadWord(2)
		print(f"Stack size: 0x{stack_size:04X}")
		ip = self.rd.ReadWord(2)
		cs = self.rd.ReadWord(2)
		print(f"CS:IP: Segment 0x{cs:04X}:0x{ip:04X}")
		sp = self.rd.ReadWord(2)
		ss = self.rd.ReadWord(2)
		print(f"SS:SP: Segment 0x{ss:04X}:0x{sp:04X}")
		segment_count = self.rd.ReadWord(2)
		module_reference_count = self.rd.ReadWord(2)
		nonresident_name_table_length = self.rd.ReadWord(2)
		segment_table_offset = self.rd.ReadWord(2)
		resource_table_offset = self.rd.ReadWord(2)
		resident_name_table_offset = self.rd.ReadWord(2)
		module_reference_table_offset = self.rd.ReadWord(2)
		imported_name_table_offset = self.rd.ReadWord(2)
		nonresident_name_table_offset = self.rd.ReadWord(4)
		movable_entry_count = self.rd.ReadWord(2)
		sector_shift_count = self.rd.ReadWord(2)
		print(f"Sector shift count: 0x{sector_shift_count:04X}")
		resource_count = self.rd.ReadWord(2)
		os_type = self.rd.ReadWord(1)
		os_type_name = {
			0x01: "OS/2",
			0x02: "Windows",
			0x03: "Multitasking/European MS-DOS 4.x",
			0x04: "Windows 386", # unsure what uses this
			0x05: "Borland Operating System Services",
			0x81: "Phar Lap 286|DOS-Extender, OS/2",
			0x82: "Phar Lap 286|DOS-Extender, Windows",
		}.get(os_type, "unknown")
		print(f"Operating system type: {os_type_name} (0x{os_type:02X})")
		os2 = (os_type & 0x7F) == 1
		exe_flags = self.rd.ReadWord(1)
		print(f"Other flags: 0x{exe_flags:02X}", end = '')
		if exe_flags & 0x01:
			print(", long file names", end = '') # OS/2
		if exe_flags & 0x02:
			print(", proportional fonts", end = '') # Windows 2.x
		if exe_flags & 0x04:
			# TODO: Ralf Brown suggests 0x04 is proportional fonts and 0x02 is protected mode
			# TODO: Watcom calls this 'clean memory'
			print(", protected mode", end = '') # Windows 2.x
		if exe_flags & 0x08:
			print(", gangload area present", end = '') # Windows 2.x
		print()
		print(f"Program flags: 0x{program_flags:02X}", end = '')
		if (program_flags & 3) == 0x00:
			print(", NODATA", end = '')
		elif (program_flags & 3) == 0x01:
			print(", SINGLEDATA", end = '')
		elif (program_flags & 3) == 0x02:
			print(", MULTIPLEDATA", end = '')
		else:
			print(", unknown DATA status", end = '')
		if program_flags & 0x04:
			print(", per-process library initialization", end = '')
		else:
			if dll:
				print(", global library initialization", end = '')
		if program_flags & 0x08:
			print(", protected mode", end = '')
		if program_flags & 0x10:
			print(", LIM EMS", end = '') # directly uses
		if program_flags & 0x20:
			if os2:
				print(", Intel 80286", end = '')
			else:
				print(", per-instance EMS bank", end = '')
		if program_flags & 0x40:
			if os2:
				print(", Intel 80386", end = '')
			else:
				if dll:
					print(", global memory above EMS line", end = '')
		if program_flags & 0x40:
			print(", Intel 80x87", end = '')
		print()
		print(f"Application flags: 0x{application_flags:02X}", end = '')
		if (application_flags & 0x03) == 0x01:
			print(", GUI incompatible", end = '')
		elif (application_flags & 0x03) == 0x02:
			print(", GUI compatible", end = '')
		elif (application_flags & 0x03) == 0x03:
			print(", uses GUI", end = '')
		else:
			print(", unknown GUI status", end = '')
		if application_flags & 0x08:
			if os2:
				print(", Family Application", end = '')
			else:
				print(", first segment loads application", end = '')
		if application_flags & 0x20:
			print(", errors during linking", end = '')
		if application_flags & 0x40:
			if os2:
				# Note: Ralf Brown's claim
				print(", non-conforming program", end = '')
			else:
				if dll:
					print(", private DLL", end = '')
		if dll:
			print(", library (DLL)", end = '')
		else:
			print(", program (EXE)", end = '')
		print()
		extra1 = self.rd.ReadWord(2)
		if os2:
			print(f"Offset to return thunks: 0x{extra1:04X}")
		else:
			print(f"Gangload area offset: 0x{extra1 << sector_shift_count:08X}")
		extra2 = self.rd.ReadWord(2)
		if os2:
			print(f"Offset to segment reference thunks: 0x{extra2:04X}")
		else:
			print(f"Gangload area length: 0x{extra2 << sector_shift_count:08X}")
		swap_size = self.rd.ReadWord(2)
		print(f"Minimum swap size area: 0x{swap_size:04X}")
		windows_version = self.rd.ReadWord(1)
		windows_version = (self.rd.ReadWord(1), windows_version)
		print(f"Windows version: {'.'.join(map(str, windows_version))}")
		print(f"Segment table offset: 0x{segment_table_offset:04X} (0x{new_header_offset + segment_table_offset:04X} in file)")
		print(f"Segment count: 0x{segment_count:04X}")
		print(f"Resource table offset: 0x{resource_table_offset:04X} (0x{new_header_offset + resource_table_offset:04X} in file)")
		print(f"Resource count: 0x{resource_count:04X}")
		print(f"Resident name table offset: 0x{resident_name_table_offset:04X} (0x{new_header_offset + resident_name_table_offset:04X} in file)")
		print(f"Module reference table offset: 0x{module_reference_table_offset:04X} (0x{new_header_offset + module_reference_table_offset:04X} in file)")
		print(f"Module reference count: 0x{module_reference_count:04X}")
		print(f"Imported name table offset: 0x{imported_name_table_offset:04X} (0x{new_header_offset + imported_name_table_offset:04X} in file)")
		print(f"Entry table offset: 0x{entry_table_offset:04X} (0x{new_header_offset + entry_table_offset:04X} in file)")
		print(f"Entry table length: 0x{entry_table_length:04X}")
		print(f"Movable entry count: 0x{movable_entry_count:04X}")
		print(f"Non-Resident name table offset: 0x{nonresident_name_table_offset:08X}")
		print(f"Non-Resident name table length: 0x{nonresident_name_table_length:04X}")

		# Segment table
		print("= Segment table =")
		if self.rd.Tell() != new_header_offset + segment_table_offset:
			print(f"Warning: gap of 0x{new_header_offset + segment_table_offset - self.rd.Tell():08X}", file = sys.stderr)
		self.rd.Seek(new_header_offset + segment_table_offset)
		segment_offsets = []
		segment_lengths = []
		segment_relocatable = []
		for segment_number in range(segment_count):
			segment_offset = self.rd.ReadWord(2) << sector_shift_count
			segment_offsets.append(segment_offset)
			segment_length = self.rd.ReadWord(2)
			if segment_length == 0:
				segment_length = 0x10000
			segment_lengths.append(segment_length)
			segment_flags = self.rd.ReadWord(2)
			segment_relocatable.append((segment_flags & 0x0100) != 0)
			segment_size = self.rd.ReadWord(2)
			if segment_size == 0:
				segment_size = 0x10000
			print(f"Segment #{segment_number + 1}:")
			print(f"Offset: 0x{segment_offset:08X}")
			print(f"Length: 0x{segment_length:04X}")
			print(f"Minimum size: 0x{segment_size:04X}")
			print(f"Flags: 0x{segment_flags:04X}", end = '')
			if segment_flags & 0x0001:
				print(", data", end = '')
			else:
				print(", code", end = '')
			if segment_flags & 0x0002:
				print(", allocated", end = '')
			if segment_flags & 0x0004:
				print(", loaded", end = '')
				# Ralf Brown: real mode
			#if segment_flags & 0x0008:
			#	# Ralf Brown
			#	print(", iterated", end = '')
			if segment_flags & 0x0010:
				print(", movable", end = '')
			if segment_flags & 0x0020:
				print(", sharable", end = '')
			if segment_flags & 0x0040:
				print(", preload", end = '')
			else:
				print(", load on call", end = '')
			if segment_flags & 0x0080:
				if segment_flags & 0x0001:
					print(", read-only", end = '')
				else:
					print(", execute-only", end = '')
			else:
				if segment_flags & 0x0001:
					print(", read-write", end = '')
				else:
					print(", read-execute", end = '')
			if segment_flags & 0x0100:
				print(", has relocations", end = '')
			if segment_flags & 0x0200:
				print(", has debug info", end = '')
			print(f", CPL = {(segment_flags >> 10) & 3}", end = '')
			if segment_flags & 0x1000:
				print(", discardable", end = '')
			# TODO: final bits: discard priority, 32-bit, part of huge segment?
			print()

		# Resource table
		if resource_table_offset != resident_name_table_offset:
			# TODO: untested
			print("= Resource table =")
			if self.rd.Tell() != new_header_offset + resource_table_offset:
				print(f"Warning: gap of 0x{new_header_offset + resource_table_offset - self.rd.Tell():08X}", file = sys.stderr)
			self.rd.Seek(new_header_offset + resource_table_offset)
			resource_shift = self.rd.ReadWord(2)
			print(f"Resource shift: 0x{resource_shift:04X}")
			while True:
				resource_type = self.rd.ReadWord(2)
				if resource_type == 0:
					break
				resource_count = self.rd.ReadWord(2)
				# TODO: type name
				print(f"- Resource type: 0x{resource_type:04X}, count: 0x{resource_count:04X}")
				for resource_number in range(resource_count):
					print(f"Resource #{resource_number + 1}")
					resource_offset = self.rd.ReadWord(2) << resource_shift
					print(f"Offset: 0x{resource_offset:08X}")
					resource_length = self.rd.ReadWord(2)
					print(f"Length: 0x{resource_length:08X}")
					resource_flags = self.rd.ReadWord(2)
					print(f"Flags: 0x{resource_flags:04X}", end = '')
					if resource_flags & 0x0010:
						print(", movable", end = '')
					if resource_flags & 0x0020:
						print(", sharable", end = '')
					if resource_flags & 0x0040:
						print(", preload", end = '')
					else:
						print(", load on call", end = '')
					print()
					resource_id = self.rd.ReadWord(2)
					# TODO: ID name
					print(f"- Resource ID: 0x{resource_id:04X}")
					self.rd.Skip(4)
			print("- Resource name table")
			resource_name_offset = 0
			while True:
				resource_name_length = self.rd.ReadWord(1)
				if resource_name_length == 0:
					break
				resource_name = self.rd.Read(resource_name_length).decode(options.get('encoding', 'cp437_full'))
				print(f"Resource name offset: 0x{resource_name_offset:08X} (0x{self.rd.Tell() - resource_name_length - 1:08X} in file), name: {resource_name}")
				resource_name_offset += resource_name_length + 1

		# Resident name table
		print("= Resident name table =")
		if self.rd.Tell() != new_header_offset + resident_name_table_offset:
			print(f"Warning: gap of 0x{new_header_offset + resident_name_table_offset - self.rd.Tell():08X}", file = sys.stderr)
		self.rd.Seek(new_header_offset + resident_name_table_offset)
		name_offset = 0
		while True:
			name_length = self.rd.ReadWord(1)
			if name_length == 0:
				break
			name = self.rd.Read(name_length).decode(options.get('encoding', 'cp437_full'))
			ordinal = self.rd.ReadWord(2)
			print(f"Name offset: 0x{name_offset:08X} (0x{new_header_offset + resident_name_table_offset + name_offset:08X} in file), name: {name!r}, ordinal: 0x{ordinal:04X}")
			name_offset += name_length + 3

		# Module reference table
		print("= Module reference table =")
		if self.rd.Tell() != new_header_offset + module_reference_table_offset:
			print(f"Warning: gap of 0x{new_header_offset + module_reference_table_offset - self.rd.Tell():08X}", file = sys.stderr)
		self.rd.Seek(new_header_offset + module_reference_table_offset)
		for module_number in range(module_reference_count):
			name_offset = self.rd.ReadWord(2)
			name = self.FetchName(new_header_offset + imported_name_table_offset + name_offset)
			print(f"Module #{module_number + 1}: name {name!r} (offset 0x{name_offset:04X})")

		# Imported name table
		print("= Imported name table =")
		if self.rd.Tell() != new_header_offset + imported_name_table_offset:
			print(f"Warning: gap of 0x{new_header_offset + imported_name_table_offset - self.rd.Tell():08X}", file = sys.stderr)
		self.rd.Seek(new_header_offset + imported_name_table_offset)
		name_offset = 0
		while self.rd.Tell() < new_header_offset + entry_table_offset:
			name_length = self.rd.ReadWord(1)
			name = self.rd.Read(name_length).decode(options.get('encoding', 'cp437_full'))
			print(f"Name offset: 0x{name_offset:08X} (0x{new_header_offset + imported_name_table_offset + name_offset:08X} in file): name: {name!r}")
			name_offset += name_length + 1

		if self.rd.Tell() != new_header_offset + entry_table_offset:
			print(f"Error: imported names overflow into entry table", file = sys.stderr)
		self.rd.Seek(new_header_offset + entry_table_offset)
		entry_index = 0
		while True:
			bundle_count = self.rd.ReadWord(1)
			if bundle_count == 0:
				break
			bundle_offset = self.rd.Tell() - 1
			if bundle_count != 1:
				print(f"-- Entry bundle of {bundle_count} at 0x{bundle_offset:08X} in file")
			entry_type = self.rd.ReadWord(1)
			entry_type_name = {
				0x00: "unused",
				0xFE: "constant",
				0xFF: "movable",
			}.get(entry_type, "fixed")
			for number in range(bundle_count):
				print(f"- Entry #0x{entry_index + number + 1:04X}")
				print(f"Type: {entry_type_name} (0x{entry_type:02X})")
				if entry_type == 0x00:
					continue
				elif entry_type == 0xFF:
					entry_flags = self.rd.ReadWord(1)
					print(f"Flags: 0x{entry_flags:02X}", end = '')
					if entry_flags & 0x01:
						print(", exported", end = '')
					if entry_flags & 0x02:
						print(", shared data", end = '')
					if entry_flags & 0xF8:
						print(f", parameter bytes: 0x{(entry_flags >> 2) & ~1:02X}", end = '')
					print()
					self.rd.Skip(2) # INT 0x3F
					entry_segment = self.rd.ReadWord(1)
					entry_offset = self.rd.ReadWord(2)
					print(f"Segment 0x{entry_segment:02X}:0x{entry_offset:04X}")
				else:
					entry_flags = self.rd.ReadWord(1)
					print(f"Flags: 0x{entry_flags:02X}", end = '')
					if entry_flags & 0x01:
						print(", exported", end = '')
					if entry_flags & 0x02:
						print(", shared data", end = '')
					if entry_flags & 0xF8:
						print(f", parameter bytes: 0x{(entry_flags >> 2) & ~1:02X}", end = '')
					print()
					entry_offset = self.rd.ReadWord(2)
					print(f"Segment 0x{entry_type:02X}:0x{entry_offset:04X}")
			entry_index += bundle_count

		# Non-Resident name table
		print("= Non-Resident name table =")
		if self.rd.Tell() != nonresident_name_table_offset:
			print(f"Warning: gap of 0x{nonresident_name_table_offset - self.rd.Tell():08X}", file = sys.stderr)
		self.rd.Seek(nonresident_name_table_offset)
		name_offset = 0
		while True:
			name_length = self.rd.ReadWord(1)
			if name_length == 0:
				break
			name = self.rd.Read(name_length).decode(options.get('encoding', 'cp437_full'))
			ordinal = self.rd.ReadWord(2)
			print(f"Name offset: 0x{name_offset:08X} (0x{new_header_offset + resident_name_table_offset + name_offset:08X} in file), name: {name!r}, ordinal: 0x{ordinal:04X}")
			name_offset += name_length + 3

		# Segment data
		for segment_number in range(segment_count):
			print(f"= Segment #{segment_number + 1}")
			relocs = {}
			if segment_relocatable[segment_number]:
				print("- Relocations")
				self.rd.Seek(segment_offsets[segment_number] + segment_lengths[segment_number])
				record_count = self.rd.ReadWord(2)
				for record_number in range(record_count):
					rel_type = self.rd.ReadWord(1)
					rel_type_name = {
						0x0: "8-bit offset",
						0x2: "16-bit selector",
						0x3: "16:16-bit pointer",
						0x5: "16-bit offset",
						0xB: "16:32-bit pointer",
						0xD: "32-bit offset",
					}.get(rel_type & 0xF, "undefined")
					rel_size = {
						0x0: 1,
						0x2: 2,
						0x3: 4,
						0x5: 2,
						0xB: 6,
						0xD: 4,
					}.get(rel_type & 0xF, 0)
					print(f"{rel_type_name} (0x{rel_type:02X})", end = '')
					rel_flags = self.rd.ReadWord(1)
					rel_flag_name = {
						0: "internal",
						1: "imported by ordinal",
						2: "imported by name",
						3: "OS fixup",
					}.get(rel_flags & 3)
					print(f" {rel_flag_name}", end = '')
					if rel_flags & 0x04:
						print(", additive", end = '')
					print(f" (0x{rel_flags:02X})", end = '')
					rel_offset = self.rd.ReadWord(2)
					relocs[rel_offset] = rel_size
					print(f" offset 0x{rel_offset:04X} ", end = '') # TODO: chain
					if rel_flags & 3 == 0:
						# internal
						rel_segment = self.rd.ReadWord(1)
						self.rd.Skip(1)
						rel_offset = self.rd.ReadWord(2)
						if rel_segment == 0xFF:
							print(f"entry 0x{rel_offset:04X}", end = '')
						else:
							print(f"segment 0x{rel_segment:02X}:0x{rel_offset:04X}", end = '')
					elif rel_flags & 3 == 1:
						# import by ordinal
						rel_module = self.rd.ReadWord(2)
						rel_ordinal = self.rd.ReadWord(2)
						module_name = self.FetchModuleName(new_header_offset + imported_name_table_offset, new_header_offset + module_reference_table_offset + 2 * (rel_module - 1)).decode('ascii_graphic')
						print(f"module {module_name!r} (0x{rel_module:04X}) ordinal 0x{rel_ordinal:04X}", end = '')
					elif rel_flags & 3 == 2:
						# import by name
						rel_module = self.rd.ReadWord(2)
						rel_name = self.rd.ReadWord(2)
						module_name = self.FetchModuleName(new_header_offset + imported_name_table_offset, new_header_offset + module_reference_table_offset + 2 * (rel_module - 1)).decode('ascii_graphic')
						procedure_name = self.FetchName(new_header_offset + imported_name_table_offset + rel_name).decode('ascii_graphic')
						print(f"module {module_name!r} (0x{rel_module:04X}) name {procedure_name!r} (0x{rel_name:08X})", end = '')
					elif rel_flags & 3 == 3:
						# OS fixup
						self.rd.Skip(4) # TODO
					print()
			if options.get('data', False) or options.get('showall', False):
				print("- Data")
				self.rd.Seek(segment_offsets[segment_number])
				print("[FILE    ] SEGMENT \tDATA")
				for row, hexcodes, text in self.process_data(options, 4, segment_lengths[segment_number],
					get_reloc_size = lambda position: relocs.get(position), text_encoding = 'cp437_full'):
					print(f"[{segment_offsets[segment_number] + row:08X}] {row:04X}\t{hexcodes}\t{text}")

class LEReader(FileReader):
	ENDIAN_NAMES = {0: 'little endian', 1: 'big endian'}
	def __init__(self, rd):
		super(LEReader, self).__init__(rd)
		self.rd.endian = Reader.LittleEndian

	def FetchName(self, offset):
		pos = self.rd.Tell()
		self.rd.Seek(offset)
		name_length = self.rd.ReadWord(1)
		name = self.rd.Read(name_length)
		if len(name) < name_length:
			name += bytes(name_length - len(name))
		self.rd.Seek(pos)
		return name

	def FetchModuleName(self, offset, index):
		pos = self.rd.Tell()
		self.rd.Seek(offset)
		for count in range(index):
			self.rd.Skip(self.rd.ReadWord(1))
		name_length = self.rd.ReadWord(1)
		name = self.rd.Read(name_length)
		if len(name) < name_length:
			name += bytes(name_length - len(name))
		self.rd.Seek(pos)
		return name

	def ReadFile(self, **options):
		print("==== LE/LX .EXE format ====")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(0)
		magic = self.rd.Read(2)
		if magic in {b'LE', b'LX'}:
			print("Stubless image")
			new_header_offset = 0
		else:
			if magic != b'MZ':
				print(f"Error: invalid stub magic {magic}", file = sys.stderr)
			self.rd.Seek(0x18)
			reloc_offset = self.rd.ReadWord(2)
			if reloc_offset != 0x40:
				print(f"Warning: stub relocation offset at 0x18 is supposed to be 0x0040, received: 0x{reloc_offset:04X}", file = sys.stderr)
			self.rd.Seek(0x3C)
			new_header_offset = self.rd.ReadWord(4)
			self.rd.Seek(new_header_offset)
			magic = self.rd.Read(2)
			if magic not in {b'LE', b'LX'}:
				print(f"Error: invalid magic {magic} at 0x{self.rd.Tell():X}", file = sys.stderr)
		if new_header_offset != 0:
			print(f"- Image offset: 0x{new_header_offset:08X}")
		print(f"Magic number: {magic}")
		byte_order = self.rd.ReadWord(1)
		byte_order_name = LEReader.ENDIAN_NAMES.get(byte_order, "invalid")
		print(f"Byte order: {byte_order_name} (0x{byte_order:02X})")
		word_order = self.rd.ReadWord(1)
		word_order_name = LEReader.ENDIAN_NAMES.get(word_order, "invalid")
		print(f"Word order: {word_order_name} (0x{word_order:02X})")
		if byte_order == 0:
			if word_order == 0:
				self.rd.endian = Reader.LittleEndian
			elif word_order == 1:
				self.rd.endian = Reader.PDP11Endian # presumably
		elif byte_order == 1:
			if word_order == 0:
				self.rd.endian = Reader.AntiPDP11Endian # presumably
			elif word_order == 1:
				self.rd.endian = Reader.BigEndian
		format_level = self.rd.ReadWord(4)
		if format_level != 0:
			print(f"Warning: unknown format level 0x{format_level:08X} might impact parsing", file = sys.stderr)
		print(f"Format level: 0x{format_level:08X}")
		cpu_type = self.rd.ReadWord(2)
		cpu_type_name = {
			0x01: "Intel 80286",
			0x02: "Intel 80386",
			0x03: "Intel 80486",
			0x04: "Intel Pentium/80586",
			0x20: "Intel i860 (N10)",
			0x21: "Intel i860 (N11)",
			0x40: "MIPS Mark I (R2000, R3000)",
			0x41: "MIPS Mark II (R6000)",
			0x42: "MIPS Mark III (R4000)",
		}.get(cpu_type, "unknown")
		print(f"CPU type: {cpu_type_name} (0x{cpu_type:04X})")
		os_type = self.rd.ReadWord(2)
		os_type_name = {
			0x01: "OS/2 or DOS/4G",
			0x02: "Windows", # unsure what uses this
			0x03: "Multitasking/European MS-DOS 4.x",
			0x04: "Windows 386",
			0x05: "IBM Microkernel Personality Neutral",
		}.get(os_type, "unknown")
		print(f"Operating system type: {os_type_name} (0x{os_type:04X})")
		module_version = self.rd.ReadWord(4)
		print(f"Module version: 0x{module_version:08X}")
		module_flags = self.rd.ReadWord(4)
		print(f"Module flags: 0x{module_flags:08X}", end = '')
		dll = (module_flags & 0x00038000) == 0x00008000
		if module_flags & 0x00000001:
			print(", single data", end = '') # Watcom
		if module_flags & 0x00000004:
			print(", per-process library initialization", end = '')
		else:
			if dll:
				print(", global library initialization", end = '')
		if module_flags & 0x00000010:
			print(", no internal fixups", end = '')
		if module_flags & 0x00000020:
			print(", no external fixups", end = '')
		if (module_flags & 0x00000300) == 0x00000100:
			print(", GUI incompatible", end = '')
		elif (module_flags & 0x00000300) == 0x00000200:
			print(", GUI compatible", end = '')
		elif (module_flags & 0x00000300) == 0x00000300:
			print(", uses GUI", end = '')
		else:
			print(", unknown GUI status", end = '')
		if module_flags & 0x00001000:
			print(", non-loadable", end = '')
		if (module_flags & 0x00038000) == 0x00000000:
			print(", program (EXE)", end = '')
		elif (module_flags & 0x00038000) == 0x00008000:
			print(", library (DLL)", end = '')
		elif (module_flags & 0x00038000) == 0x00018000:
			print(", protected memory library (DLL)", end = '') # earlier OS/2
		elif (module_flags & 0x00038000) == 0x00020000:
			print(", physical device driver", end = '')
		elif (module_flags & 0x00038000) == 0x00028000:
			print(", virtual device driver", end = '')
		else:
			print(", unknown module type", end = '')
		if module_flags & 0x00080000:
			print(", MP-unsafe", end = '') # later OS/2
		if module_flags & 0x40000000:
			print(", per-process library termination", end = '')
		else:
			if dll:
				print(", global library termination", end = '')
		print()
		page_count = self.rd.ReadWord(4)
		print(f"Total page count: 0x{page_count:08X}")
		eip_object = self.rd.ReadWord(4)
		eip = self.rd.ReadWord(4)
		print(f"EIP = Object 0x{eip_object:X}:0x{eip:08X}")
		esp_object = self.rd.ReadWord(4)
		esp = self.rd.ReadWord(4)
		print(f"ESP = Object 0x{esp_object:X}:0x{esp:08X}")
		page_size = self.rd.ReadWord(4)
		print(f"Page size: 0x{page_size:08X}")
		if magic == b'LE':
			last_page_size = self.rd.ReadWord(4)
			print(f"Last page size: 0x{last_page_size:08X}")
		elif magic == b'LX':
			page_shift_count = self.rd.ReadWord(4)
			print(f"Page shift count: 0x{page_shift_count:08X}")
		fixup_section_size = self.rd.ReadWord(4)
		print(f"Fixup section size: 0x{fixup_section_size:08X}")
		fixup_section_checksum = self.rd.ReadWord(4)
		print(f"Fixup section checksum: 0x{fixup_section_checksum:08X}") # TODO: check maybe?
		loader_section_size = self.rd.ReadWord(4)
		print(f"Loader section size: 0x{loader_section_size:08X}")
		loader_section_checksum = self.rd.ReadWord(4)
		print(f"Loader section checksum: 0x{loader_section_checksum:08X}") # TODO: check maybe?
		object_table_offset = self.rd.ReadWord(4)
		print(f"Object table offset: 0x{object_table_offset:08X} (0x{new_header_offset + object_table_offset:08X} in file)")
		object_count = self.rd.ReadWord(4)
		print(f"Object count: 0x{object_count:08X}")
		print(f"Object table length: 0x{object_count * 24:08X}")
		object_page_table_offset = self.rd.ReadWord(4)
		print(f"Object page table offset: 0x{object_page_table_offset:08X} (0x{new_header_offset + object_page_table_offset:08X} in file)")
		object_iterated_page_table_offset = self.rd.ReadWord(4)
		if object_iterated_page_table_offset != 0:
			print(f"Object iterated page table offset: 0x{object_iterated_page_table_offset:08X} (0x{new_header_offset + object_iterated_page_table_offset:08X} in file)")
		resource_table_offset = self.rd.ReadWord(4)
		print(f"Resource table offset: 0x{resource_table_offset:08X} (0x{new_header_offset + resource_table_offset:08X} in file)")
		resource_count = self.rd.ReadWord(4)
		print(f"Resource count: 0x{resource_count:08X}")
		print(f"Resource table length: 0x{resource_count * 14:08X}")
		resident_name_table_offset = self.rd.ReadWord(4)
		print(f"Resident name table offset: 0x{resident_name_table_offset:08X} (0x{new_header_offset + resident_name_table_offset:08X} in file)")
		entry_table_offset = self.rd.ReadWord(4)
		print(f"Entry table offset: 0x{entry_table_offset:08X} (0x{new_header_offset + entry_table_offset:08X} in file)")
		module_directives_table_offset = self.rd.ReadWord(4)
		module_directive_count = self.rd.ReadWord(4)
		if module_directives_table_offset != 0 or module_directive_count != 0:
			print(f"Module format directives table offset: 0x{module_directives_table_offset:08X} (0x{new_header_offset + module_directives_table_offset:08X} in file)")
			print(f"Module format directive count: 0x{module_directive_count:08X}") # TODO
		fixup_page_table_offset = self.rd.ReadWord(4)
		print(f"Fixup page table offset: 0x{fixup_page_table_offset:08X} (0x{new_header_offset + fixup_page_table_offset:08X} in file)")
		fixup_record_table_offset = self.rd.ReadWord(4)
		print(f"Fixup record table offset: 0x{fixup_record_table_offset:08X} (0x{new_header_offset + fixup_record_table_offset:08X} in file)")
		imported_module_table_offset = self.rd.ReadWord(4)
		print(f"Imported module table offset: 0x{imported_module_table_offset:08X} (0x{new_header_offset + imported_module_table_offset:08X} in file)")
		imported_module_count = self.rd.ReadWord(4)
		print(f"Imported module count: 0x{imported_module_count:08X}")
		# TODO: size
		imported_procedure_table_offset = self.rd.ReadWord(4)
		print(f"Imported procedure table offset: 0x{imported_procedure_table_offset:08X} (0x{new_header_offset + imported_procedure_table_offset:08X} in file)")
		# TODO: size
		per_page_checksum_table_offset = self.rd.ReadWord(4)
		if per_page_checksum_table_offset != 0:
			print(f"Per-page checksum table offset: 0x{per_page_checksum_table_offset:08X} (0x{new_header_offset + per_page_checksum_table_offset:08X} in file)")
		data_pages_offset = self.rd.ReadWord(4)
		print(f"Data pages offset: 0x{data_pages_offset:08X}")
		preload_page_count = self.rd.ReadWord(4)
		if preload_page_count != 0:
			print(f"Preload page count: 0x{preload_page_count:08X}")
		nonresident_name_table_offset = self.rd.ReadWord(4)
		print(f"Non-Resident name table offset: 0x{nonresident_name_table_offset:08X}")
		nonresident_name_table_length = self.rd.ReadWord(4)
		print(f"Non-Resident name table length: 0x{nonresident_name_table_length:08X}")
		nonresident_name_table_checksum = self.rd.ReadWord(4)
		print(f"Non-Resident name table checksum: 0x{nonresident_name_table_checksum:08X}")
		auto_data_segment_object = self.rd.ReadWord(4)
		print(f"Automatic data segment object: 0x{auto_data_segment_object:08X}")
		# Note: later OS/2 states this is from beginning of file and previous versions were incorrect
		debug_info_offset = self.rd.ReadWord(4)
		debug_info_length = self.rd.ReadWord(4)
		if debug_info_offset != 0 or debug_info_length != 0:
			print(f"Debug information offset: 0x{debug_info_offset:08X}")
			print(f"Debug information length: 0x{debug_info_length:08X}")
		preload_instance_page_count = self.rd.ReadWord(4)
		if preload_instance_page_count != 0:
			print(f"Preload instance page count: 0x{preload_instance_page_count:08X}")
		demand_instance_page_count = self.rd.ReadWord(4)
		if demand_instance_page_count != 0:
			print(f"Demand instance page count: 0x{demand_instance_page_count:08X}")
		heap_size = self.rd.ReadWord(4)
		print(f"Heap size: 0x{heap_size}")
		stack_size = self.rd.ReadWord(4)
		print(f"Stack size: 0x{stack_size}")
		self.rd.Skip(8)
		version_info_resource_offset = self.rd.ReadWord(4)
		version_info_resource_length = self.rd.ReadWord(4)
		if version_info_resource_offset != 0 or version_info_resource_length != 0:
			print(f"Windows VxD version info resource offset: 0x{version_info_resource_offset:08X}") # TODO: from where?
			print(f"Windows VxD version info resource length: 0x{version_info_resource_length:08X}")
		device_id = self.rd.ReadWord(2)
		if device_id != 0:
			print(f"Windows VxD device ID: 0x{device_id:08X}")
		ddk_version = self.rd.ReadWord(2)
		if ddk_version != 0:
			print(f"Windows VxD DDK version: 0x{ddk_version:08X}")
		#print(hex(self.rd.Tell() - new_header_offset))

		### Loader section
		print("=== Loader section ===")
		# - Object table
		print("= Object table =")
		if self.rd.Tell() != new_header_offset + object_table_offset:
			print(f"Warning: gap of 0x{new_header_offset + object_table_offset - self.rd.Tell():08X}", file = sys.stderr)
		self.rd.Seek(new_header_offset + object_table_offset)
		object_bases = []
		page_objects = [None] * page_count
		for number in range(object_count):
			print(f"- Object #{number + 1}")
			virtual_size = self.rd.ReadWord(4)
			print(f"Total size: 0x{virtual_size:08X}")
			base_address = self.rd.ReadWord(4)
			print(f"Base address: 0x{base_address:08X}")
			object_bases.append(base_address)
			flags = self.rd.ReadWord(4)
			print(f"Flags: 0x{flags:08X}", end = '')
			if flags & 0x00000002:
				if flags & 0x00000001:
					print(f", readable", end = '')
				print(f", writable", end = '')
			elif flags & 0x00000001:
				print(f", read-only", end = '')
			if flags & 0x00000004:
				print(f", executable", end = '')
			if flags & 0x00000008:
				print(f", resource", end = '')
			if flags & 0x00000010:
				print(f", discardable", end = '')
			if flags & 0x00000020:
				print(f", shared", end = '')
			if flags & 0x00000040:
				print(f", has preload pages", end = '')
			if flags & 0x00000080:
				print(f", has invalid pages", end = '')
			if (flags & 0x00000700) == 0x00000100:
				print(f", has zero-filled pages", end = '')
			elif (flags & 0x00000700) == 0x00000200:
				print(f", resident", end = '') # device driver
			elif (flags & 0x00000700) == 0x00000300:
				print(f", resident and contiguous", end = '') # device driver
			elif (flags & 0x00000700) == 0x00000400:
				print(f", resident and long-lockable", end = '') # device driver
			if flags & 0x00000800:
				print(f", IBM Microkernel extension", end = '') # later OS/2
			if flags & 0x00001000:
				print(f", 16:16 alias required", end = '') # x86
			if flags & 0x00002000:
				print(f", 32-bit", end = '') # x86
			if flags & 0x00004000:
				print(f", conforming code", end = '') # x86
			if flags & 0x00008000:
				print(f", I/O privilege level", end = '') # x86
			print()
			page_table_index = self.rd.ReadWord(4)
			print(f"Page table index: 0x{page_table_index:08X}")
			page_table_count = self.rd.ReadWord(4)
			print(f"Page table count: 0x{page_table_count:08X}")
			self.rd.Skip(4)
			for i in range(page_table_count):
				if page_objects[page_table_index + i - 1] is not None:
					print(f"Error: Overlapping pages by object #0x{number + 1:08X} and #0x{page_objects[page_table_index + i]:08X}", file = sys.stderr)
				page_objects[page_table_index + i - 1] = number

		# - Object page table
		print("= Object page table =")
		if self.rd.Tell() != new_header_offset + object_page_table_offset:
			print(f"Warning: gap of 0x{new_header_offset + object_page_table_offset - self.rd.Tell():08X}", file = sys.stderr)
		self.rd.Seek(new_header_offset + object_page_table_offset)
		page_offsets = []
		page_sizes = []
		if magic == b'LE':
			for number in range(page_count):
				if page_objects[number] is None:
					print(f"Error: page #{number + 1} does not belong to an object", file = sys.stderr)
					print(f"- Page #{number + 1}")
				else:
					print(f"- Page #{number + 1} in object #{page_objects[number] + 1}")
				page_fixup_index = self.rd.ReadWord(3, endian = Reader.BigEndian)
				print(f"Page fixup index: 0x{page_fixup_index:06X}")
				page_type = self.rd.ReadWord(1)
				print(f"Page type: 0x{page_type:02X}")
				page_offsets.append(page_size * number)
				page_sizes.append(page_size if number != page_count - 1 else last_page_size)
		elif magic == b'LX':
			for number in range(page_count):
				if page_objects[number] is None:
					print(f"Error: page #{number + 1} does not belong to an object", file = sys.stderr)
					print(f"- Page #{number + 1}")
				else:
					print(f"- Page #{number + 1} in object #{page_objects[number] + 1}")
				page_data_offset = self.rd.ReadWord(4) << page_shift_count
				print(f"Page data offset: 0x{page_data_offset:08X}")
				page_data_size = self.rd.ReadWord(2) << page_shift_count
				print(f"Page size: 0x{page_data_size:04X}")
				page_flags = self.rd.ReadWord(2) << page_shift_count
				page_type = {
					0: 'data',
					1: 'iterated',
					2: 'invalid',
					3: 'zero-filled',
					4: 'range',
					5: 'compressed'
				}.get(page_flags, "unknown")
				print(f"Page flags: {page_type}, 0x{page_flags:04X}")
				page_offsets.append(page_data_offset)
				page_sizes.append(page_data_size)

		# - Resource table
		print("= Resource table =")
		if self.rd.Tell() != new_header_offset + resource_table_offset:
			print(f"Warning: gap of 0x{new_header_offset + resource_table_offset - self.rd.Tell():08X}", file = sys.stderr)
		self.rd.Seek(new_header_offset + resource_table_offset)
		for number in range(resource_count):
			print(f"- Resource #{number + 1}")
			resource_type = self.rd.ReadWord(2)
			resource_type_name = {
				0x01: "RT_POINTER (cursor)",
				0x02: "RT_BITMAP",
				0x03: "RT_MENU",
				0x04: "RT_DIALOG",
				0x05: "RT_STRING",
				0x06: "RT_FONTDIR",
				0x07: "RT_FONT",
				0x08: "RT_ACCELERATOR",
				0x09: "RT_RCDATA (binary)",
				0x0A: "RT_MESSAGE (error message)",
				0x0B: "RT_DLGINCLUDE",
				0x0C: "RT_VKEYTBL",
				0x0D: "RT_KEYTBL",
				0x0E: "RT_CHARTBL",
				0x0F: "RT_DISPLAYINFO",
				0x10: "RT_FKASHORT (function key area)",
				0x11: "RT_FKALONG (function key area)",
				0x12: "RT_HELPTABLE",
				0x13: "RT_HELPSUBTABLE",
				0x14: "RT_FDDIR",
				0x15: "RT_FD",
			}.get(resource_type, "unknown")
			print(f"Resource type: {resource_type_name} (0x{resource_type:04X})")
			resource_id = self.rd.ReadWord(2)
			print(f"Resource ID: 0x{resource_id:04X}")
			resource_size = self.rd.ReadWord(4)
			print(f"Resource size: 0x{resource_size:08X}")
			resource_object = self.rd.ReadWord(2)
			resource_offset = self.rd.ReadWord(4)
			print(f"Resource position: Object 0x{resource_object:04X}:0x{resource_offset:08X}")

		# - Resident name table
		print("= Resident name table =")
		if self.rd.Tell() != new_header_offset + resident_name_table_offset:
			print(f"Warning: gap of 0x{new_header_offset + resident_name_table_offset - self.rd.Tell():08X}", file = sys.stderr)
		self.rd.Seek(new_header_offset + resident_name_table_offset)
		name_offset = 0
		while True:
			name_length = self.rd.ReadWord(1)
			if name_length == 0:
				break
			name = self.rd.Read(name_length)
			name_ordinal = self.rd.ReadWord(2)
			print(f"Offset 0x{name_offset:04X}: name {name!r}, ordinal 0x{name_ordinal:04X}")
			name_offset += name_length + 3

		# - Entry table
		print("= Entry table =")
		if self.rd.Tell() != new_header_offset + entry_table_offset:
			print(f"Warning: gap of 0x{new_header_offset + entry_table_offset - self.rd.Tell():08X}", file = sys.stderr)
		self.rd.Seek(new_header_offset + entry_table_offset)
		entry_index = 0
		while True:
			entry_count = self.rd.ReadWord(1)
			if entry_count == 0:
				break
			if entry_count != 1:
				print(f"-- Entry bundle of {entry_count}")
			entry_type = self.rd.ReadWord(1)
			entry_type_name = {
				0: "unused",
				1: "16-bit",
				2: "286 call gate",
				3: "32-bit",
				4: "forwarder",
			}.get(entry_type & 0x7, "unknown")
			if entry_type & 0x80:
				entry_type_name += " with parameter typing information"
			if (entry_type & 0x7) == 0:
				pass
			elif (entry_type & 0x7) == 1:
				# 16-bit
				entry_object = self.rd.ReadWord(2)
			elif (entry_type & 0x7) == 2:
				# 286 call gate
				entry_object = self.rd.ReadWord(2)
			elif (entry_type & 0x7) == 3:
				# 32-bit
				entry_object = self.rd.ReadWord(2)
			elif (entry_type & 0x7) == 4:
				# forwarder
				self.rd.Skip(2) # reserved
			for number in range(entry_count):
				print(f"- Entry #0x{entry_index + number:04X}")
				print(f"Type: {entry_type_name} (0x{entry_type:02X})")
				if (entry_type & 0x7) == 0:
					pass
				elif (entry_type & 0x7) == 1:
					# 16-bit
					entry_flags = self.rd.ReadWord(1)
					entry_offset = self.rd.ReadWord(2)
					print(f"Object 0x{entry_object:04X}:0x{entry_offset:04X}, flags: {entry_flags}", end = '')
					if entry_flags & 1:
						print(", exported", end = '')
					if entry_flags & 0xF8:
						print(", parameter bytes: 0x{(entry_flags >> 2) & ~1:02X}", end = '')
					print()
				elif (entry_type & 0x7) == 2:
					# 286 call gate
					entry_flags = self.rd.ReadWord(1)
					entry_offset = self.rd.ReadWord(2)
					self.rd.Skip(2) # callgate selector reserved field
					print(f"Object 0x{entry_object:04X}:0x{entry_offset:04X}, flags: {entry_flags}", end = '')
					if entry_flags & 1:
						print(", exported", end = '')
					if entry_flags & 0xF8:
						print(", parameter bytes: 0x{(entry_flags >> 2) & ~1:02X}", end = '')
					print()
				elif (entry_type & 0x7) == 3:
					# 16-bit
					entry_flags = self.rd.ReadWord(1)
					entry_offset = self.rd.ReadWord(4)
					print(f"Object 0x{entry_object:04X}:0x{entry_offset:08X}, flags: {entry_flags}", end = '')
					if entry_flags & 1:
						print(", exported", end = '')
					if entry_flags & 0xF8:
						print(", parameter bytes: 0x{(entry_flags >> 1) & ~3:02X}", end = '')
					print()
				elif (entry_type & 0x7) == 3:
					# forwarder
					entry_flags = self.rd.ReadWord(1)
					entry_module = self.rd.ReadWord(2)
					entry_offset = self.rd.ReadWord(2)
					module_name = self.FetchModuleName(new_header_offset + imported_module_table_offset, rel_module - 1).decode('ascii_graphic')
					if entry_flags & 1:
						print(f"Module {module_name} (0x{entry_object:04X}):ordinal 0x{entry_offset:02X}, flags: {entry_flags}")
					else:
						print(f"Module {module_name} (0x{entry_object:04X}):procedure 0x{entry_offset:02X}, flags: {entry_flags}")
			entry_index += entry_count

		# - Module format directives table
		# TODO
		# - Verify record directive table
		# TODO
		# - Per-page checksum
		# TODO

		### Fixup section
		# - Fixup page table
		print("= Fixup page table =")
		if self.rd.Tell() != new_header_offset + fixup_page_table_offset:
			print(f"Warning: gap of 0x{new_header_offset + fixup_page_table_offset - self.rd.Tell():08X}", file = sys.stderr)
		self.rd.Seek(new_header_offset + fixup_page_table_offset)
		page_fixup_offsets = []
		for number in range(page_count + 1):
			page_fixup_offset = self.rd.ReadWord(4)
			if number == page_count:
				print(f"Page fixup offset end: 0x{page_fixup_offset:08X}")
			else:
				print(f"Page #0x{number + 1:08X} fixup offset: 0x{page_fixup_offset:08X}")
			page_fixup_offsets.append(page_fixup_offset)

		# - Fixup record table
		print("= Fixup record table =")
		if self.rd.Tell() != new_header_offset + fixup_record_table_offset:
			print(f"Warning: gap of 0x{new_header_offset + fixup_record_table_offset - self.rd.Tell():08X}", file = sys.stderr)
		self.rd.Seek(new_header_offset + fixup_record_table_offset)
		current_page = 0
		page_relocs = []
		if options.get('rel', False) or options.get('showall', False):
			while True:
				while self.rd.Tell() - (new_header_offset + fixup_record_table_offset) >= page_fixup_offsets[current_page]:
					skip_count = self.rd.Tell() - (new_header_offset + fixup_record_table_offset) - page_fixup_offsets[current_page]
					if skip_count > 0:
						print(f"Warning: skipped over 0x{skip_count:X} bytes to page fixup table for the next page", file = sys.stderr)
					current_page += 1
					if current_page == page_count + 1:
						break
					print(f"- Relocations for page #0x{current_page:08X}")
					page_relocs.append({})
				if current_page == page_count + 1:
					break
				src = self.rd.ReadWord(1)
				src_name = {
					0x0: "8-bit offset",
					0x2: "16-bit selector",
					0x3: "16:16-bit pointer",
					0x5: "16-bit offset",
					0x6: "16:32-bit pointer",
					0x7: "32-bit offset",
					0x8: "32-bit self-relative offset",
				}.get(src & 0xF, "undefined")
				src_size = {
					0: 1, 2: 2, 3: 4, 5: 2, 6: 6, 7: 4, 8: 4
				}.get(src & 0xF, 0)
				print(f"{src_name}", end = '')
				if src & 0x10:
					print(", 16-bit alias", end = '')
				if src & 0x20:
					print(", source list", end = '')
				print(f" (0x{src:02X})", end = '')
				flags = self.rd.ReadWord(1)
				target_name = {
					0: "internal",
					1: "imported by ordinal",
					2: "imported by name",
					3: "internal entry",
				}[flags & 3]
				print(f" {target_name}", end = '')
				if flags & 0x04:
					print(", additive", end = '')
				if flags & 0x08:
					print(", chained", end = '')
				if flags & 0x10:
					print(", 32-bit target offset", end = '')
				if flags & 0x20:
					print(", 32-bit additive fixup", end = '')
				if flags & 0x40:
					print(", 16-bit number/ordinal", end = '')
				if flags & 0x80:
					print(", 8-bit ordinal", end = '')
				print(f" (0x{flags:02X}) to ", end = '')
				if src & 0x20:
					cnt = self.rd.ReadWord(1)
				else:
					srcoff = self.rd.ReadWord(2)
				if flags & 3 == 0:
					# internal
					rel_object = self.rd.ReadWord(2 if flags & 0x40 else 1)
					if src & 0xF != 2:
						rel_offset = self.rd.ReadWord(4 if flags & 0x10 else 2)
						print(f"object 0x{rel_object:04X}:0x{rel_offset:08X}", end = '')
					else:
						print(f"object 0x{rel_object:04X}", end = '')
				elif flags & 3 == 1:
					# import by ordinal
					rel_module = self.rd.ReadWord(2 if flags & 0x40 else 1)
					rel_ordinal = self.rd.ReadWord(1 if flags & 0x80 else 4 if flags & 0x10 else 2)
					module_name = self.FetchModuleName(new_header_offset + imported_module_table_offset, rel_module - 1).decode('ascii_graphic')
					print(f"module {module_name} (0x{rel_module:04X}) ordinal 0x{rel_ordinal:08X}", end = '')
					if flags & 0x04:
						rel_add = self.rd.ReadWord(4 if flags & 0x20 else 2)
						print(f", add 0x{rel_add:08X}", end = '')
				elif flags & 3 == 2:
					# import by name
					rel_module = self.rd.ReadWord(2 if flags & 0x40 else 1)
					rel_name = self.rd.ReadWord(4 if flags & 0x10 else 2)
					module_name = self.FetchModuleName(new_header_offset + imported_module_table_offset, rel_module - 1).decode('ascii_graphic')
					procedure_name = self.FetchName(new_header_offset + imported_procedure_table_offset + rel_name).decode('ascii_graphic')
					print(f"module {module_name} (0x{rel_module:04X}) name {procedure_name} (0x{rel_name:08X})", end = '')
					if flags & 0x04:
						rel_add = self.rd.ReadWord(4 if flags & 0x20 else 2)
						print(f", add 0x{rel_add:08X}", end = '')
				elif flags & 3 == 3:
					# internal entry
					rel_ordinal = self.rd.ReadWord(1 if flags & 0x80 else 4 if flags & 0x10 else 2)
					print(f"entry 0x{rel_ordinal:08X}", end = '')
					if flags & 0x04:
						rel_add = self.rd.ReadWord(4 if flags & 0x20 else 2)
						print(f", add 0x{rel_add:08X}", end = '')
				# TODO: chained
				if src & 0x20:
					srcoffs = []
					for number in range(cnt):
						srcoffs.append(self.rd.ReadWord(2))
						page_relocs[-1][srcoff] = src_size
					print(", offsets: " + ", ".join(f"0x{srcoff:04X}" for srcoff in srcoffs))
				else:
					print(f", offset: 0x{srcoff:04X}")
					page_relocs[-1][srcoff] = src_size

		# - Imported module name table
		print("= Imported module name table =")
		if self.rd.Tell() != new_header_offset + imported_module_table_offset:
			print(f"Warning: gap of 0x{new_header_offset + imported_module_table_offset - self.rd.Tell():08X}", file = sys.stderr)
		self.rd.Seek(new_header_offset + imported_module_table_offset)
		name_offset = 0
		for name_count in range(imported_module_count):
			name_length = self.rd.ReadWord(1)
			name = self.rd.Read(name_length)
			print(f"Module 0x{name_count + 1:04X}, offset 0x{name_offset:04X}: name {name!r}")
			name_offset += name_length + 1

		# - Imported procedure name table
		print("= Imported procedure name table =")
		if self.rd.Tell() != new_header_offset + imported_procedure_table_offset:
			print(f"Warning: gap of 0x{new_header_offset + imported_procedure_table_offset - self.rd.Tell():08X}", file = sys.stderr)
		self.rd.Seek(new_header_offset + imported_procedure_table_offset)
		name_offset = 0
		while self.rd.Tell() < new_header_offset + fixup_page_table_offset + fixup_section_size:
			name_length = self.rd.ReadWord(1)
			name = self.rd.Read(name_length)
			print(f"Procedure offset 0x{name_offset:04X}: name {name!r}")
			name_offset += name_length + 1

		### Data section
		print("= Data pages =")

		if options.get('data', False) or options.get('showall', False):
			for page_number in range(page_count):
				self.rd.Seek(data_pages_offset + page_offsets[page_number])
				print(f"Page #0x{page_number + 1:X} data")
				print("[FILE    ] (PAGE) OBJECT  \tDATA")
				file_offset = self.rd.Tell()
				for row, hexcodes, text in self.process_data(options, 4, page_sizes[page_number],
						get_reloc_size = lambda position: page_relocs[page_number].get(position), text_encoding = 'cp437_full'):
					print(f"[{file_offset + row:08X}] ({row:04X}) {object_bases[page_objects[page_number]] + row:08X}\t{hexcodes}\t{text}")

			# TODO: iterated pages

			# TODO: debug information

class PEReader(FileReader):
	def __init__(self, rd):
		super(PEReader, self).__init__(rd)
		self.rd.endian = Reader.LittleEndian
		self.image_base = 0
		self.memory_map = {}

	def GetOffset(self, rva):
		tmp = set(sect_rva for sect_rva in self.memory_map.keys() if sect_rva <= rva)
		if len(tmp) == 0:
			return None, None, None
		tmp = max(tmp)
		disp, name = self.memory_map[tmp]
		if disp is not None:
			off = rva - tmp
			disp += rva
		else:
			off = None
		return disp, name, off

	def PrintRva(self, rva):
		offset, section, location = self.GetOffset(rva)
		text = f"0x{rva:08X} (0x{self.image_base + rva:08X} virtual)"
		if offset is not None:
			text += f" [{section}:0x{location:X}, 0x{offset:08X} in file]"
		else:
			text += " [not in file]"
		return text

	def FetchName(self, rva):
		offset = self.GetOffset(rva)[0]
		if offset is None:
			return None
		pos = self.rd.Tell()
		self.rd.Seek(offset)
		name = self.rd.ReadToZero()
		self.rd.Seek(pos)
		return name

	def FetchHintName(self, rva):
		offset = self.GetOffset(rva)[0]
		if offset is None:
			return None, None
		pos = self.rd.Tell()
		self.rd.Seek(offset)
		hint = self.rd.ReadWord(2)
		name = self.rd.ReadToZero()
		self.rd.Seek(pos)
		return hint, name

	def ReadFile(self, **options):
		print("==== Windows PE .EXE format ====")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(0)
		magic = self.rd.Read(4)
		if magic in {b'PE\0', b'PL\0'}:
			print("Stubless image")
			new_header_offset = 0
		else:
			if magic[:2] != b'MZ':
				print(f"Error: invalid stub magic {magic}", file = sys.stderr)
			self.rd.Seek(0x18)
			reloc_offset = self.rd.ReadWord(2)
			if reloc_offset != 0x40:
				print(f"Warning: stub relocation offset at 0x18 is supposed to be 0x0040, received: 0x{reloc_offset:04X}", file = sys.stderr)
			self.rd.Seek(0x3C)
			new_header_offset = self.rd.ReadWord(4)
			self.rd.Seek(new_header_offset)
			magic = self.rd.Read(4)
			if magic not in {b'PE\0\0', b'PL\0\0'}:
				print(f"Error: invalid magic {magic} at 0x{self.rd.Tell():X}", file = sys.stderr)
		if new_header_offset != 0:
			print(f"- Image offset: 0x{new_header_offset:08X}")
		print(f"Magic number: {magic}")
		machine = self.rd.ReadWord(2)
		machine_name = {
			0x0000: "unknown",
			0x014C: "Intel i386",
			0x014D: "Intel i860", # undocumented
			0x8664: "AMD64",
			0x0162: "MIPS R3000", # undocumented
			0x0166: "MIPS (little endian)",
			0x0169: "MIPS Windows CE v2 (little endian)",
			0x0266: "MIPS16",
			0x0366: "MIPS with FPU",
			0x0466: "MIPS16 with FPU",
			0x6232: "LoongArch 32-bit",
			0x6264: "LoongArch 64-bit",
			0x0183: "DEC Alpha AXP", # undocumented
			0x01A2: "Hitachi SH3",
			0x01A3: "Hitachi SH3 DSP",
			0x01A6: "Hitachi SH4",
			0x01A8: "Hitachi SH5",
			0x01C0: "ARM (little endian)",
			0x01C2: "ARM Thumb",
			0x01C4: "ARM Thumb-2 (little endian)",
			0xAA64: "ARM (little endian)",
			0x01D4: "Matsushita AM33",
			0x01F0: "PowerPC (little endian)",
			0x01F1: "PowerPC with floating point",
			0x0200: "Intel Itanium",
			0x0268: "Motorola 68000 (Macintosh)", # undocumented
			0x0601: "PowerPC (Macintosh)", # undocumented
			0x0EBC: "EFI byte code",
			0x5032: "RISC-V 32-bit",
			0x5064: "RISC-V 64-bit",
			0x5128: "RISC-V 128-bit",
			0x9041: "Mitsubishi M32R (little endian)",
			# TODO: other values
		}.get(machine, ("undefined", 1))
		print(f"Machine type: {machine_name} (0x{machine:04X})")
		section_count = self.rd.ReadWord(2)
		print(f"Section count: {section_count}")
		timestamp = self.rd.ReadWord(4)
		if timestamp != 0:
			print(f"Time stamp: {timestamp}") # TODO: parse
		symtab_offset = self.rd.ReadWord(4)
		symtab_count = self.rd.ReadWord(4)
		if symtab_offset != 0 or symtab_count != 0:
			print("COFF symbol table:")
			print(f"- Offset: 0x{symtab_offset:08X}")
			print(f"- Count:  0x{symtab_count:08X}")
		opthdr_length = self.rd.ReadWord(2)
		print(f"Optional header:")
		print(f"- Offset: 0x{new_header_offset + 0x18:08X}")
		print(f"- Length: 0x{opthdr_length:02X}")
		flags = self.rd.ReadWord(2)
		print(f"File flags: 0x{flags:04X}", end = '')
		if flags & 0x0001:
			print(", stripped (non-relocatable)", end = '')
		if flags & 0x0002:
			print(", executable", end = '')
		if flags & 0x0004:
			print(", COFF line numbers removed", end = '') # obsolete
		if flags & 0x0008:
			print(", COFF symbols removed", end = '') # obsolete
		if flags & 0x0010:
			print(", aggressively trim working set", end = '') # obsolete
		if flags & 0x0020:
			print(", large address aware (> 2GiB addresses)", end = '')
		#if flags & 0x0040:
		if flags & 0x0100:
			print(", 32-bit", end = '')
		if flags & 0x0200:
			print(", no debug information", end = '')
		if flags & 0x0400:
			print(", run from swap if on removable media", end = '')
		if flags & 0x0800:
			print(", run from swap if on network media", end = '')
		if flags & 0x1000:
			print(", system file", end = '')
		if flags & 0x2000:
			print(", library", end = '')
		if flags & 0x4000:
			print(", uniprocessor only", end = '')
		if flags & 0x8000:
			print(", big endian", end = '') # deprecated
		print()

		opthdr_offset = self.rd.Tell()

		# Build up mapping for sections to calculate file offsets

		self.memory_map = {}
		for section_number in range(section_count):
			self.rd.Seek(new_header_offset + 24 + opthdr_length + section_number * 40)
			data = self.rd.Read(8)
			ix = data.find(b'\0')
			if ix != -1:
				data = data[:ix]
			section_name = data.decode(options.get('encoding', 'cp437_full')) # TODO: different behavior when it starts with '/'
			section_virtual_size = self.rd.ReadWord(4)
			section_address = self.rd.ReadWord(4)
			section_length = self.rd.ReadWord(4)
			section_offset = self.rd.ReadWord(4)
			self.memory_map[section_address] = section_offset - section_address, section_name
			section_end = section_address + min(section_length, section_virtual_size)
			if section_end not in self.memory_map:
				self.memory_map[section_end] = None, None

		self.rd.Seek(opthdr_offset)

		if opthdr_length >= 2:
			magic = self.rd.ReadWord(2)
			magic_name = {0x010B: "PE32 (32-bit)", 0x020B: "PE32+ (64-bit)"}.get(magic, "unknown")
			wordsize = {0x010B: 4, 0x020B: 8}.get(magic, 4)
			W=2*wordsize
			print(f"Magic: {magic_name} (0x{magic:04X})")
		if opthdr_length >= 4:
			linker_version = self.rd.ReadWord(1)
			linker_version = (linker_version, self.rd.ReadWord(1))
			print(f"Linker version: {'.'.join(map(str, linker_version))}")
		if opthdr_length >= 8:
			code_size = self.rd.ReadWord(4)
			print(f"Total size of code sections: 0x{code_size:08X}")
		if opthdr_length >= 12:
			data_size = self.rd.ReadWord(4)
			print(f"Total size of data sections: 0x{data_size:08X}")
		if opthdr_length >= 16:
			bss_size = self.rd.ReadWord(4)
			print(f"Total size of bss sections:  0x{bss_size:08X}")
		if opthdr_length >= 20:
			entry = self.rd.ReadWord(4)
		if opthdr_length >= 24:
			code_base = self.rd.ReadWord(4)
			print(f"Base address of code: 0x{code_base:08X}")
		if opthdr_length >= 28 and magic == 0x010B:
			data_base = self.rd.ReadWord(4)
			print(f"Base address of data: 0x{data_base:08X}")
		# [file] (RVA) {virtual}
		if opthdr_length >= 32:
			self.image_base = self.rd.ReadWord(wordsize)
			print(f"Image base: 0x{self.image_base:0{W}X}")
			#print(f"Entry point: 0x{entry:0{W}X}, (0x{self.image_base + entry:0{W}X} virtual)") # TODO: in file
			print(f"Entry point: {self.PrintRva(entry)}")
		if opthdr_length >= 36:
			section_align = self.rd.ReadWord(4)
			print(f"Section alignment in memory: 0x{section_align:08X}")
		if opthdr_length >= 40:
			file_align = self.rd.ReadWord(4)
			print(f"Section alignment in file:   0x{file_align:08X}")
		if opthdr_length >= 44:
			os_version = self.rd.ReadWord(2)
			os_version = (os_version, self.rd.ReadWord(2))
			print(f"OS version: {'.'.join(map(str, os_version))}")
		if opthdr_length >= 48:
			image_version = self.rd.ReadWord(2)
			image_version = (image_version, self.rd.ReadWord(2))
			print(f"Image version: {'.'.join(map(str, image_version))}")
		if opthdr_length >= 52:
			subsys_version = self.rd.ReadWord(2)
			subsys_version = (subsys_version, self.rd.ReadWord(2))
			print(f"Subsystem version: {'.'.join(map(str, subsys_version))}")
		if opthdr_length >= 56:
			win32_version = self.rd.ReadWord(4) # reserved
			if win32_version != 0:
				print(f"Win32 version: 0x{win32_version:08X}")
		if opthdr_length >= 60:
			image_size = self.rd.ReadWord(4)
			print(f"Size of total image with headers in memory: 0x{image_size:08X}")
		if opthdr_length >= 64:
			header_size = self.rd.ReadWord(4)
			print(f"Size of headers: 0x{header_size:08X}")
		if opthdr_length >= 68:
			checksum = self.rd.ReadWord(4)
			if checksum != 0:
				print(f"Checksum: 0x{checksum:08X}")
		if opthdr_length >= 70:
			subsystem = self.rd.ReadWord(2)
			subsystem_name = {
				0x0000: "unknown",
				0x0001: "native",
				0x0002: "Windows GUI",
				0x0003: "Windows text",
				0x0005: "OS/2 text",
				0x0007: "POSIX text",
				0x0008: "Native Win9x driver",
				0x0009: "Windows CE",
				0x000A: "EFI application",
				0x000B: "EFI driver with boot devices",
				0x000C: "EFI driver with run-time services",
				0x000D: "EFI ROM image",
				0x000E: "XBOX",
				0x0010: "Windows boot application",
			}.get(subsystem, "undefined")
			print(f"Subsystem: {subsystem_name} (0x{subsystem:04X})")
		if opthdr_length >= 72:
			dll_flags = self.rd.ReadWord(2)
			if dll_flags != 0:
				print(f"DLL flags: 0x{dll_flags:04X}", end = '')
				if dll_flags & 0x0001:
					print(", call on first load", end = '') # removed
				if dll_flags & 0x0002:
					print(", call on thread termination", end = '') # removed
				if dll_flags & 0x0004:
					print(", call on thread start up", end = '') # removed
				if dll_flags & 0x0008:
					print(", call on DLL exit", end = '') # removed
				#if dll_flags & 0x0010:
				if dll_flags & 0x0020:
					print(", high entropy 64-bit virtual address space support", end = '')
				if dll_flags & 0x0040:
					print(", relocatable", end = '')
				if dll_flags & 0x0080:
					print(", code integrity checks", end = '')
				if dll_flags & 0x0100:
					print(", NX compatible", end = '')
				if dll_flags & 0x0200:
					print(", isolation aware but non-isolated", end = '')
				if dll_flags & 0x0400:
					print(", no structured exception handling", end = '')
				if dll_flags & 0x0800:
					print(", not bindable", end = '')
				if dll_flags & 0x1000:
					print(", must run in AppContiainer", end = '')
				if dll_flags & 0x2000:
					print(", WDM driver", end = '')
				if dll_flags & 0x4000:
					print(", Control Flow Guard support", end = '')
				if dll_flags & 0x8000:
					print(", Terminal Server aware", end = '')
				print()
		if opthdr_length >= 72 + wordsize:
			stack_reserve = self.rd.ReadWord(wordsize)
			print(f"Size of stack to reserve: 0x{stack_reserve:0{W}X}")
		if opthdr_length >= 72 + wordsize * 2:
			stack_commit = self.rd.ReadWord(wordsize)
			print(f"Size of stack to commit:  0x{stack_commit:0{W}X}")
		if opthdr_length >= 72 + wordsize * 3:
			heap_reserve = self.rd.ReadWord(wordsize)
			print(f"Size of heap to reserve: 0x{heap_reserve:0{W}X}")
		if opthdr_length >= 72 + wordsize * 4:
			heap_commit = self.rd.ReadWord(wordsize)
			print(f"Size of heap to commit:  0x{heap_commit:0{W}X}")
		if opthdr_length >= 76 + wordsize * 4:
			loader_flags = self.rd.ReadWord(4) # reserved
			if loader_flags != 0:
				print(f"Loader flags: 0x{loader_flags:08X}")
		if opthdr_length >= 80 + wordsize * 4:
			directory_count = self.rd.ReadWord(4)
			print(f"Data directory count: 0x{directory_count:08X}")
		if directory_count >= 1 and opthdr_length >= 88 + wordsize * 4:
			edata_rva = self.rd.ReadWord(4)
			edata_len = self.rd.ReadWord(4)
			if edata_len != 0:
				print("Export table:")
				print(f"- Offset: {self.PrintRva(edata_rva)}")
				print(f"- Length: 0x{edata_len:08X}")
		else:
			edata_rva = edata_len = 0
		if directory_count >= 2 and opthdr_length >= 96 + wordsize * 4:
			idata_rva = self.rd.ReadWord(4)
			idata_len = self.rd.ReadWord(4)
			if idata_len != 0:
				print("Import table:")
				print(f"- Offset: {self.PrintRva(idata_rva)}")
				print(f"- Length: 0x{idata_len:08X}")
		if directory_count >= 3 and opthdr_length >= 104 + wordsize * 4:
			rsrc_rva = self.rd.ReadWord(4)
			rsrc_len = self.rd.ReadWord(4)
			if rsrc_len != 0:
				print("Resource table:")
				print(f"- Offset: {self.PrintRva(rsrc_rva)}")
				print(f"- Length: 0x{rsrc_len:08X}")
		if directory_count >= 4 and opthdr_length >= 112 + wordsize * 4:
			pdata_rva = self.rd.ReadWord(4)
			pdata_len = self.rd.ReadWord(4)
			if pdata_len != 0:
				print("Exception table:")
				print(f"- Offset: {self.PrintRva(pdata_rva)}")
				print(f"- Length: 0x{pdata_len:08X}")
		if directory_count >= 5 and opthdr_length >= 120 + wordsize * 4:
			certificate_rva = self.rd.ReadWord(4)
			certificate_len = self.rd.ReadWord(4)
			if certificate_len != 0:
				print("Certificate table:")
				print(f"- Offset: {self.PrintRva(certificate_rva)}")
				print(f"- Length: 0x{certificate_len:08X}")
		if directory_count >= 6 and opthdr_length >= 128 + wordsize * 4:
			reloc_rva = self.rd.ReadWord(4)
			reloc_len = self.rd.ReadWord(4)
			if reloc_len != 0:
				print("Base relocation table:")
				print(f"- Offset: {self.PrintRva(reloc_rva)}")
				print(f"- Length: 0x{reloc_len:08X}")
		else:
			reloc_rva = reloc_len = 0
		if directory_count >= 7 and opthdr_length >= 136 + wordsize * 4:
			debug_rva = self.rd.ReadWord(4)
			debug_len = self.rd.ReadWord(4)
			if debug_len != 0:
				print("Debug data:")
				print(f"- Offset: {self.PrintRva(debug_rva)}")
				print(f"- Length: 0x{debug_len:08X}")
		if directory_count >= 8 and opthdr_length >= 144 + wordsize * 4:
			arch_rva = self.rd.ReadWord(4) # reserved
			arch_len = self.rd.ReadWord(4) # reserved
			if arch_len != 0:
				print("Architecture:")
				print(f"- Offset: {self.PrintRva(arch_rva)}")
				print(f"- Length: 0x{arch_len:08X}")
		if directory_count >= 9 and opthdr_length >= 148 + wordsize * 4:
			global_ptr = self.rd.ReadWord(4)
			self.rd.Skip(4) # reserved
			if global_ptr != 0:
				print("Global pointer: {self.PrintRva(global_ptr)}")
		if directory_count >= 10 and opthdr_length >= 160 + wordsize * 4:
			tls_rva = self.rd.ReadWord(4)
			tls_len = self.rd.ReadWord(4)
			if tls_len != 0:
				print("Architecture:")
				print(f"- Offset: {self.PrintRva(tls_rva)}")
				print(f"- Length: 0x{tls_len:08X}")
		if directory_count >= 11 and opthdr_length >= 168 + wordsize * 4:
			loadconfig_rva = self.rd.ReadWord(4)
			loadconfig_len = self.rd.ReadWord(4)
			if loadconfig_len != 0:
				print("Load Config Table:")
				print(f"- Offset: {self.PrintRva(loadconfig_rva)}")
				print(f"- Length: 0x{loadconfig_len:08X}")
		if directory_count >= 12 and opthdr_length >= 176 + wordsize * 4:
			boundimport_rva = self.rd.ReadWord(4)
			boundimport_len = self.rd.ReadWord(4)
			if boundimport_len != 0:
				print("Bound Import Table:")
				print(f"- Offset: {self.PrintRva(boundimport_rva)}")
				print(f"- Length: 0x{boundimport_len:08X}")
		if directory_count >= 13 and opthdr_length >= 184 + wordsize * 4:
			iat_rva = self.rd.ReadWord(4)
			iat_len = self.rd.ReadWord(4)
			if iat_len != 0:
				print("Import address table:")
				print(f"- Offset: {self.PrintRva(iat_rva)}")
				print(f"- Length: 0x{iat_len:08X}")
		if directory_count >= 14 and opthdr_length >= 192 + wordsize * 4:
			delayimport_rva = self.rd.ReadWord(4)
			delayimport_len = self.rd.ReadWord(4)
			if delayimport_len != 0:
				print("Delay import descriptor:")
				print(f"- Offset: {self.PrintRva(delayimport_rva)}")
				print(f"- Length: 0x{delayimport_len:08X}")
		if directory_count >= 15 and opthdr_length >= 200 + wordsize * 4:
			clr_rva = self.rd.ReadWord(4)
			clr_len = self.rd.ReadWord(4)
			if clr_len != 0:
				print("CLR Runtime Header (.NET):")
				print(f"- Offset: {self.PrintRva(clr_rva)}")
				print(f"- Length: 0x{clr_len:08X}")

		self.rd.Seek(new_header_offset + 24 + opthdr_length)
		#memory_map = {}
		print("== Section table")
		section_offsets = []
		section_lengths = []
		section_bases = []
		section_isloaded = []
		for section_number in range(1, section_count + 1):
			data = self.rd.Read(8)
			ix = data.find(b'\0')
			if ix != -1:
				data = data[:ix]
			section_name = data.decode(options.get('encoding', 'cp437_full')) # TODO: different behavior when it starts with '/'
			section_virtual_size = self.rd.ReadWord(4)
			section_address = self.rd.ReadWord(4)
			section_length = self.rd.ReadWord(4)
			section_offset = self.rd.ReadWord(4)
			section_reloc_offset = self.rd.ReadWord(4)
			section_lineno_offset = self.rd.ReadWord(4)
			section_reloc_count = self.rd.ReadWord(2)
			section_lineno_count = self.rd.ReadWord(2)
			section_flags = self.rd.ReadWord(4)
			section_offsets.append(section_offset)
			section_lengths.append(min(section_virtual_size, section_length) if section_virtual_size != 0 else section_length)
			section_isloaded.append(section_virtual_size != 0)
			section_bases.append(section_address)
			#memory_map[section_address] = section_offset - section_address
			#section_end = section_address + min(section_length, section_virtual_size)
			#if section_end not in memory_map:
			#	memory_map[section_end] = None
			print(f"Section #{section_number}: {section_name}")
			print(f"- Address: 0x{section_address:08X} (0x{self.image_base + section_address:08X} virtual)")
			print(f"- Offset: 0x{section_offset:08X}")
			print(f"- Length: 0x{section_length:08X}")
			print(f"- Memory: 0x{section_virtual_size:08X}")
			print(f"- Flags: 0x{section_flags:08X}", end = '')
			if section_flags & 0x00000008:
				print(", no padding", end = '')
			if section_flags & 0x00000020:
				print(", code", end = '')
			if section_flags & 0x00000040:
				print(", data", end = '')
			if section_flags & 0x00000080:
				print(", bss", end = '')
			if section_flags & 0x00000100:
				print(", other", end = '') # undefined
			if section_flags & 0x00000200:
				print(", comment", end = '')
			if section_flags & 0x00000800:
				print(", removed", end = '')
			if section_flags & 0x00001000:
				print(", COMDAT", end = '')
			if section_flags & 0x00008000:
				print(", global pointer data", end = '')
			if section_flags & 0x00010000:
				print(", purgeable", end = '') # undefined
			if section_flags & 0x00020000:
				print(", 16-bit", end = '') # undefined
			if section_flags & 0x00040000:
				print(", locked", end = '') # undefined
			if section_flags & 0x00080000:
				print(", preload", end = '') # undefined
			if section_flags & 0x00F00000:
				print(f", {1 << (((section_flags >> 5) & 0xF) - 1)}-byte aligned", end = '')
			if section_flags & 0x01000000:
				print(", extended relocations", end = '')
			if section_flags & 0x02000000:
				print(", discardable", end = '')
			if section_flags & 0x04000000:
				print(", non-cached", end = '')
			if section_flags & 0x08000000:
				print(", non-pageable", end = '')
			if section_flags & 0x10000000:
				print(", shared", end = '')
			if section_flags & 0x20000000:
				print(", executable", end = '')
			if section_flags & 0x40000000:
				print(", readable", end = '')
			if section_flags & 0x80000000:
				print(", writable", end = '')
			print()
			if section_reloc_count != 0:
				print("COFF relocations:")
				print(f"- Offset: 0x{section_reloc_offset:08X}")
				print(f"- Count: 0x{section_reloc_count:04X}")
			if section_lineno_count != 0:
				print("COFF line numbers:")
				print(f"- Offset: 0x{section_lineno_offset:08X}")
				print(f"- Count: 0x{section_lineno_count:04X}")
		#print(memory_map)

		relocs = {}

		# TODO: COFF relocations

		# TODO: COFF line numbers (deprecated)

		# TODO: COFF symbol table

		# TODO: COFF string table

		# TODO: other tables

		# Export directory table
		if edata_len != 0:
			try:
				print("== Export directory table")
				edata_start = self.GetOffset(edata_rva)[0]
				if edata_start is None:
					raise Exception("Export directory table RVA falls outside section data")
				self.rd.Seek(edata_start)
				self.rd.Skip(4) # flags
				timestamp = self.rd.ReadWord(4)
				print(f"Time stamp: 0x{timestamp:08X}") # TODO: format
				version = self.rd.ReadWord(2)
				version = (version, self.rd.ReadWord(2))
				print(f"Version: {'.'.join(map(str, version))}")
				name_rva = self.rd.ReadWord(4)
				name = self.FetchName(name_rva)
				print(f"Name: {name!r} (0x{name_rva:08X})")
				ordinal_base = self.rd.ReadWord(4)
				print(f"Ordinal base: 0x{ordinal_base:08X}")
				address_count = self.rd.ReadWord(4)
				name_pointer_count = self.rd.ReadWord(4)
				address_table_rva = self.rd.ReadWord(4)
				name_pointer_table_rva = self.rd.ReadWord(4)
				ordinal_table_rva = self.rd.ReadWord(4)
				print("Export address table:")
				print(f"- Offset: {self.PrintRva(address_table_rva)}")
				print(f"- Count: 0x{address_count:08X}")
				print("Export name table:")
				print(f"- Offset: {self.PrintRva(name_pointer_table_rva)}")
				print(f"- Count: 0x{name_pointer_count:08X}")
				print("Ordinal table:")
				print(f"- Offset: {self.PrintRva(ordinal_table_rva)}")
				address_table_offset = self.GetOffset(address_table_rva)[0]
				if address_table_offset is None:
					raise Exception("Address table RVA falls outside section data")
				self.rd.Seek(address_table_offset)
				print("= Entries:")
				for address_number in range(address_count):
					offset = self.rd.Tell()
					rva = self.rd.ReadWord(4)
					if edata_rva <= rva < edata_rva + edata_len:
						print(f"[0x{offset:08X}] Entry #{ordinal_base + address_number}: {self.FetchName(rva)} -- name string at {self.PrintRva(rva)}")
					else:
						print(f"[0x{offset:08X}] Entry #{ordinal_base + address_number}: {self.PrintRva(rva)}")
				name_pointer_table_offset = self.GetOffset(name_pointer_table_rva)[0]
				ordinal_table_offset = self.GetOffset(ordinal_table_rva)[0]
				if name_pointer_table_offset is None:
					raise Exception("Name pointer table RVA falls outside section data")
				if ordinal_table_offset is None:
					raise Exception("Ordinal table RVA falls outside section data")
				print("= Exported names:")
				for name_number in range(name_pointer_count):
					self.rd.Seek(name_pointer_table_offset + name_number * 4)
					rva = self.rd.ReadWord(4)
					name = self.FetchName(rva)
					self.rd.Seek(ordinal_table_offset + name_number * 2)
					ordinal = self.rd.ReadWord(2)
					print(f"Ordinal 0x{ordinal_base + ordinal} name {name} -- name string at {self.PrintRva(rva)}")
			except Exception as e:
				print(e, file = sys.stderr)

		# Import directory table
		if idata_len != 0:
			try:
				print("== Import directory table")
				idata_start = self.GetOffset(idata_rva)[0]
				if idata_start is None:
					raise Exception("Import directory table RVA falls outside section data")
				dll_number = 0
				while True:
					self.rd.Seek(idata_start + dll_number * 20)
					import_lookup_table = self.rd.ReadWord(4)
					if import_lookup_table == 0:
						break
					print(f"- Imported DLL #{dll_number + 1} at 0x{idata_start + dll_number * 20:08X}")
					timestamp = self.rd.ReadWord(4)
					forwarder_chain = self.rd.ReadWord(4) # TODO
					name_rva = self.rd.ReadWord(4)
					print(f"Name: {self.FetchName(name_rva)} -- name string at {self.PrintRva(name_rva)}")
					print(f"Time stamp: 0x{timestamp:08X}") # TODO: format
					import_address_table = self.rd.ReadWord(4)
					print(f"Import lookup table address: {self.PrintRva(import_lookup_table)}")
					print(f"Import address table (thunk table) address: {self.PrintRva(import_address_table)}")

					dll_number += 1

					import_lookup_table_offset = self.GetOffset(import_lookup_table)[0]
					if import_lookup_table_offset is None:
						print("Invalid import lookup table RVA", file = sys.stderr)
						continue

					print("- Import lookup table")

					entry_count = 0
					while True:
						self.rd.Seek(import_lookup_table_offset + entry_count * 4)
						entry = self.rd.ReadWord(4)
						if entry == 0:
							break
						if entry & (1 << ((wordsize << 3) - 1)):
							print(f"[0x{import_lookup_table_offset + entry_count * 4:08X}] Ordinal 0x{entry & 0xFFFF:04X}")
						else:
							hint, name = self.FetchHintName(entry)
							print(f"[0x{import_lookup_table_offset + entry_count * 4:08X}] Name {name!r} hint 0x{hint:04X} -- hint/name at {self.PrintRva(entry)}")
						entry_count += 1

					import_address_table_offset = self.GetOffset(import_address_table)[0]
					if import_address_table_offset is None:
						print("Invalid import address table RVA", file = sys.stderr)
						continue

					print("- Import address table")

					entry_count = 0
					while True:
						self.rd.Seek(import_address_table_offset + entry_count * wordsize)
						entry = self.rd.ReadWord(wordsize)
						if entry == 0:
							break
						if entry & (1 << ((wordsize << 3) - 1)):
							print(f"[0x{import_address_table_offset + entry_count * 4:08X}] Ordinal 0x{entry & 0xFFFF:04X}")
						else:
							hint, name = self.FetchHintName(entry)
							print(f"[0x{import_address_table_offset + entry_count * 4:08X}] Name {name!r} hint 0x{hint:04X} -- hint/name at {self.PrintRva(entry)}")
						relocs[import_address_table + entry_count * wordsize] = wordsize
						entry_count += 1

					# TODO: finish
			except Exception as e:
				print(e, file = sys.stderr)

		# TODO: resources

		# Base relocations
		if options.get('relshow', False) or options.get('showall', False):
			if reloc_len != 0:
				print("== Base relocations")
				reloc_start = self.GetOffset(reloc_rva)[0]
				self.rd.Seek(reloc_start)
				while self.rd.Tell() < reloc_start + reloc_len:
					block_start = self.rd.Tell()
					page_rva = self.rd.ReadWord(4)
					block_len = self.rd.ReadWord(4)
					print(f"- Page {self.PrintRva(page_rva)}, length: 0x{block_len:08X}")
					while self.rd.Tell() < block_start + block_len:
						file_offset = self.rd.Tell()
						data = self.rd.ReadWord(2)
						rel_offset = data & 0x0FFF
						rel_type = data >> 12
						rel_type_name = {
							0: "unused",
							1: "high 16-bit",
							2: "low 16-bit",
							3: "32-bit",
							# TODO: other types, machine dependent
						}.get(rel_type, "unknown")
						rel_type_size = {
							1: 2,
							2: 2,
							3: 4,
							# TODO: other types, machine dependent
						}.get(rel_type, 0)
						print(f"[0x{file_offset:08X}] Offset 0x{rel_offset:04X}, type {rel_type_name} (0x{data:03X})")
						relocs[page_rva + rel_offset] = rel_type_size

		# Section data
		if options.get('data', False) or options.get('showall', False):
			print("== Section data")
			for section_number in range(section_count):
				self.rd.Seek(section_offsets[section_number])
				print(f"Section #0x{section_number + 1:X} data")
				print("[FILE    ] (SECTION ) MEMORY  \tDATA")
				file_offset = self.rd.Tell()
				for row, hexcodes, text in self.process_data(options, 4, section_lengths[section_number],
						get_reloc_size = lambda position: relocs.get(section_bases[section_number] + position), text_encoding = 'cp437_full'):
					if section_isloaded[section_number]:
						print(f"[{file_offset + row:08X}] ({row:08X}) {self.image_base + section_bases[section_number] + row:08X}\t{hexcodes}\t{text}")
					else:
						print(f"[{file_offset + row:08X}] ({row:08X})         \t{hexcodes}\t{text}")


		# TODO: finish

class AOutReader(FileReader):
	def __init__(self, sys, rd):
		sys = {'djgpp': 'djgpp', 'pdos32': 'pdos32'}.get(sys, sys) # TODO: others
		assert sys in {'djgpp', 'pdos32'}
		super(AOutReader, self).__init__(rd)
		self.sys = sys
		self.rd.endian = Reader.LittleEndian

	def ReadFile(self, **options):
		print("==== 32-bit a.out format ====")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(0)
		magic = self.rd.Read(2)
		# TODO: only little endian a.out is supported
		if magic in {b'\x07\x01', b'\x08\x01', b'\x0B\x01', b'\xCC\x00'}:
			new_header_offset = 0
		else:
			if magic != b'MZ':
				print(f"Error: invalid stub magic {magic}", file = sys.stderr)
			new_header_offset = self.rd.ReadWord(2)
			new_header_offset = (self.rd.ReadWord(2) << 9) - (-new_header_offset & 0x1FF)
			self.rd.Seek(new_header_offset)
			magic = self.rd.Read(2)
			if magic not in {b'\x07\x01', b'\x08\x01', b'\x0B\x01', b'\xCC\x00'}:
				print(f"Error: invalid magic {magic} at 0x{self.rd.Tell():X}", file = sys.stderr)
		if new_header_offset != 0:
			print(f"- Image offset: 0x{new_header_offset:08X}")
		magic_name = {
			b'\x07\x01': "OMAGIC (impure)",
			b'\x08\x01': "NMAGIC (pure)",
			b'\x0B\x01': "ZMAGIC (demand-paged)",
			b'\xCC\x00': "QMAGIC (demand-paged)"
		}.get(magic)
		print(f"Magic number: magic_name (0x{self.rd.ParseWord(magic):04X})")
		cpu = self.rd.ReadWord(1)
		cpu_name = {
			0x64: "Intel 80386",
		}.get(cpu, "unknown")
		print(f"CPU: {cpu_name} (0x{cpu:02X})")
		self.rd.Skip(1)
		textsize = self.rd.ReadWord(4)
		datasize = self.rd.ReadWord(4)
		bsssize = self.rd.ReadWord(4)
		symtabsize = self.rd.ReadWord(4)
		entry = self.rd.ReadWord(4)
		trsize = self.rd.ReadWord(4)
		drsize = self.rd.ReadWord(4)

		print(f"Entry: 0x{entry:08X}")
		print("Text")
		print(f"- Offset: 0x{new_header_offset + 32:08X}")
		print(f"- Length: 0x{textsize:08X}")
		print("Data")
		print(f"- Offset: 0x{new_header_offset + 32 + textsize:08X}")
		print(f"- Length: 0x{datasize:08X}")
		print("Bss")
		print(f"- Length: 0x{bsssize:08X}")
		if symtabsize != 0:
			print("Symbol table")
			print(f"- Offset: 0x{new_header_offset + 32 + textsize + datasize:08X}")

		# TODO: relocations

		if options.get('data', False) or options.get('showall', False):
			print("= Text segment")
			print("[FILE    ] (SEGMENT ) MEMORY  \tDATA")
			if self.sys == 'djgpp' and magic == b'\x0B\x01':
				textbase = 0
				textoffset = new_header_offset
				textsize += 32
				self.rd.Seek(textoffset)
				if entry >= 0x1000:
					# zero page
					textbase += 0x1000
			else:
				textbase = 0
				textoffset = self.rd.Tell()
			for row, hexcodes, text in self.process_data(options, 4, textsize, text_encoding = 'ascii_graphic'):
				print(f"[{textoffset + row:08X}] ({row:08X}) {textbase + row:08X}\t{hexcodes}\t{text}")

			print("= Data segment")
			print("[FILE    ] (SEGMENT ) MEMORY  \tDATA")
			if self.sys == 'djgpp' and magic == b'\x0B\x01':
				database = (textbase + textsize + 0x3FFFFF) & ~0x3FFFFF
				dataoffset = ((textoffset - new_header_offset + textsize + 0xFFF) & ~0xFFF) + new_header_offset
				self.rd.Seek(dataoffset)
			else:
				database = textbase + textsize
				dataoffset = textoffset + textsize
			for row, hexcodes, text in self.process_data(options, 4, datasize, text_encoding = 'ascii_graphic'):
				print(f"[{dataoffset + row:08X}] ({row:08X}) {database + row:08X}\t{hexcodes}\t{text}")

class MINIXAOutReader(FileReader):
	def __init__(self, rd):
		super(MINIXAOutReader, self).__init__(rd)
		self.rd.endian = Reader.LittleEndian

	def ReadFile(self, **options):
		print("==== MINIX a.out format ====")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(2)
		flags = self.rd.ReadWord(1)
		cpu = self.rd.ReadWord(1)
		if cpu & 3 == 0:
			self.rd.endian = Reader.LittleEndian
		elif cpu & 3 == 1:
			self.rd.endian = Reader.AntiPDP11Endian
		elif cpu & 3 == 2:
			self.rd.endian = Reader.PDP11Endian
		elif cpu & 3 == 3:
			self.rd.endian = Reader.BigEndian
		cpu_name = {
			0x04: "Intel 8086",
			0x0B: "Motorola 68000",
			0x0C: "NS32000 (16032)",
			0x10: "Intel 80386",
			0x17: "SPARC"
		}.get(cpu, "unknown")

		print(f"CPU: {cpu_name} (0x{cpu:02X})")
		print(f"Flags: 0x{flags:02X}", end = '')
		if flags & 0x01:
			print(", unmapped zero page", end = '')
		if flags & 0x02:
			print(", page aligned executable", end = '')
		if flags & 0x04:
			print(", new symbol table", end = '')
		if flags & 0x10:
			print(", executable", end = '')
		if flags & 0x20:
			print(", separate code/data", end = '')
		if flags & 0x40:
			print(", pure text", end = '')
		if flags & 0x80:
			print(", text overlay", end = '')
		print()

		header_size = self.rd.ReadWord(1)
		self.rd.Skip(1)

		if header_size >= 8:
			version = self.rd.ReadWord(1)
			version = (version, self.rd.ReadWord(1)) # TODO: unknown order
			print(f"Header length: 0x{header_size:02X}")
		else:
			version = None

		if version is not None and version != (0, 0):
			print(f"Version: {'.'.join(map(str, version))} (TODO: order)")

		if header_size >= 12:
			code_size = self.rd.ReadWord(4)
		else:
			code_size = 0

		if header_size >= 16:
			data_size = self.rd.ReadWord(4)
		else:
			data_size = 0

		if header_size >= 20:
			bss_size = self.rd.ReadWord(4)
		else:
			bss_size = 0

		if header_size >= 24:
			entry = self.rd.ReadWord(4)
		else:
			entry = 0

		if header_size >= 28:
			total_size = self.rd.ReadWord(4)
		else:
			total_size = 0

		if header_size >= 32:
			symtab_size = self.rd.ReadWord(4)
		else:
			symtab_size = 0

		if header_size >= 36:
			code_relsize = self.rd.ReadWord(4)
		else:
			code_relsize = 0

		if header_size >= 40:
			data_relsize = self.rd.ReadWord(4)
		else:
			data_relsize = 0

		if header_size >= 44:
			code_base = self.rd.ReadWord(4)
		else:
			code_base = 0

		if header_size >= 48:
			data_base = self.rd.ReadWord(4)
		elif flags & 0x20:
			data_base = 0
		else:
			data_base = code_base + code_size

		print("Code segment:")
		print(f"- Offset: 0x{header_size:08X}")
		print(f"- Length: 0x{code_size:08X}")
		print(f"- Address: 0x{code_base:08X}")
		if code_relsize != 0:
			print(f"- Relocations length: 0x{code_relsize:08X}")

		print("Data segment:")
		print(f"- Offset: 0x{header_size + code_size:08X}")
		print(f"- Length: 0x{data_size:08X}")
		print(f"- Address: 0x{data_base:08X}")
		if data_relsize != 0:
			print(f"- Relocations length: 0x{data_relsize:08X}")

		print("Bss segment:")
		print(f"- Length: 0x{bss_size:08X}")
		print(f"- Address: 0x{data_base + data_size:08X}")

		print("Symbol table:")
		print(f"- Offset: 0x{header_size + code_size + data_size:08X}")
		print(f"- Length: 0x{symtab_size:08X}")

		print(f"Entry: 0x{entry:08X}")

		print(f"Total memory: 0x{total_size:08X}")

		if options.get('data', False) or options.get('showall', False):
			print("== Code data")
			print("[FILE    ] (SEGMENT ) MEMORY  \tDATA")
			section_base = (code_base // 16) * 16
			section_offset = header_size - (code_base % 16)
			for row, hexcodes, text in self.process_data(options, 4, code_size,
					data_offset = code_base % 16, text_encoding = 'cp437_full'):
				print(f"[{section_offset + row:08X}] ({row:08X}) {section_base + row:08X}\t{hexcodes}\t{text}")

			print("== Data data")
			print("[FILE    ] (SEGMENT ) MEMORY  \tDATA")
			section_base = (data_base // 16) * 16
			section_offset = header_size + code_size - (data_base % 16)
			for row, hexcodes, text in self.process_data(options, 4, data_size,
					data_offset = data_base % 16, text_encoding = 'cp437_full'):
				print(f"[{section_offset + row:08X}] ({row:08X}) {section_base + row:08X}\t{hexcodes}\t{text}")

class COFFReader(FileReader):
	def __init__(self, rd):
		super(COFFReader, self).__init__(rd)
		self.rd.endian = Reader.LittleEndian

	def ReadFile(self, **options):
		print("==== COFF format ====")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(0)
		magic = self.rd.Read(2)
		if magic in {b'\x4C\x01', b'\x01\x50'}:
			new_header_offset = 0
		else:
			if magic != b'MZ':
				print(f"Error: invalid stub magic {magic}", file = sys.stderr)
			new_header_offset = self.rd.ReadWord(2)
			new_header_offset = (self.rd.ReadWord(2) << 9) - (-new_header_offset & 0x1FF)
			self.rd.Seek(new_header_offset)
			magic = self.rd.Read(2)
			if magic not in {b'\x4C\x01', b'\x01\x50'}:
				print(f"Error: invalid magic {magic} at 0x{self.rd.Tell():X}", file = sys.stderr)
		if new_header_offset != 0:
			print(f"- Image offset: 0x{new_header_offset:08X}")
		cpu_name, endian = {
			b'\x4C\x01': ("Intel 80386", Reader.LittleEndian),
			b'\x01\x50': ("Motorola 68000", Reader.BigEndian),
		}.get(magic, ('unknown', None))
		print(f"CPU: {cpu_name} (0x{magic[0]:02X} 0x{magic[1]:02X})")

		if endian is None:
			return

		if magic == b'\x4C\x01':
			# DJGPP, use cp437
			default_encoding = 'cp437_full'
		else:
			default_encoding = 'ascii_graphic'

		self.rd.endian = endian
		section_count = self.rd.ReadWord(2)
		print(f"Section count: {section_count}")
		timestamp = self.rd.ReadWord(4)
		if timestamp != 0:
			print(f"Time stamp: {timestamp}") # TODO: parse
		symtab_offset = self.rd.ReadWord(4)
		symtab_count = self.rd.ReadWord(4)
		if symtab_offset != 0 or symtab_count != 0:
			print("COFF symbol table:")
			print(f"- Offset: 0x{symtab_offset:08X}")
			print(f"- Count:  0x{symtab_count:08X}")
		opthdr_length = self.rd.ReadWord(2)
		print(f"Optional header:")
		print(f"- Offset: 0x{new_header_offset + 0x18:08X}")
		print(f"- Length: 0x{opthdr_length:02X}")
		flags = self.rd.ReadWord(2)
		print(f"File flags: 0x{flags:04X}", end = '')
		if flags & 0x0001:
			print(", stripped (non-relocatable)", end = '')
		if flags & 0x0002:
			print(", executable", end = '')
		if flags & 0x0004:
			print(", COFF line numbers removed", end = '') # obsolete
		if flags & 0x0008:
			print(", COFF symbols removed", end = '') # obsolete
		if flags & 0x0100:
			print(", 32-bit little endian", end = '')
		if flags & 0x0200:
			print(", 32-bit big endian", end = '')
		print()

		if opthdr_length >= 2:
			magic = self.rd.ReadWord(2)
			magic_name = {0x010B: "ZMAGIC"}.get(magic, "unknown")
			if magic != 0:
				print(f"Magic: {magic_name} (0x{magic:04X})")
		if opthdr_length >= 4:
			linker_version = self.rd.ReadWord(1)
			linker_version = (linker_version, self.rd.ReadWord(1))
			if linker_version != (0, 0):
				print(f"Linker version: {'.'.join(map(str, linker_version))}")
		if opthdr_length >= 8:
			code_size = self.rd.ReadWord(4)
			print(f"Total size of code sections: 0x{code_size:08X}")
		if opthdr_length >= 12:
			data_size = self.rd.ReadWord(4)
			print(f"Total size of data sections: 0x{data_size:08X}")
		if opthdr_length >= 16:
			bss_size = self.rd.ReadWord(4)
			print(f"Total size of bss sections:  0x{bss_size:08X}")
		if opthdr_length >= 20:
			entry = self.rd.ReadWord(4)
			print(f"Entry: 0x{entry:08X}")
		if opthdr_length >= 24:
			code_base = self.rd.ReadWord(4)
			print(f"Base address of code: 0x{code_base:08X}")
		if opthdr_length >= 28:
			data_base = self.rd.ReadWord(4)
			print(f"Base address of data: 0x{data_base:08X}")
		if opthdr_length >= 32:
			# Concurrent DOS 68K
			reloc_offset = self.rd.ReadWord(4)
			print(f"Relocation offset: 0x{reloc_offset:08X}")
		else:
			reloc_offset = None
		if opthdr_length >= 36:
			# Concurrent DOS 68K
			stack_size = self.rd.ReadWord(4)
			print(f"Stack size: 0x{stack_size:08X}")

		print("= Section table")
		self.rd.Seek(new_header_offset + 0x14 + opthdr_length)
		section_offsets = []
		section_lengths = []
		section_bases = []
		for section_number in range(1, section_count + 1):
			data = self.rd.Read(8)
			ix = data.find(b'\0')
			if ix != -1:
				data = data[:ix]
			section_name = data.decode(options.get('encoding', default_encoding))
			section_p_address = self.rd.ReadWord(4)
			section_address = self.rd.ReadWord(4)
			section_length = self.rd.ReadWord(4)
			section_offset = self.rd.ReadWord(4)
			section_reloc_offset = self.rd.ReadWord(4)
			section_lineno_offset = self.rd.ReadWord(4)
			section_reloc_count = self.rd.ReadWord(2)
			section_lineno_count = self.rd.ReadWord(2)
			section_flags = self.rd.ReadWord(4)
			print(f"Section #{section_number}: {section_name}")
			print(f"- Address: 0x{section_address:08X}")
			if section_p_address != 0 and section_p_address != section_address:
				print(f"- Physical: 0x{section_address:08X}")
			if new_header_offset == 0:
				print(f"- Offset: 0x{section_offset:08X}")
			else:
				print(f"- Offset: 0x{section_offset:08X} (0x{new_header_offset + section_offset:08X} in file)")
			print(f"- Length: 0x{section_length:08X}")
			print(f"- Flags: 0x{section_flags:08X}", end = '')
			if section_flags & 0x00000020:
				print(", code", end = '')
				include = True
			if section_flags & 0x00000040:
				print(", data", end = '')
				include = True
			if section_flags & 0x00000080:
				print(", bss", end = '')
				include = False
			print()
			if include:
				section_offsets.append(section_offset)
			else:
				section_offsets.append(None)
			section_lengths.append(section_length)
			section_bases.append(section_address)
			if section_reloc_count != 0:
				print("COFF relocations:")
				print(f"- Offset: 0x{section_reloc_offset:08X}")
				print(f"- Count: 0x{section_reloc_count:04X}")
			if section_lineno_count != 0:
				print("COFF line numbers:")
				print(f"- Offset: 0x{section_lineno_offset:08X}")
				print(f"- Count: 0x{section_lineno_count:04X}")

		relocs = {}
		if options.get('rel', False) or options.get('showall', False):
			if reloc_offset is not None:
				print("= Concurrent DOS 68K relocations")
				self.rd.Seek(new_header_offset + reloc_offset)
				offset = 0
				while True:
					reloc_offset = self.rd.Tell()
					displacement = self.rd.ReadWord(1)
					if displacement == 0:
						break

					if displacement & 0x80:
						size = 2
						sizename = 'word'
						displacement |= 0x7F
					else:
						size = 4
						sizename = 'long word'

					if displacement == 0x7F:
						displacement = self.rd.ReadWord(4)
					elif displacement == 0x7E:
						displacement = self.rd.ReadWord(2)
					elif displacement == 0x7D:
						displacement = self.rd.ReadWord(1)

					offset += displacement
					print(f"- [0x{reloc_offset:08X}] Relocation at 0x{offset:08X} to {sizename}")
					relocs[offset] = size

		if options.get('data', False) or options.get('showall', False):
			print("== Section data")
			for section_number in range(section_count):
				if section_offsets[section_number] is None:
					continue
				self.rd.Seek(new_header_offset + section_offsets[section_number])
				print(f"Section #0x{section_number + 1:X} data")
				file_offset = self.rd.Tell()
				section_base = (section_bases[section_number] // 16) * 16
				section_offset = new_header_offset + section_offsets[section_number] - (section_bases[section_number] % 16)
				print("[FILE    ] (SECTION ) MEMORY  \tDATA")
				for row, hexcodes, text in self.process_data(options, 4, section_lengths[section_number],
						data_offset = section_bases[section_number] % 16,
						get_reloc_size = lambda position: relocs.get(section_base + position), text_encoding = default_encoding):
					print(f"[{section_offset + row:08X}] ({row:08X}) {section_base + row:08X}\t{hexcodes}\t{text}")

class MachOReader(FileReader):
	# TODO: implement format
	def __init__(self, rd):
		super(MachOReader, self).__init__(rd)
		self.rd.endian = Reader.LittleEndian

	def ReadFile(self, **options):
		print("==== Mach-O format ====")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(0)

class ELFReader(FileReader):
	# TODO: implement format
	def __init__(self, rd):
		super(ELFReader, self).__init__(rd)
		self.rd.endian = Reader.LittleEndian

	def ReadFile(self, **options):
		print("==== ELF format ====")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(0)

		if new_header_offset != 0:
			print(f"- Image offset: 0x{new_header_offset:08X}")

class CPM68KReader(FileReader):
	def __init__(self, sys, rd):
		sys = {'cpm': 'cpm68k', 'tos': 'gemdos', 'gem': 'gemdos', 'h68k': 'human68k', 'cdos': 'cdos68k'}.get(sys, sys)
		assert sys in {'cpm68k', 'gemdos', 'human68k', 'cdos68k'}
		super(CPM68KReader, self).__init__(rd)
		self.sys = sys
		self.rd.endian = Reader.BigEndian

	def ReadFile(self, **options):
		print("==== CP/M-68K format ====")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(0)
		magic = self.rd.ReadWord(2)
		if magic not in {0x601A, 0x601B, 0x601C} \
		or magic == 0x601B and self.sys != 'cpm68k' \
		or magic == 0x601C and self.sys != 'cdos68k':
			print(f"Error: invalid magic {magic} at 0x{self.rd.Tell():X}", file = sys.stderr)

		default_encoding = 'st_full' if self.sys == 'gemdos' else 'ascii_graphic'

		textsize = self.rd.ReadWord(4)
		datasize = self.rd.ReadWord(4)
		bsssize = self.rd.ReadWord(4)
		symtabsize = self.rd.ReadWord(4)
		stacksize = self.rd.ReadWord(4)
		textbase = self.rd.ReadWord(4)
		noreloc = self.rd.ReadWord(2)
		if self.sys == 'human68k' and noreloc != 0xFFFF:
			print(f"Error: Expected 0xFFFF at offset 0x1A, received 0x{noreloc:04X}", file = sys.stderr)
			noreloc = 0xFFFF
		if self.sys == 'gemdos':
			prgflags = textbase
			textbase = 0
			print(f"Program flags: 0x{prgflags:08X}", end = '')
			# TODO: flags
			print()
		if magic == 0x601B:
			database = self.rd.ReadWord(4)
			bssbase = self.rd.ReadWord(4)
			textoffset = 0x24
		else:
			database = textbase + textsize
			bssbase = database + datasize
			textoffset = 0x1C
		dataoffset = textoffset + textsize
		symtaboffset = dataoffset + datasize
		fixupoffset = symtaboffset + symtabsize
		print(f"Text segment:")
		print(f"- Offset: 0x{textoffset:08X}")
		print(f"- Length: 0x{textsize:08X}")
		if self.sys != 'gemdos':
			print(f"- Address: 0x{textbase:08X}")
		print(f"Data segment:")
		print(f"- Offset: 0x{dataoffset:08X}")
		print(f"- Length: 0x{datasize:08X}")
		if self.sys != 'gemdos':
			print(f"- Address: 0x{database:08X}")
		print(f"Bss segment:")
		print(f"- Length: 0x{bsssize:08X}")
		if self.sys != 'gemdos':
			print(f"- Address: 0x{bssbase:08X}")
		if self.sys != 'human68k':
			print(f"Symbol table:")
			print(f"- Offset: 0x{symtaboffset:08X}")
			print(f"- Length: 0x{symtabsize:08X}")
		elif symtabsize != 0:
			print(f"Reserved field - Symbol table size: 0x{symtabsize:08X}")
		if noreloc == 0:
			print(f"Fixup information:")
			print(f"- Offset: 0x{fixupoffset:08X}")
			#print(f"- Length: 0x{fixupsize:08X}")

		relocs = {}
		if options.get('rel', False) or options.get('showall', False):
			if noreloc == 0:
				print("== Fixups ==")
				self.rd.Seek(fixupoffset)
				if self.sys == 'gemdos':
					offset = self.rd.ReadWord(4)
					relocs[offset] = 4
					print(f"Fixup longword at 0x{offset:08X}")
					while True:
						delta = self.rd.ReadWord(1)
						if delta == 0:
							break
						elif delta == 1:
							offset += 254
						else:
							offset += delta
							relocs[offset] = 4
							print(f"Fixup longword at 0x{offset:08X}")
				elif magic != 0x601C:
					size = 2
					sizename = "word"
					for offset in range(0, textsize + datasize, 2):
						word = self.rd.ReadWord(2)
						name = None
						if (word & 7) == 1:
							name = 'data'
						elif (word & 7) == 2:
							name = 'text'
						elif (word & 7) == 3:
							name = 'bss'
						elif (word & 7) == 4:
							print(f"- 0x{offset + 2 - size:08X}: {sizename} to undefined symbol")
						elif (word & 7) == 5:
							size = 4
							sizename = "long word"
							continue
						# 0: absolute, 6: PC-relative, 7: instruction
						if name is not None:
							print(f"- 0x{offset + 2 - size:08X}: {sizename} to {name}")
							relocs[offset + 2 - size] = size
						size = 2
						sizename = "word"
				else:
					pass # TODO

		if options.get('data', False) or options.get('showall', False):
			print(f"== Text segment ==")
			print("[FILE    ] (SEGMENT ) MEMORY  \tDATA")
			self.rd.Seek(textoffset)
			for row, hexcodes, text in self.process_data(options, 4, textsize,
					get_reloc_size = lambda position: relocs.get(position), text_encoding = options.get('encoding', default_encoding)):
				print(f"[{textoffset + row:08X}] ({row:08X}) {textbase + row:08X}\t{hexcodes}\t{text}")

			print(f"== Data segment ==")
			print("[FILE    ] (SEGMENT ) MEMORY  \tDATA")
			current_offset = textsize % 16
			segment_offset = dataoffset - current_offset
			segment_base = database - current_offset
			for row, hexcodes, text in self.process_data(options, 4, datasize, data_offset = current_offset,
					get_reloc_size = lambda position: relocs.get(textsize + position), text_encoding = options.get('encoding', default_encoding)):
				print(f"[{segment_offset + row:08X}] ({row:08X}) {segment_base + row:08X}\t{hexcodes}\t{text}")

class CPM8000Reader(FileReader):
	# TODO: implement format
	def __init__(self):
		super(CPM8000Reader, self).__init__(rd)
		self.rd.endian = Reader.BigEndian

	def ReadFile(self, **options):
		print("==== CP/M-8000 format ====")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(0)
		magic = self.rd.ReadWord(2)

class HUReader(FileReader):
	def __init__(self, rd):
		super(HUReader, self).__init__(rd)
		self.rd.endian = Reader.BigEndian

	def ReadFile(self, **options):
		print("==== Human68k HU .X format ====")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(0)
		magic = self.rd.Read(2)
		if magic != b'HU':
			print(f"Error: invalid magic {magic} at 0x{self.rd.Tell():X}", file = sys.stderr)

		self.rd.Skip(1)
		loadmode = self.rd.ReadWord(1)
		loadmode_text = {
			0: 'normal',
			1: 'smallest',
			2: 'high'
		}.get(loadmode, 'unknown')
		print(f"Load mode: {loadmode_text} (0x{loadmode:02X})")
		textbase = self.rd.ReadWord(4)
		entry = self.rd.ReadWord(4)
		print(f"PC = 0x{entry:08X}")
		textsize = self.rd.ReadWord(4)
		datasize = self.rd.ReadWord(4)
		bsssize = self.rd.ReadWord(4)
		relocsize = self.rd.ReadWord(4)
		symtabsize = self.rd.ReadWord(4)
		scd_lineno_size = self.rd.ReadWord(4)
		scd_symtab_size = self.rd.ReadWord(4)
		scd_strtab_size = self.rd.ReadWord(4)
		self.rd.Skip(16)
		bml_offset = self.rd.ReadWord(4)

		print(f"Text segment:")
		print(f"- Offset: 0x{0x40:08X}")
		print(f"- Length: 0x{textsize:08X}")
		print(f"- Address: 0x{textbase:08X}")
		print(f"Data segment:")
		print(f"- Offset: 0x{0x40 + textsize:08X}")
		print(f"- Length: 0x{datasize:08X}")
		print(f"- Address: 0x{textbase + textsize:08X}")
		print(f"Bss segment:")
		print(f"- Length: 0x{bsssize:08X}")
		print(f"- Address: 0x{textbase + textsize + datasize:08X}")
		if relocsize != 0:
			print(f"Relocation table:")
			print(f"- Offset: 0x{0x40 + textsize + datasize:08X}")
			print(f"- Length: 0x{relocsize:08X}")
		if symtabsize != 0:
			print(f"Symbol table:")
			print(f"- Offset: 0x{0x40 + textsize + datasize + relocsize:08X}")
			print(f"- Length: 0x{symtabsize:08X}")

		if scd_lineno_size != 0 or scd_symtab_size != 0 or scd_strtab_size != 0:
			print(f"SCD line number table size: 0x{scd_lineno_size:08X}")
			print(f"SCD symbol table size: 0x{scd_symtab_size:08X}")
			print(f"SCD string table size: 0x{scd_strtab_size:08X}")

		if bml_offset != 0:
			print(f"Bound module list offset: 0x{bml_offset:08X}") # TODO: unsure
			# Google Translate: バインドされたモジュールリストのファイル先頭からの位置
			# Position from the beginning of the file in the bound module list
			# The position of the bound module list from the beginning of the file

		relocs = {}
		if options.get('rel', False) or options.get('showall', False):
			print("== Fixups ==")
			self.rd.Seek(0x40 + textsize + datasize)
			offset = 0
			count = 0
			while count < relocsize:
				word = self.rd.ReadWord(2); count += 2
				if word == 1:
					word = self.rd.ReadWord(4); count += 4
				if word & 1:
					size = 2
					sizename = 'word'
					word &= ~1
				else:
					size = 4
					sizename = 'long word'
				offset += word
				print(f"- 0x{offset:08X}: {sizename}")
				relocs[offset] = size

		if options.get('data', False) or options.get('showall', False):
			print(f"== Text segment ==")
			print("[FILE    ] (SEGMENT ) MEMORY  \tDATA")
			self.rd.Seek(0x40)

			for row, hexcodes, text in self.process_data(options, 4, textsize,
					get_reloc_size = lambda position: relocs.get(position)):
				print(f"[{0x40 + row:08X}] ({row:08X}) {textbase + row:08X}\t{hexcodes}\t{text}")

			print(f"== Data segment ==")
			print("[FILE    ] (SEGMENT ) MEMORY  \tDATA")
			current_offset = textsize % 16
			segment_offset = 0x40 + textsize - current_offset
			segment_base = textbase + textsize - current_offset
			for row, hexcodes, text in self.process_data(options, 4, datasize, data_offset = current_offset,
					get_reloc_size = lambda position: relocs.get(textsize + position)):
				print(f"[{segment_offset + row:08X}] ({row:08X}) {segment_base + row:08X}\t{hexcodes}\t{text}")


class HunkReader(FileReader):
	# TODO: implement format
	def __init__(self, rd):
		super(HunkReader, self).__init__(rd)
		self.rd.endian = Reader.BigEndian
		self.last_hunk_data = False
		self.relocs = None

	def ReadString(self, **options):
		size = self.rd.ReadWord(4)
		if size == 0:
			return None
		name = self.rd.Read(size * 4)
		ix = name.find('\0')
		if ix != -1:
			name = name[:ix]
		return name.decode(options.get('encoding', 'ascii_graphic'))

	def ReadHeaderBlock(self, **options):
		print("== Header block")
		while True:
			library = self.ReadString(**options)
			if library is None:
				break
			print(f"Library: {library!r}")
		table_size = self.rd.ReadWord(4)
		first_hunk = self.rd.ReadWord(4)
		last_hunk = self.rd.ReadWord(4)
		print(f"Total hunk: 0x{table_size:08X}")
		print(f"First hunk: #0x{first_hunk:08X}")
		print(f"Last hunk: #0x{last_hunk:08X}")
		for i in range(first_hunk, last_hunk + 1):
			hunk_size = self.rd.ReadWord(4)
			print(f"Hunk #{i} size: 0x{hunk_size * 4:08X}")

	def ReadCodeOrDataBlock(self, hunk_type, **options):
		if hunk_type == 0x3E9:
			print("== Code block (Motorola 68000 instructions)")
		elif hunk_type == 0x3EA:
			print("== Data block")
		elif hunk_type == 0x4E9:
			print("== Code block (PowerPC instructions)")
		size = self.rd.ReadWord(4)
		offset = self.rd.Tell()
		print(f"- Offset: 0x{offset:08X}")
		print(f"- Length: 0x{size * 4:08X}")
		if options.get('data', False) or options.get('showall', False):
			if not (options.get('relshow', False) or options.get('showall', False)):
				self.DisplayData(size, **options)
			else:
				self.last_hunk_data = (size * 4, self.rd.Tell())
				self.relocs = {}
				self.rd.Skip(size * 4)
		else:
			self.rd.Skip(size * 4)

	def ReadBssBlock(self, **options):
		print("== Bss block")
		size = self.rd.ReadWord(4)
		print(f"- Length: 0x{size * 4:08X}")

	def ReadRelocBlock(self, hunk_type, **options):
		if hunk_type == 0x3EC:
			print("== 32-bit relocations")
		else:
			print(f"Internal error: unhandled type 0x{hunk_type:08X}")
		while True:
			offset_count = self.rd.ReadWord(4)
			if offset_count == 0:
				break
			hunk_number = self.rd.ReadWord(4)
			print(f"- Offsets to hunk number #{hunk_number} (number: {offset_count})")
			if options.get('rel', False) or options.get('relshow', False) or options.get('showall', False):
				for i in range(offset_count):
					pos = self.rd.Tell()
					offset = self.rd.ReadWord(4)
					if options.get('rel', False) or options.get('showall', False):
						print(f"[{offset:08X}] 32-bit relocation at 0x{offset:08X} to #{hunk_number}")
					if options.get('relshow', False) or options.get('showall', False):
						self.relocs[offset] = 4
			else:
				self.rd.Skip(offset_count * 4)

	def ReadEndBlock(self, **options):
		if ((options.get('data', False) and options.get('relshow', False)) or options.get('showall', False)) \
		and self.last_hunk_data is not None:
			pos = self.rd.Tell()
			size = self.last_hunk_data[0]
			self.rd.Seek(self.last_hunk_data[1])
			self.last_hunk_data = None
			self.DisplayData(size, **options)
			self.relocs = None
			self.rd.Seek(pos)
		print("== End hunk")

	def DisplayData(self, size, **options):
		print("- Hunk data")
		offset = self.rd.Tell()
		for row, hexcodes, text in self.process_data(options, 4, size,
				get_reloc_size = lambda position: self.relocs.get(position) if self.relocs is not None else None,
				text_encoding = 'ascii_graphic'):
			print(f"[{offset + row:08X}] {row:08X}\t{hexcodes}\t{text}")

	def ReadHunk(self, **options):
		hunk_type = self.rd.ReadWord(4)
		if hunk_type in {0x3E9, 0x3EA, 0x4E9}:
			self.ReadCodeOrDataBlock(hunk_type, **options)
		elif hunk_type == 0x3EB:
			self.ReadBssBlock(**options)
		elif hunk_type in {0x3EC}:
			self.ReadRelocBlock(hunk_type, **options)
		elif hunk_type == 0x3F2:
			self.ReadEndBlock(**options)
		elif hunk_type == 0x3F3:
			self.ReadHeaderBlock(**options)
			# TODO: other types
		else:
			print(f"Error: unknown hunk type (0x{hunk_type:08X}), exiting", file = sys.stderr)
			exit(1)

	def ReadFile(self, **options):
		print("==== Hunk format ====")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(0)
		while self.rd.Tell() < size:
			self.ReadHunk(**options)

class MacintoshResourceReader(FileReader):
	MOVE_DATA_SP = 0x3F3C
	LOADSEG = 0xA9F0

	def __init__(self, rd, offset = 0):
		super(MacintoshResourceReader, self).__init__(rd)
		self.rd.endian = Reader.BigEndian
		self.offset = offset

	def ReadFile(self, **options):
		if self.offset == 0:
			print("==== Macintosh resource format ====")
		else:
			print(f"- Resource fork offset: 0x{self.offset:08X}")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(self.offset)
		data_offset = self.rd.ReadWord(4)
		map_offset = self.rd.ReadWord(4)
		data_length = self.rd.ReadWord(4)
		map_length = self.rd.ReadWord(4)
		print(f"Data offset: 0x{data_offset:08X} (0x{self.offset + data_offset:08X} in file), length: 0x{data_length:08X}")
		print(f"Map offset: 0x{map_offset:08X} (0x{self.offset + map_offset:08X} in file), length: 0x{map_length:08X}")
		self.rd.Seek(self.offset + map_offset + 22)
		fork_attributes = self.rd.ReadWord(2)
		print(f"Fork attributes: 0x{fork_attributes:04X}")
		resource_type_list_offset = self.rd.ReadWord(2)
		print(f"Offset to resource type list: 0x{resource_type_list_offset:02X} (0x{self.offset + map_offset + resource_type_list_offset:08X} in file)")
		resource_name_list_offset = self.rd.ReadWord(2)
		print(f"Offset to resource name list: 0x{resource_name_list_offset:02X} (0x{self.offset + map_offset + resource_name_list_offset:08X} in file)")
		if self.rd.Tell() != self.offset + map_offset + resource_type_list_offset:
			print(f"Warning: skipped bytes before resource type list: 0x{self.offset + map_offset + resource_type_list_offset - self.rd.Tell()}")
		self.rd.Seek(self.offset + map_offset + resource_type_list_offset)
		resource_type_count = self.rd.ReadWord(2) + 1
		print(f"Number of resource types: 0x{resource_type_count:04X}")
		for type_number in range(resource_type_count):
			self.rd.Seek(self.offset + map_offset + resource_type_list_offset + 2 + type_number * 8)
			resource_type = self.rd.Read(4)
			resource_type_text = resource_type.decode(options.get('encoding', 'macroman_graphic'))
			resource_count = self.rd.ReadWord(2) + 1
			resource_offset = self.rd.ReadWord(2)
			print(f"Type #{type_number + 1}: {resource_type_text}, count: {resource_count}, offset: 0x{resource_offset:04X} (0x{self.offset + map_offset + resource_type_list_offset + resource_offset} in file)")
			for resource_number in range(resource_count):
				self.rd.Seek(self.offset + map_offset + resource_type_list_offset + resource_offset + resource_number * 12)
				resource_id = self.rd.ReadWord(2)
				resource_name_offset = self.rd.ReadWord(2)
				resource_data_offset = self.rd.ReadWord(4)
				resource_attributes = resource_data_offset >> 24
				resource_data_offset &= 0x00FFFFFF
				print(f"- Resource #{resource_number} ID: 0x{resource_id:04X}", end = '')
				if resource_name_offset != 0xFFFF:
					# TODO: test
					self.rd.Seek(self.offset + map_offset + resource_name_list_offset + resource_offset + resource_name_offset)
					resource_name_length = self.rd.ReadWord(1)
					resource_name = self.rd.Read(resource_name_length)
					resource_name = resource_name.decode(options.get('encoding', 'macroman_graphic'))
					print(f", name: {resource_name} (offset: 0x{resource_name_offset:04X} 0x{self.offset + map_offset + resource_name_list_offset + resource_offset + resource_name_offset:04X} in file", end = '')
				if resource_attributes != 0:
					print(f", attributes: 0x{resource_attributes:02X}", end = '')
				print(f", data offset: 0x{resource_data_offset:06X} (0x{self.offset + data_offset + resource_data_offset:08X} in file)")
				self.rd.Seek(self.offset + data_offset + resource_data_offset)
				resource_size = self.rd.ReadWord(4)
				print(f"Resource offset in file: 0x{self.offset + data_offset + resource_data_offset + 4:08X}")
				print(f"Resource length: 0x{resource_size:08X}")
				if resource_type == b'CODE':
					if resource_id == 0:
						above_a5 = self.rd.ReadWord(4)
						print(f"Above A5: 0x{above_a5:08X}")
						below_a5 = self.rd.ReadWord(4)
						print(f"Below A5: 0x{below_a5:08X}")
						jump_table_size = self.rd.ReadWord(4)
						print(f"Jump table size: 0x{jump_table_size:08X}")
						if 16 + jump_table_size != resource_size:
							print(f"Warning: expected jump table size to be 0x{resource_size - 16}")
						if jump_table_size % 8 != 0:
							print(f"Warning: expected jump table size to be multiple of 8 bytes")
						jump_table_offset = self.rd.ReadWord(4)
						if jump_table_offset != 0x20:
							print(f"Warning: expected jump table offset to be 0x20")
							print("Jump table offset: 0x{jump_table_offset}")
						far_entries = False
						for offset in range(0, jump_table_size, 8):
							word1 = self.rd.ReadWord(2)
							word2 = self.rd.ReadWord(2)
							word3 = self.rd.ReadWord(2)
							word4 = self.rd.ReadWord(2)
							if word2 == MacintoshResourceReader.LOADSEG:
								# far entry
								if not far_entries:
									print(f"Error: far entry before separator at offset 0x{offset:08X}", file = sys.stderr)
								print(f"Entry at 0x{offset:08X}: far, segment 0x{word1:04X}:0x{(word3 << 16) | word4:04X}")
							elif word4 == MacintoshResourceReader.LOADSEG:
								# near entry
								if word2 == MacintoshResourceReader.MOVE_DATA_SP:
									if far_entries:
										print(f"Error: near entry after separator at offset 0x{offset:08X}", file = sys.stderr)
									print(f"Entry at 0x{offset:08X}: near, segment 0x{word3:04X}:0x{word1:04X}")
								else:
									print(f"Entry at 0x{offset:08X}: near (unknown format), segment 0x{word3:04X}, content: 0x{(word1 << 16) | word2:08X}")
							elif word1 == word3 == word4 == 0 and word2 == 0xFFFF:
								# separator
								if far_entries:
									print(f"Error: multiple separator entries at offset 0x{offset:08X}", file = sys.stderr)
								print(f"Entry at 0x{offset:08X}: separator")
								far_entries = True
							else:
								print(f"Error: invalid entry at offset 0x{offset:08X}", file = sys.stderr)
								print(f"Entry at 0x{offset:08X}: unknown, 0x{word1:04X}, 0x{word2:04X}, 0x{word3:04X}, 0x{word4:04X}")
					else:
						near_entry_offset = self.rd.ReadWord(2)
						near_entry_count = self.rd.ReadWord(2)
						if near_entry_offset == 0xFFFF and near_entry_count == 0x0000:
							# TODO: not tested
							# far segment
							near_entry_offset = self.rd.ReadWord(4)
							near_entry_count = self.rd.ReadWord(4)
							far_entry_offset = self.rd.ReadWord(4)
							far_entry_count = self.rd.ReadWord(4)
							a5_reloc_offset = self.rd.ReadWord(4)
							a5_address = self.rd.ReadWord(4)
							segment_reloc_offset = self.rd.ReadWord(4)
							segment_address = self.rd.ReadWord(4)
							self.rd.Skip(4) # reserved
							segment_offset = self.offset + data_offset + resource_data_offset + 44
							segment_length = a5_reloc_offset - 40
							print(f"Segment offset: 0x{segment_offset:08X}")
							print(f"Segment length: 0x{segment_length:08X}")
							print(f"Near entry offset: 0x{near_entry_offset:08X}")
							print(f"Near entry count: 0x{near_entry_count:08X}")
							print(f"Far entry offset: 0x{far_entry_offset:08X}")
							print(f"Far entry count: 0x{far_entry_count:08X}")
							print(f"A5 base address: 0x{a5_address:08X} (0x{self.offset + data_offset + resource_data_offset + 4 + a5_address:08X} in file)")
							print(f"A5 relocations offset: 0x{a5_reloc_offset:08X}")
							print(f"Segment base address: 0x{segment_address:08X} (0x{self.offset + data_offset + resource_data_offset + 4 + segment_address:08X} in file)")
							print(f"Segment relocations offset: 0x{segment_reloc_offset:08X}")

							# TODO: relocations
						else:
							# near segment
							segment_offset = self.offset + data_offset + resource_data_offset + 8
							segment_length = resource_size - 4
							print(f"Segment offset: 0x{segment_offset:08X}")
							print(f"Segment length: 0x{segment_length:08X}")
							print(f"Near entry offset: 0x{near_entry_offset:04X}")
							print(f"Near entry count: 0x{near_entry_count:04X}")

						if options.get('data', False) or options.get('showall', False):
							print("[FILE    ] SEGMENT \tDATA")
							self.rd.Seek(segment_offset)
							for row, hexcodes, text in self.process_data(options, 4, segment_length,
									text_encoding = 'macroman_graphic'):
								print(f"[{segment_offset + row:08X}] {row:08X}\t{hexcodes}\t{text}")

				else:
					if options.get('data', False) or options.get('showall', False):
						print("[FILE    ] SEGMENT \tDATA")
						for row, hexcodes, text in self.process_data(options, 4, resource_size,
								text_encoding = 'macroman_graphic'):
							print(f"[{self.offset + data_offset + resource_data_offset + row:08X}] {row:08X}\t{hexcodes}\t{text}")

class AppleReader(FileReader):
	def __init__(self, rd):
		super(AppleReader, self).__init__(rd)
		self.rd.endian = Reader.BigEndian

	def ReadFile(self, **options):
		print("==== AppleSingle/AppleDouble format ====")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(0)
		magic = self.rd.ReadWord(4)
		if magic == 0x00051600:
			print("AppleSingle")
		elif magic == 0x00051607:
			print("AppleDouble")
		elif magic == struct.unpack('>I', b'Joy!')[0]:
			print("PEF")
			print(f"Error: unsupported file format", file = sys.stderr)
			return
		else:
			print("Unknown magic format, assuming Macintosh resource file")
			MacintoshResourceReader(self.rd).ReadFile(**options)
			return
		version = self.rd.ReadWord(4)
		if version == 0x10000:
			print("Version 1")
		elif version == 0x20000:
			print("Version 2")
		else:
			print(f"Unknown version 0x{version:08X}")
		host_system = self.rd.Read(16)
		if host_system != bytes(16):
			host_system_text = host_system.decode(options.get('encoding', 'macroman_graphic'))
			print(f"Host system: {host_system_text}")
			if self.version != 0x10000:
				print("Warning: only version 1 allows a host system to be present")
		entry_count = self.rd.ReadWord(2)
		print(f"Entry count: {entry_count}")
		entries = []
		entry_types = set()
		for number in range(entry_count):
			entry_id = self.rd.ReadWord(4)
			entry_offset = self.rd.ReadWord(4)
			entry_length = self.rd.ReadWord(4)
			entry_id_text = {
				1: "Data Fork",
				2: "Resource Fork",
				3: "Real Name",
				4: "Comment",
				5: "Icon, Black & White",
				6: "Icon, Color",
				7: "File Info",
				8: "File Dates Info",
				9: "Finder Info",
				10: "Macintosh File Info",
				11: "ProDOS File Info",
				12: "MS-DOS File Info",
				13: "AFP Short Name",
				14: "AFP File Info",
				15: "Directory ID",
			}.get(entry_id, "undefined")
			if entry_id in entry_types and 1 <= entry_id <= 15:
				print(f"Warning: Duplicate {entry_id_text}", file = sys.stderr)
			elif version != 0x00010000 and entry_id == 7:
				print("Warning: File Info only supported for version 1", file = sys.stderr)
			elif version == 0x00010000 and 10 <= entry_id <= 15:
				print(f"Warning: {entry_id_text} only supported for version 2", file = sys.stderr)
			elif magic == 0x00051607 and entry_id == 1:
				print("Warning: Data Fork only supposed to be in AppleSingle", file = sys.stderr)
			entry_types.add(entry_id)
			print(f"Entry: {entry_id_text} (0x{entry_id:08X}), offset: 0x{entry_offset:08X}, length: 0x{entry_length:08X}")
			entries.append((entry_id, entry_offset, entry_length))
		for (e_id, e_offset, e_length) in entries:
			if e_id == 1:
				print("=== Data Fork ===")
				pass # TODO
			elif e_id == 2:
				print("=== Resource Fork ===")
				MacintoshResourceReader(self.rd, e_offset).ReadFile(**options)
				continue
			elif e_id == 3:
				print("=== Real Name ===")
				pass # TODO
			elif e_id == 4:
				print("=== Comment ===")
				pass # TODO
			elif e_id == 5:
				print("=== Icon, Black & White ===")
				pass # TODO
			elif e_id == 6:
				print("=== Icon, Color ===")
				pass # TODO
			elif e_id == 7:
				print("=== File Info ===")
				pass # TODO
			elif e_id == 8:
				print("=== File Dates Info ===")
				pass # TODO
			elif e_id == 9:
				print("=== Finder Info ===")
				pass # TODO
			elif e_id == 10:
				print("=== Macintosh File Info ===")
				pass # TODO
			elif e_id == 11:
				print("=== ProDOS File Info ===")
				pass # TODO
			elif e_id == 12:
				print("=== MS-DOS File Info ===")
				pass # TODO
			elif e_id == 13:
				print("=== AFP Short Name ===")
				pass # TODO
			elif e_id == 14:
				print("=== AFP File Info ===")
				pass # TODO
			elif e_id == 15:
				print("=== AFP Directory ID ===")
				pass # TODO
			if options.get('data', False) or options.get('showall', False):
				self.rd.Seek(e_offset)
				print("[FILE    ] ENTRY   \tDATA")
				for row, hexcodes, text in self.process_data(options, 4, e_length,
						text_encoding = 'macroman_graphic'):
					print(f"[{e_offset + row:08X}] {row:08X}\t{hexcodes}\t{text}")

class PEFReader(FileReader):
	# TODO: implement format
	def __init__(self, rd):
		super(PEFReader, self).__init__(rd)
		self.rd.endian = Reader.BigEndian
		self.offset = 0

	def ReadFile(self, **options):
		print("==== Macintosh PEF format ====")
		if self.offset != 0:
			print(f"- Image offset: 0x{self.offset:08X}")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(self.offset)

class GSOSOMFReader(FileReader):
	def __init__(self, rd):
		super(GSOSOMFReader, self).__init__(rd)
		self.rd.endian = Reader.LittleEndian
		self.label_size = 0

	def ReadName(self):
		if self.label_size == 0:
			label_size = self.rd.ReadWord(1)
			return self.rd.Read(label_size)
		else:
			return self.rd.Read(self.label_size)

	def ReadFile(self, **options):
		print("==== GS/OS OMF format ====")
		self.rd.SeekEnd()
		size = self.rd.Tell()
		self.rd.Seek(0)

		print(f"Total file size: 0x{size:08X}")

		current_segment_offset = 0
		segment_number = 1

		while current_segment_offset < size:
			self.rd.Seek(current_segment_offset + 0x0F)
			version = self.rd.ReadWord(1)
			self.rd.Seek(current_segment_offset + 0x20)
			endian = self.rd.ReadWord(1)
			if endian == 0:
				self.rd.endian = Reader.LittleEndian
			elif endian == 1:
				self.rd.endian = Reader.BigEndian
			self.rd.Seek(current_segment_offset)
			segment_file_size = self.rd.ReadWord(4)
			if version == 1:
				unshifted_segment_file_size = segment_file_size
				segment_file_size <<= 9
			zero_fill_size = self.rd.ReadWord(4)
			memory_size = self.rd.ReadWord(4)

			endian_name = {
				0: "little-endian",
				1: "big-endian"
			}.get(endian, "undefined")

			if version == 1:
				kind = self.rd.ReadWord(1)
			else:
				self.rd.Skip(1)
			self.label_size = self.rd.ReadWord(1)
			number_size = self.rd.ReadWord(1)
			self.rd.Skip(1) # version
			bank_size = self.rd.ReadWord(4)

			if version == 2:
				kind = self.rd.ReadWord(2)
				self.rd.Skip(2)
			else:
				self.rd.Skip(4)

			base = self.rd.ReadWord(4)
			align = self.rd.ReadWord(4)

			self.rd.Skip(1) # endian

			if version == 1:
				lcbank = self.rd.ReadWord(1)
				revision = 0
			else:
				revision = self.rd.ReadWord(1)

			segnum = self.rd.ReadWord(2)
			if segnum != segment_number:
				print(f"Error: invalid segment number, expected 0x{segment_number:04X}, received 0x{segnum:04X}", file = sys.stderr)

			entry = self.rd.ReadWord(4)

			loadname_offset = self.rd.ReadWord(2)
			segment_data_offset = self.rd.ReadWord(2)
			if loadname_offset >= 48:
				virtual_base = self.rd.ReadWord(4)
			else:
				virtual_base = None

			self.rd.Seek(current_segment_offset + loadname_offset)

			object_name = self.rd.Read(10).decode(options.get('encoding', 'ascii_graphic'))
			segment_name = self.ReadName().decode(options.get('encoding', 'ascii_graphic'))

			print(f"=== Segment #{segment_number}:")
			print(f"Name: {segment_name!r} (object: {object_name!r}) at offset 0x{loadname_offset:04X} (0x{current_segment_offset + loadname_offset:08X} in file)")
			print(f"- Header offset: 0x{current_segment_offset:08X}")
			print(f"- Data offset: 0x{segment_data_offset:04X} (0x{current_segment_offset + segment_data_offset:08X} in file)")
			if version == 1:
				print(f"- File length: 0x{segment_file_size:08X} (0x{unshifted_segment_file_size:08X})")
			else:
				print(f"- File length: 0x{segment_file_size:08X}")
			print(f"- Memory size: 0x{memory_size:08X}")
			print(f"- Zero fill: 0x{zero_fill_size:08X}")
			print(f"- Address: 0x{base:08X}")
			if virtual_base is not None:
				print(f"- Virtual address: 0x{virtual_base:08X}")
			print(f"- Align: 0x{align:08X}")
			print(f"Version: {version}.{revision}")

			print(f"Endian: {endian_name} (0x{endian:02X})")
			print(f"Number size: 0x{number_size:02X}")

			if version == 2:
				kind_name = {
					0x00: "code segment",
					0x01: "data segment",
					0x02: "jump table segment",
					0x04: "pathname segment",
					0x08: "library dictionary segment",
					0x10: "initialization segment",
					0x12: "direct page/stack segment",
				}.get(kind & 0x1F)
				print(f"Kind: {kind_name} (0x{kind:02X})", end = '')
				if kind & 0x0100:
					print(", bank relative", end = '')
				if kind & 0x0200:
					print(", skip", end = '')
				if kind & 0x0400:
					print(", reload", end = '')
				if kind & 0x0800:
					print(", absolute bank", end = '')
				if not kind & 0x1000:
					print(", can be loaded to special memory", end = '')
				if kind & 0x2000:
					print(", position independent", end = '')
				if kind & 0x4000:
					print(", private", end = '')
				if kind & 0x8000:
					print(", dynamic", end = '')
				else:
					print(", static", end = '')
				print()
			else:
				kind_name = {
					0x00: "code segment",
					0x01: "data segment",
					0x02: "jump table segment",
					0x04: "pathname segment",
					0x08: "library dictionary segment",
					0x10: "initialization segment",
					0x11: "absolute bank segment",
					0x12: "direct page/stack segment",
				}.get(kind & 0x1F)
				print(f"Kind: {kind_name} (0x{kind:02X})", end = '')
				if kind & 0x20:
					print(", position independent", end = '')
				if kind & 0x40:
					print(", private", end = '')
				if kind & 0x80:
					print(", dynamic", end = '')
				else:
					print(", static", end = '')
				print()

			if self.label_size == 0:
				print(f"Label size: variable length")
			else:
				print(f"Label size: 0x{self.label_size:02X}")

			print(f"Bank size: 0x{bank_size:08X}")

			if version == 1:
				print(f"Language card bank: {lcbank+1}")

			print(f"Entry: 0x{entry:08X} within segment")

			self.rd.Seek(current_segment_offset + segment_data_offset)
			segment_data_end = current_segment_offset + segment_file_size

			show_records = options.get('relshow', False) or options.get('showall', False)
			if show_records:
				print("== Records")

			segment_data = bytes()
			relocs = {}

			while self.rd.Tell() < segment_data_end:
				record_location = self.rd.Tell()
				record_type = self.rd.ReadWord(1)
				record_name, record_size = {
					0x00: ('END', 0),
					0xE0: ('ALIGN', None), # object
					0xE1: ('ORG', None), # object
					0xE2: ('RELOC', None), # load
					0xE3: ('INTERSEG', None), # load
					0xE4: ('USING', None), # object
					0xE5: ('STRONG', None), # object
					0xE6: ('GLOBAL', None), # object
					0xE7: ('GEQU', None), # object
					0xE8: ('MEM', None), # object
					0xE9: ('undefined', None),
					0xEA: ('undefined', None),
					0xEB: ('EXPR', None), # object
					0xEC: ('ZEXPR', None), # object
					0xED: ('BEXPR', None), # object
					0xEE: ('RELEXPR', None), # object
					0xEF: ('LOCAL', None), # object
					0xF0: ('EQU', None), # object
					0xF1: ('DS', None),
					0xF2: ('LCONST', None),
					0xF3: ('LEXPR', None), # object
					0xF4: ('ENTRY', None), # run-time library dictionary
					0xF5: ('cRELOC', None), # load
					0xF6: ('cINTERSEG', None), # load
					0xF7: ('SUPER', None), # load
					0xF8: ('undefined', None),
					0xF9: ('undefined', None),
					0xFA: ('undefined', None),
					0xFB: ('undefined', None),
					0xFC: ('undefined', None),
					0xFD: ('undefined', None),
					0xFE: ('undefined', None),
					0xFF: ('undefined', None),
				}.get(record_type, ('CONST', record_type)) # object
				if show_records:
					print(f"[0x{record_location:08X}] {record_name} (0x{record_type:02X})")
				if record_type == 0x00:
					# END
					break
				elif record_type == 0xE2 or record_type == 0xF5:
					# RELOC, cRELOC
					reloc_size = self.rd.ReadWord(1)
					reloc_shift = self.rd.ReadWord(1, signed = True)
					reloc_offset = self.rd.ReadWord(4 if record_type == 0xE2 else 2)
					reloc_addend = self.rd.ReadWord(4 if record_type == 0xE2 else 2)
					if show_records:
						print(f"- Size: 0x{reloc_size:02X}", end = '')
						if reloc_shift != 0:
							print(f", shift: {reloc_shift}", end = '')
						print(f", offset: 0x{reloc_offset:08X}, target: 0x{reloc_addend:08X}")
					relocs[reloc_offset] = reloc_size
				elif record_type == 0xE3 or record_type == 0xF6:
					# INTERSEG, cINTERSEG
					reloc_size = self.rd.ReadWord(1)
					reloc_shift = self.rd.ReadWord(1, signed = True)
					reloc_offset = self.rd.ReadWord(4 if record_type == 0xE3 else 2)
					reloc_file = self.rd.ReadWord(2) if record_type == 0xE3 else 1
					reloc_segment = self.rd.ReadWord(2 if record_type == 0xE3 else 1)
					reloc_addend = self.rd.ReadWord(4 if record_type == 0xE3 else 2)
					if show_records:
						print(f"- Size: 0x{reloc_size:02X}", end = '')
						if reloc_shift != 0:
							print(f", shift: {reloc_shift}", end = '')
						print(f", offset: 0x{reloc_offset:08X}, target: ", end = '')
						if reloc_file != 1:
							print(f", file #{reloc_file}", end = '')
						print(f"#0x{reloc_segment:02X}:0x{reloc_addend:08X}")
					relocs[reloc_offset] = reloc_size
				elif record_type == 0xF1:
					# DS
					count = self.rd.ReadWord(number_size)
					if show_records:
						print(f"- Zero bytes: 0x{count:0{2*number_size}X}")
					segment_data += bytes(count)
				elif record_type == 0xF2:
					# LCONST
					count = self.rd.ReadWord(4)
					data = self.rd.Read(count)
					segment_data += data
					text = data.decode(options.get('encoding', 'ascii_graphic'))
					if show_records:
						print(f"- Data: {text!r}")
				elif record_type == 0xF7:
					# SUPER
					record_size = self.rd.ReadWord(4)
					if record_size == 0:
						if show_records:
							print(f"- Size: 0x{record_size:08X}")
						continue
					record_end = self.rd.Tell() + record_size
					record_type = self.rd.ReadWord(1)
					if record_type > 37:
						if show_records:
							print(f"- Size: 0x{record_size:08X}, type: unknown (0x{record_type:02X})")
						continue
					elif record_type == 0:
						if show_records:
							print(f"- Size: 0x{record_size:08X}, type: RELOC2 (0x{record_type:02X})")
					elif record_type == 1:
						if show_records:
							print(f"- Size: 0x{record_size:08X}, type: RELOC3 (0x{record_type:02X})")
					else:
						if show_records:
							print(f"- Size: 0x{record_size:08X}, type: INTERSEG{record_type-1} (0x{record_type:02X})")
					while self.rd.Tell() < record_end:
						if record_type <= 1:
							# RELOC2-RELOC3
							reloc_size = 2 + record_type
							reloc_offset = self.rd.ReadWord(2)
							reloc_addend = self.rd.ReadWord(2)
							if show_records:
								print(f"-- Size: 0x{reloc_size:02X}", end = '')
								print(f", offset: 0x{reloc_offset:08X}, target: 0x{reloc_addend:08X}")
							relocs[reloc_offset] = reloc_size
						else:
							if record_type == 2:
								# INTERSEG1
								reloc_size = 3 # changed
								reloc_shift = 0 # changed
								reloc_offset = self.rd.ReadWord(2)
								reloc_file = 1
								reloc_segment = self.rd.ReadWord(1)
								reloc_addend = self.rd.ReadWord(2)
							elif record_type < 14:
								# INTERSEG2-INTERSEG12
								reloc_size = 3 # changed
								reloc_shift = 0 # changed
								reloc_offset = self.rd.ReadWord(2)
								reloc_file = record_type - 1 # changed
								reloc_segment = self.rd.ReadWord(1)
								reloc_addend = self.rd.ReadWord(2)
							elif record_type < 26:
								# INTERSEG13-INTERSEG24
								reloc_size = 2 # changed
								reloc_shift = 0 # changed
								reloc_offset = self.rd.ReadWord(2)
								reloc_file = 1
								reloc_segment = record_type - 14
								reloc_addend = self.rd.ReadWord(2)
							else:
								# INTERSEG26-INTERSEG36
								reloc_size = 2 # changed
								reloc_shift = -16 # changed
								reloc_offset = self.rd.ReadWord(2)
								reloc_file = 1
								reloc_segment = record_type - 26 # changed
								reloc_addend = self.rd.ReadWord(2)
							if show_records:
								print(f"- Size: 0x{reloc_size:02X}", end = '')
								if reloc_shift != 0:
									print(f", shift: {reloc_shift}", end = '')
								print(f", offset: 0x{reloc_offset:08X}, target: ", end = '')
								if reloc_file != 1:
									print(f", file #{reloc_file}", end = '')
								print(f"#0x{reloc_segment:02X}:0x{reloc_addend:08X}")
							relocs[reloc_offset] = reloc_size
				elif record_size is None:
					print(f"Internal error: unknown record size")
					assert False
					break
				else:
					# TODO: other record types
					self.rd.Skip(record_size)

			print("[FILE    ] MEMORY  \tDATA")
			for row, hexcodes, text in self.process_data(options, 4, segment_data,
					get_reloc_size = lambda position: relocs.get(position)):
				print(f"({row:08X}) {base + row:08X}\t{hexcodes}\t{text}")

			###

			segment_number += 1
			current_segment_offset += segment_file_size
			self.rd.Seek(current_segment_offset)

def usage(argv0):
	print(f"Usage: {argv0} -F{{cmd|mz|ne|le|pe|aout|coff|minix|68k|tos|zfile|cdos68k|hu|hunk|apple|rsrc|gsos}} <filename>", file = sys.stderr)

def main():
	if len(sys.argv) < 2:
		usage(sys.argv[0])
		exit(0)
	filename = None
	options = {}
	i = 1
	fmt = None
	while i < len(sys.argv):
		if sys.argv[i] == '-Fcmd':
			fmt = CPM86Reader
		elif sys.argv[i] == '-Fmz':
			fmt = MZReader
		elif sys.argv[i] == '-Fne':
			fmt = NEReader
		elif sys.argv[i] == '-Fle' or sys.argv[i] == '-Flx':
			fmt = LEReader
		elif sys.argv[i] == '-Fpe':
			fmt = PEReader
		elif sys.argv[i] == '-Faout':
			fmt = AOutReader
		elif sys.argv[i] == '-Fcoff':
			fmt = COFFReader
		elif sys.argv[i] == '-Fminix':
			fmt = MINIXAOutReader
		elif sys.argv[i] == '-F68k':
			fmt = lambda rd: CPM68KReader('cpm68k', rd)
		elif sys.argv[i] == '-Ftos':
			fmt = lambda rd: CPM68KReader('gemdos', rd)
		elif sys.argv[i] == '-Fzfile':
			fmt = lambda rd: CPM68KReader('human68k', rd)
		elif sys.argv[i] == '-Fcdos68k':
			fmt = lambda rd: CPM68KReader('cdos68k', rd)
		elif sys.argv[i] == '-Fhu':
			fmt = HUReader
		elif sys.argv[i] == '-Fhunk':
			fmt = HunkReader
		elif sys.argv[i] == '-Fapple':
			fmt = AppleReader
		elif sys.argv[i] == '-Frsrc':
			fmt = MacintoshResourceReader
		elif sys.argv[i] == '-Fgsos':
			fmt = GSOSOMFReader
		elif sys.argv[i].startswith('-F'):
			print(f"Error: unknown format {sys.argv[i]}", file = sys.stderr)
			usage(sys.argv[0])
			exit(0)
		elif sys.argv[i].startswith('-O'):
			option = sys.argv[i][2:]
			ix = option.find('=')
			if ix != -1:
				value = option[ix + 1:]
				option = option[:ix]
			else:
				value = True
			options[option] = value
		elif sys.argv[i].startswith('-'):
			print(f"Error: unknown option {sys.argv[i]}", file = sys.stderr)
			usage(sys.argv[0])
			exit(0)
		else:
			if filename is not None:
				print(f"Error: multiple files provided, using {filename} instead of {sys.argv[i]}", file = sys.stderr)
			else:
				filename = sys.argv[i]
		i += 1
	if filename is None:
		print(f"Error: no file provided", file = sys.stderr)
		usage(sys.argv[0])
		exit(0)
	with open(filename, "rb") as fp:
		rd = Reader(fp)
		if fmt is None:
			print("No format provided, attempting to automatically determine it")
			fmtname = Determiner(rd).ReadMagic()
			if fmtname is None:
				print(f"Error: unable to determine file format, exiting", file = sys.stderr)
				exit(1)
			elif fmtname == Determiner.FMT_MZ:
				print("Attempting MZ format")
				fmt = MZReader
			elif fmtname == Determiner.FMT_NE:
				print("Attempting NE format")
				fmt = NEReader
			elif fmtname == Determiner.FMT_LE:
				print("Attempting LE/LX format")
				fmt = LEReader
			elif fmtname == Determiner.FMT_68K_CONTIGUOUS:
				print("Attempting GEMDOS format")
				fmt = lambda rd: CPM68KReader('gemdos', rd)
			elif fmtname == Determiner.FMT_68K_NONCONTIGUOUS:
				print("Attempting CP/M-68K non-contiguous format")
				fmt = lambda rd: CPM68KReader('cpm68k', rd)
			elif fmtname == Determiner.FMT_68K_CRUNCHED:
				print("Attempting Concurrent DOS 68K crunched format")
				#fmt = None # TODO
			elif fmtname == Determiner.FMT_HU:
				print("Attempting HU format")
				fmt = HUReader
			elif fmtname == Determiner.FMT_Apple:
				print("Attempting AppleSingle/AppleDouble format")
				fmt = AppleReader
			elif fmtname == Determiner.FMT_MacRsrc:
				# TODO: this is not actually returned
				print("Attempting Macintosh resource format")
				fmt = MacintoshResourceReader
			elif fmtname == Determiner.FMT_AOut:
				print("Attempting 32-bit a.out format")
				fmt = lambda rd: AOutReader('djgpp', rd)
			elif fmtname == Determiner.FMT_MINIX:
				print("Attempting MINIX a.out format")
				fmt = MINIXAOutReader
			elif fmtname == Determiner.FMT_COFF:
				print("Attempting COFF format")
				fmt = COFFReader
			elif fmtname == Determiner.FMT_ELF:
				print("Attempting ELF format")
				#fmt = None # TODO
			elif fmtname == Determiner.FMT_MP_MQ:
				print("Attempting MP/MQ format")
				#fmt = None # TODO
			elif fmtname == Determiner.FMT_P2_P3:
				print("Attempting P2/P3 format")
				#fmt = None # TODO
			elif fmtname == Determiner.FMT_BW:
				print("Attempting BW format")
				#fmt = None # TODO
			elif fmtname == Determiner.FMT_UZI280:
				print("Attempting UZI-280 format")
				#fmt = None # TODO
			elif fmtname == Determiner.FMT_PE:
				print("Attempting PE format")
				fmt = PEReader
			elif fmtname == Determiner.FMT_Hunk:
				print("Attempting Hunk format")
				fmt = HunkReader
			elif fmtname == Determiner.FMT_Adam:
				print("Attempting Adam format")
				#fmt = None # TODO
			elif fmtname == Determiner.FMT_D3X:
				print("Attempting D3X format")
				#fmt = None # TODO
			elif fmtname == Determiner.FMT_DX64: # TODO: real name?
				print("Attempting DX64 format")
				#fmt = None # TODO
			elif fmtname == Determiner.FMT_CPM8000:
				print("Attempting CP/M-8000 format")
				#fmt = None # TODO
		if fmt is None:
			print("Parser not implemented", file = sys.stderr)
			exit(1)
		reader = fmt(rd)
		reader.ReadFile(**options)

if __name__ == '__main__':
	main()

