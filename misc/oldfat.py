#! /usr/bin/python3

# support for early FAT versions (as old as 86-DOS 0.1)
# Usage: oldfat.py -i <image> -f <FAT version> <command>
# supported FAT versions: 0 (86-DOS pre-0.42), 1 (86-DOS 0.42)
# supported commands ls/dir, cat/type, cp/copy; on-system files have to be prefixed with :

import os
import sys
import struct

class File:
	ATTR_READONLY = 0x01
	ATTR_HIDDEN = 0x02
	ATTR_SYSTEM = 0x04
	ATTR_VOLUME = 0x08
	ATTR_DIRECTORY = 0x10
	ATTR_ARCHIVE = 0x20
	ATTR_DEVICE = 0x40

	def __init__(self, **kwds):
		self.filename = kwds.get('filename', '')
		self.extension = kwds.get('extension', '')
		self.attributes = kwds.get('attributes', set())
		self.modification_date = kwds.get('modification_date', (1980, 1, 1))
		self.modification_time = kwds.get('modification_time', (0, 0, 0))
		self.first_cluster = kwds.get('first_cluster', 0)
		self.file_size = kwds.get('file_size', 0)
		self.deleted = kwds.get('deleted', False)

class AbstractFAT:
	def __init__(self, fp):
		self.fp = fp
		self.encoding = 'CP437'
		self.initialize()

	def initialize(self):
		pass

	def get_fat_type(self):
		assert False

	def get_fat_count(self):
		assert False

	def get_fat_start(self):
		assert False

	def get_fat_size(self):
		assert False

	def get_root_start(self):
		assert False

	def get_root_entry_count(self):
		assert False

	def get_root_entry_size(self):
		assert False

	def get_clusters_start(self):
		assert False

	def get_cluster_size(self):
		assert False

	def is_cluster_free(self, cluster):
		return cluster == 0

	def is_cluster_terminating(self, cluster):
		return not 2 <= cluster < {12: 0xFF0, 16: 0xFFF0, 32: 0x0FFFFFF0}[self.get_fat_type()]

	def get_terminating_cluster(self):
		return {12: 0xFFF, 16: 0xFFFF, 32: 0x0FFFFFFF}[self.get_fat_type()]

	def get_cluster_count(self):
		fat_type = self.get_fat_type()
		if fat_type == 12:
			return self.get_fat_size() * 2 // 3
		elif fat_type == 16:
			return self.get_fat_size() // 2
		elif fat_type == 32:
			return self.get_fat_size() // 4
		else:
			raise Exception(f"Invalid fat type: {fat_type}")

	def read_next_cluster(self, cluster, fat_number = 0):
		fat_type = self.get_fat_type()
		if fat_type == 12:
			offset = cluster * 3 // 2
		elif fat_type == 16:
			offset = cluster * 2
		elif fat_type == 32:
			offset = cluster * 4
		else:
			raise Exception(f"Invalid fat type: {fat_type}")

		self.fp.seek(self.get_fat_start() + fat_number * self.get_fat_size() + offset)

		if fat_type == 32:
			value = struct.unpack('<I', self.fp.read(4))[0]
		else:
			value = struct.unpack('<H', self.fp.read(2))[0]
			if fat_type == 12:
				if cluster & 1 == 0:
					value &= 0xFFF
				else:
					value >>= 4

		return value

	def write_next_cluster(self, cluster, next_cluster):
		fat_type = self.get_fat_type()
		if fat_type == 12:
			offset = cluster * 3 // 2
		elif fat_type == 16:
			offset = cluster * 2
		elif fat_type == 32:
			offset = cluster * 4
		else:
			raise Exception(f"Invalid fat type: {fat_type}")

		for fat_number in range(self.get_fat_count()):
			self.fp.seek(self.get_fat_start() + fat_number * self.get_fat_size() + offset)

			if fat_type == 12:
				value = struct.unpack('<H', self.fp.read(2))[0]
				if cluster & 1 == 0:
					value = (value & ~0xFFF) | (next_cluster & 0xFFF)
				else:
					value = (value & 0xF) | (next_cluster << 4)
				self.fp.seek(-2, os.SEEK_CUR)
#				print("!", hex(value), hex(next_cluster))
				self.fp.write(struct.pack('<H', value))
			elif fat_type == 16:
				self.fp.write(struct.pack('<H', next_cluster))
			elif fat_type == 32:
				self.fp.write(struct.pack('<I', next_cluster))

#			print("~", cluster, next_cluster, self.read_next_cluster(cluster, fat_number))

	def allocate_clusters(self, count = 1, fat_number = 0):
		first_number = None
		consecutive = 0

		for number in range(self.get_cluster_count()):
			if self.is_cluster_free(self.read_next_cluster(number, fat_number = fat_number)):
				if first_number is None:
					first_number = number
				consecutive += 1
				if consecutive == count:
					break
			elif first_number is not None:
				first_number = None
				consecutive = 0

		if consecutive == count:
			return first_number
		else:
			return None

	def write_cluster_chain(self, first_cluster, *clusters):
		current_cluster = first_cluster
		for next_cluster in clusters:
			self.write_next_cluster(current_cluster, next_cluster)
			current_cluster = next_cluster
		self.write_next_cluster(current_cluster, self.get_terminating_cluster())

	def read_directory_entry(self):
		assert False

	def write_directory_entry(self, entry):
		assert False

	def read_cluster(self, cluster):
		self.fp.seek(self.get_clusters_start() + cluster * self.get_cluster_size())
		return self.fp.read(self.get_cluster_size())

	def write_cluster(self, cluster, cluster_data):
#		print(hex(cluster), hex(self.get_clusters_start() + cluster * self.get_cluster_size()))
		self.fp.seek(self.get_clusters_start() + cluster * self.get_cluster_size())
		if len(cluster_data) > self.get_cluster_size():
			cluster_data = cluster_data[:self.get_cluster_size()]
		self.fp.write(cluster_data)

	def read_clusters(self, cluster, fat_number = 0):
		read_clusters = set()
		data = b''
		while True:
			if cluster in read_clusters:
				# avoid circular files
				break # TODO: raise an exception
			data += self.read_cluster(cluster)
			read_clusters.add(cluster)
			cluster = self.read_next_cluster(cluster, fat_number = fat_number)
			if self.is_cluster_terminating(cluster):
				break
		return data

	def write_clusters(self, data, cluster):
		cluster_size = self.get_cluster_size()
		offset = 0
		while offset < len(data):
			if self.is_cluster_terminating(cluster):
				print("Internal error: run out of clusters", file = sys.stderr)
				break # TODO: raise an exception
			chunk = data[offset:offset + cluster_size]
			self.write_cluster(cluster, chunk)
			offset += cluster_size
			cluster = self.read_next_cluster(cluster)
		if not self.is_cluster_terminating(cluster):
			pass # TODO: raise an exception
		return data

	def add_directory_entry(self, file):
		# TODO: implement other allocation strategies? (last deleted file)
		self.fp.seek(self.get_root_start())
		entry_number = None
		for i in range(self.get_root_entry_count()):
			entry = self.read_directory_entry()
			if entry is None or entry.deleted:
				entry_number = i
				break
			if file.filename == entry.filename and file.extension == entry.extension:
				# TODO: mark as colliding?
				entry_number = i
				break
		if entry_number is None:
			# TODO: raise exception, directory is full
			return False
		self.fp.seek(self.get_root_start() + entry_number * self.get_root_entry_size())
		self.write_directory_entry(file)
		return True

	def load_file(self, file, mode = 'b'):
		data = self.read_clusters(file.first_cluster)
		if len(data) > file.file_size:
			# TODO: directories will have size 0
			data = data[:file.file_size]
		# TODO: what if it is less?
		if mode == 't':
			data = data.replace(b'\r\n', b'\n')
		return data

	def fill_file(self, data):
		return data

	def store_file(self, file, data, mode = 'b'):
		if mode == 't':
			data = data.replace(b'\n', b'\r\n')

		data = self.fill_file(data)

		file.file_size = len(data)
		# TODO: empty files?

		cluster_size = self.get_cluster_size()
		cluster_count = (file.file_size + cluster_size - 1) // cluster_size
		# attempt to allocate consecutive clusters
		first_cluster = self.allocate_clusters(cluster_count)
		if first_cluster is not None:
			clusters = [first_cluster + i for i in range(cluster_count)]
		else:
			# allocates them one by one (TODO: should it try to allocate longest?)
			clusters = []
			for i in range(cluster_count):
				cluster = self.allocate_clusters()
				if cluster is None:
					return False # out of clusters
				clusters.append(cluster)

		file.first_cluster = clusters[0]
		self.write_cluster_chain(*clusters)
		self.write_clusters(data, file.first_cluster)
		return True

	def list_files(self):
		self.fp.seek(self.get_root_start())
		for i in range(self.get_root_entry_count()):
			entry = self.read_directory_entry()
			if entry is None:
				return
			yield entry

	def split_filename(self, filename):
		if ':' in filename:
			# remove drive specification
			filename = filename[filename.index(':') + 1:]

		parts = filename.upper().split('.')

		filename = parts[0]
		extension = '' if len(parts) == 1 else parts[-1]

		if len(filename) > 8:
			filename = filename[:8]

		if len(extension) > 3:
			extension = extension[:3]

		return filename, extension

	def find_files(self, *specification):
		for spec in specification:
			filename, extension = self.split_filename(spec)

			filename_begin = '*' in filename
			if filename_begin:
				filename = filename[:filename.index('*')]

			extension_begin = '*' in extension
			if extension_begin:
				extension = extension[:extension.index('*')]

			found = False

			for entry in self.list_files():
				#print(entry.filename, filename, entry.extension, extension)
				if entry.deleted: # TODO: should be possible to search
					continue

				if filename_begin:
					if not entry.filename.startswith(filename):
						continue
				else:
					if entry.filename != filename:
						continue
				if filename_begin:
					if not entry.extension.startswith(extension):
						continue
				else:
					if entry.extension != extension:
						continue
				yield entry
				found = True

			if not found:
				print("File not found: " + filename + ("." + extension if extension != '' else ""), file = sys.stderr)

class FAT0(AbstractFAT):
	"""
	Early version of FAT used in pre-0.42 versions of 86-DOS, with a 16-byte directory entry.
	"""
	def get_fat_type(self):
		return 12

	def get_fat_count(self):
		return 2

	def get_fat_start(self):
		return 0x1A00

	def get_fat_size(self):
		return 0x300

	def get_root_start(self):
		return 0x2000

	def get_root_entry_count(self):
		return 64

	def get_root_entry_size(self):
		return 16

	def get_clusters_start(self):
		# including root directories
		return 0x2000

	def get_cluster_size(self):
		return 0x200

	def read_directory_entry(self):
		filename = self.fp.read(8)
		deleted = False
		if filename[0] == 0:
			return None
		elif filename[0] == 0xe5:
			filename = b'?' + filename[1:]
			deleted = True
		filename = filename.decode(self.encoding, errors = 'ignore').strip()
		extension = self.fp.read(3).decode(self.encoding, errors = 'ignore').strip()
		first_cluster = struct.unpack('<H', self.fp.read(2))[0]
		file_size = struct.unpack('<I', self.fp.read(3) + b'\0')[0]
		file = File(
			filename = filename,
			extension = extension,
			deleted = deleted,
			first_cluster = first_cluster,
			file_size = file_size
		)
		return file

	def write_directory_entry(self, entry):
		filename = entry.filename.encode(self.encoding, errors = 'ignore')
		if len(filename) < 8:
			filename += b' ' * (8 - len(filename))
		elif len(filename) > 8:
			filename = filename[:8]
		if entry.deleted:
			filename = b'\xe5' + filename[1:]
		self.fp.write(filename)

		extension = entry.extension.encode(self.encoding, errors = 'ignore')
		if len(extension) < 3:
			extension += b' ' * (3 - len(extension))
		elif len(extension) > 3:
			extension = extension[:3]
		self.fp.write(extension)

		self.fp.write(struct.pack('<H', entry.first_cluster))
		self.fp.write(struct.pack('<I', entry.file_size)[:3])

	def fill_file(self, data):
		if len(data) % self.get_cluster_size() != 0:
			data += b'\x1A' * (self.get_cluster_size() - len(data) % self.get_cluster_size())
		return data

class FAT1(FAT0):
	"""
	Version used since 86-DOS 0.42.
	"""

	def get_root_entry_size(self):
		return 32

	def get_clusters_start(self):
		# cluster 2 must start at 0x2800
		return 0x2400

	def read_extended_directory_entry(self, entry):
		self.fp.seek(0xC, os.SEEK_CUR)

	def read_directory_entry(self):
		filename = self.fp.read(8)
		deleted = False
		if filename[1] == 0:
			return None
		elif filename[0] == 0xe5:
			filename = b'?' + filename[1:]
			deleted = True
		filename = filename.decode(self.encoding, errors = 'ignore').strip()
		extension = self.fp.read(3).decode(self.encoding, errors = 'ignore').strip()
		attributes = self.fp.read(1)[0]
		attributes = set(1 << i for i in range(8) if attributes & (1 << i))
		entry = File(
			filename = filename,
			extension = extension,
			deleted = deleted,
			attributes = attributes
		)
		self.read_extended_directory_entry(entry)
		mod_date = struct.unpack('<H', self.fp.read(2))[0]
		entry.modification_date = (1980 + (mod_date >> 9), (mod_date >> 5) & 0xF, mod_date & 0x1F)
		entry.first_cluster = struct.unpack('<H', self.fp.read(2))[0]
		entry.file_size = struct.unpack('<I', self.fp.read(4))[0]
		return entry

	def write_extended_directory_entry(self, entry):
		self.fp.seek(0xC, os.SEEK_CUR)

	def write_directory_entry(self, entry):
		filename = entry.filename.encode(self.encoding, errors = 'ignore')
		if len(filename) < 8:
			filename += b' ' * (8 - len(filename))
		elif len(filename) > 8:
			filename = filename[:8]
		if entry.deleted:
			filename = b'\xe5' + filename[1:]
		self.fp.write(filename)

		extension = entry.extension.encode(self.encoding, errors = 'ignore')
		if len(extension) < 3:
			extension += b' ' * (3 - len(extension))
		elif len(extension) > 3:
			extension = extension[:3]
		self.fp.write(extension)

		attributes = sum(entry.attributes)
		self.fp.write(bytes([attributes]))

		self.write_extended_directory_entry(entry)

		y, m, d = entry.modification_date
		self.fp.write(struct.pack('<H', ((y - 1980) << 9) | (m << 5) | d))
		self.fp.write(struct.pack('<H', entry.first_cluster))
		self.fp.write(struct.pack('<I', entry.file_size))

class FAT(FAT1):
	"""
	Full implementation of FAT.
	"""
	def initialize(self):
		self.fp.seek(0xB)
		self.sector_size = struct.unpack('<H', self.fp.read(2))[0]
#		print("Sector size:", self.sector_size)
		self.sector_per_cluster = self.fp.read(1)[0]
#		print("Sector per cluster:", self.sector_per_cluster)
		self.reserved_sectors = struct.unpack('<H', self.fp.read(2))[0]
		self.fat_count = self.fp.read(1)[0]
		self.root_entry_count = struct.unpack('<H', self.fp.read(2))[0]
		self.sector_count = struct.unpack('<H', self.fp.read(2))[0]
		self.media_descriptor = self.fp.read(1)[0]
		self.sector_per_fat = struct.unpack('<H', self.fp.read(2))[0]

	def get_fat_type(self):
		# TODO
		return 12

	def get_fat_count(self):
		return self.fat_count

	def get_fat_start(self):
		return self.reserved_sectors * self.sector_size

	def get_fat_size(self):
		return self.sector_per_fat * self.sector_size

	def get_root_start(self):
		return self.get_fat_start() + self.get_fat_size() * self.get_fat_count()

	def get_root_entry_count(self):
		return self.root_entry_count

	def get_root_entry_size(self):
		return 32

	def get_clusters_start(self):
		# including root directories
		return self.get_root_start() + self.get_root_entry_size() * self.get_root_entry_count() - 2 * self.get_cluster_size()

	def get_cluster_size(self):
		return self.sector_size * self.sector_per_cluster

	def read_extended_directory_entry(self, entry):
		self.fp.seek(0xA, os.SEEK_CUR)
		mod_time = struct.unpack('<H', self.fp.read(2))[0]
		entry.modification_time = (mod_time >> 11, (mod_time >> 5) & 0x3F, (mod_time & 0x1F) << 1)

	def write_extended_directory_entry(self, entry):
		self.fp.seek(0xA, os.SEEK_CUR)
		h, m, s = entry.modification_time
		self.fp.write(struct.pack('<H', (h << 11) | (m << 5) | (s >> 1)))

	def read_directory_entry(self):
		entry = super(FAT, self).read_directory_entry()
		if entry is not None and entry.filename[0] == '\0':
			return None
		return entry

def main():
	image_filename = None
	disk_format = None
	command = None
	encoding = None

	i = 1
	while i < len(sys.argv):
		if sys.argv[i].startswith('-i'):
			arg = sys.argv[i][len('-i'):]
			if arg == '':
				if i + 1 < len(sys.argv):
					i += 1
					arg = sys.argv[i]
				else:
					print("Missing argument to -i", file = sys.stderr)
					exit(1)
			image_filename = arg
		elif sys.argv[i].startswith('-f'):
			arg = sys.argv[i][len('-f'):]
			if arg == '':
				if i + 1 < len(sys.argv):
					i += 1
					arg = sys.argv[i]
				else:
					print("Missing argument to -f", file = sys.stderr)
					exit(1)
			disk_format = arg
		elif sys.argv[i].startswith('-e'):
			arg = sys.argv[i][len('-e'):]
			if arg == '':
				if i + 1 < len(sys.argv):
					i += 1
					arg = sys.argv[i]
				else:
					print("Missing argument to -e", file = sys.stderr)
					exit(1)
			encoding = arg
		elif sys.argv[i].startswith('-'):
			print(f"Unknown flag {sys.argv[i]}", file = sys.stderr)
			exit(1)
		else:
			command = sys.argv[i]
			i += 1
			args = sys.argv[i:]
			break
		i += 1

	disk_class = {
		None: FAT,

		'qdos': FAT0,
		'86dos': FAT0,
		'qdos0.10': FAT0,
		'qdos0.1': FAT0,
		'qdos010': FAT0,
		'qdos01': FAT0,
		'86dos0.10': FAT0,
		'86dos0.1': FAT0,
		'86dos010': FAT0,
		'86dos01': FAT0,
		'0.10': FAT0,
		'0.1': FAT0,
		'010': FAT0,
		'01': FAT0,
		'qdos0.34': FAT0,
		'qdos0.3': FAT0,
		'qdos034': FAT0,
		'qdos03': FAT0,
		'86dos0.34': FAT0,
		'86dos0.3': FAT0,
		'86dos034': FAT0,
		'86dos03': FAT0,
		'0.10': FAT0,
		'0.1': FAT0,
		'010': FAT0,
		'01': FAT0,
		'qdos0': FAT0,
		'86dos0': FAT0,
		'0': FAT0,

		'86dos0.42': FAT1,
		'86dos042': FAT1,
		'86dos0.4': FAT1,
		'86dos04': FAT1,
		'0.42': FAT1,
		'042': FAT1,
		'0.4': FAT1,
		'04': FAT1,
		'86dos1.14': FAT1,
		'86dos114': FAT1,
		'86dos1.1': FAT1,
		'86dos11': FAT1,
		'86dos1': FAT1,
		'1.14': FAT1,
		'114': FAT1,
		'1.1': FAT1,
		'11': FAT1,
		'1': FAT1,
	}.get(disk_format)

	if (disk_class) is None:
		print("Unknown format: " + disk_format, file = sys.stderr)
		exit(1)

	if encoding is not None:
		disk_class.encoding = encoding

	if image_filename is None:
		print("Unspecified image name", file = sys.stderr)
		exit(1)
	disk = disk_class(open(image_filename, 'r+b'))

	if command in {'ls', 'dir'}:
		specification = []
		i = 0
		while i < len(args):
			if args[i].startswith('-'):
				print(f"Unknown flag {args[i]}", file = sys.stderr)
				exit(1)
			else:
				specification.append(args[i])
			i += 1

		for file in disk.find_files(*specification) if len(specification) != 0 else disk.list_files():
			a = ''.join(
				[
					'd' if file.deleted else ' ',
					'r' if File.ATTR_READONLY in file.attributes else ' ',
					'h' if File.ATTR_HIDDEN in file.attributes else ' ',
					's' if File.ATTR_SYSTEM in file.attributes else ' ',
					'V' if File.ATTR_VOLUME in file.attributes else ' ',
					'D' if File.ATTR_DIRECTORY in file.attributes else ' ',
					'a' if File.ATTR_ARCHIVE in file.attributes else ' ',
					'd' if File.ATTR_DEVICE in file.attributes else ' ',
					'?' if 0x80 in file.attributes else ' ',
				])
			if file.deleted:
				print("\33[9m", end = '')
			print(f"{file.filename:8} {file.extension:3}", end = '')
			if file.deleted:
				print("\33[m", end = '')
			print(f" {a} {file.file_size:10} B", end = '')
			if file.modification_date != (1980, 1, 1):
				print(f" {file.modification_date[0]:04}-{file.modification_date[1]:02}-{file.modification_date[2]:02}", end = '')
			if file.modification_time != (0, 0, 0):
				print(f" {file.modification_time[0]:02}:{file.modification_time[1]:02}:{file.modification_time[2]:02}", end = '')
			print()

	elif command in {'cat', 'type'}:
		filenames = []
		mode = None
		i = 0
		while i < len(args):
			if args[i].startswith('-t'):
				mode = 't'
			elif args[i].startswith('-b'):
				mode = 'b'
			elif args[i].startswith('-'):
				print(f"Unknown flag {args[i]}", file = sys.stderr)
				exit(1)
			else:
				filenames.append((mode, args[i]))
			i += 1

		#print(filenames)

		for mode, filename in filenames:
			for file in disk.find_files(filename):

			#if file is None:
			#	print("File missing: " + filename + ("." + extension if extension != '' else ""), file = sys.stderr)
			#	continue

				data = disk.load_file(file, mode = mode)

				sys.stdout.buffer.write(data)

	elif command in {'cp', 'copy'}:
		source = []
		target = None
		i = 0
		mode = None
		while i < len(args):
			if args[i].startswith('-t'):
				mode = 't'
			elif args[i].startswith('-b'):
				mode = 'b'
			elif args[i].startswith('-'):
				print(f"Unknown flag {args[i]}", file = sys.stderr)
				exit(1)
			elif len(source) == 0:
				source.append(args[i])
			elif target is None:
				target = args[i]
			else:
				source.append(target)
				target = args[i]
			i += 1

#		print(source, target)

		if len(source) == 0:
			print("No sources provided", file = sys.stderr)

		# TODO: preserve date and flags
		if ':' in source[0]:
			# load
			files = list(disk.find_files(*source))
			if len(files) != 1 or target is None or os.path.isdir(target):
				# directory
				for file in files:
					filename = (file.filename + ('.' + file.extension if file.extension != '' else '')).lower()
					target_filename = os.path.join(target, filename) if target is not None else filename
					with open(target_filename, 'wb') as target_file:
						target_file.write(disk.load_file(file, mode = mode))
			else:
				# file
				with open(target, 'wb') as target_file:
					target_file.write(disk.load_file(files[0], mode = mode))

		else:
			# store
			if len(source) == 1:
				# file
				# TODO: what if it is a directory
				if target is None or target.endswith(':') or target.endswith(':.'):
					target = os.path.split(source[0])[1]
				filename, extension = disk.split_filename(target)
				file = File(
					filename = filename,
					extension = extension)
				with open(source[0], 'rb') as fp:
					if not disk.store_file(file, fp.read(), mode = mode):
						print("Unable to store file", file = sys.stderr)
						exit(1)
				disk.add_directory_entry(file)
			else:
				# directory
				# TODO: do not ignore
				print("Unimplemented", file = sys.stderr)
				pass

	else:
		print("Unknown command", file = sys.stderr)

if __name__ == '__main__':
	main()

