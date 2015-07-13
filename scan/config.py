# copyright 2015 by mike lodato (zvxryb@gmail.com)
# this work is subject to the terms of the MIT license

from contextlib import contextmanager
import scan.native.capture as native_capture
import fractions
import os
import pyglet
import stat
import struct
import yaml

try:
	input = raw_input
except NameError:
	pass

@contextmanager
def descriptor(path, flags):
	fd = os.open(path, flags)
	yield fd
	os.close(fd)

def user_input(prompt):
	def get_value(convert):
		value = None
		while value is None:
			try:
				value = convert(input(prompt))
			except ValueError:
				pass
		return value
	return get_value

def config(f):
	display = pyglet.canvas.get_display()
	screens = display.get_screens()
	print('\nscreens:')
	for i, screen in enumerate(screens):
		print('\t%d: (%d, %d) %dx%d' % (i, screen.x, screen.y,
			screen.width, screen.height))
	
	@user_input('choose a screen: ')
	def screen_index(x):
		y = int(x)
		if y < 0 or y >= len(screens):
			raise ValueError('invalid screen index')
		return y
	
	print('\ncameras:')
	for i in range(10):
		path = '/dev/video%d' % i
		if not os.path.exists(path):
			continue
		mode = os.stat(path).st_mode
		if not stat.S_ISCHR(mode):
			continue
		with descriptor(path, os.O_RDONLY) as fd:
			info = native_capture.query(fd)
		if not info.capture:
			continue
		print('\t%s: %s' % (path, str(info)))
	@user_input('choose a camera: ')
	def camera_path(x):
		if not os.path.exists(x):
			return None
		return x
	
	with descriptor(camera_path, os.O_RDONLY) as fd:
		print('\ncamera formats:')
		formats = native_capture.formats(fd)
		for i, fmt in enumerate(formats):
			line = '\t%d: %s' % (i, fmt.description)
			if (fmt.compressed):
				line += ' (compressed)'
			if (fmt.emulated):
				line += ' (emulated)'
			print(line)
		@user_input('preferred capture format: ')
		def camera_format(x):
			y = int(x)
			if y < 0 or y >= len(formats):
				raise ValueError('invalid format index')
			return formats[y].fourcc
	
		print('\ncamera frame sizes:')
		frame_sizes = native_capture.frame_sizes(fd, camera_format)
		for i, size in enumerate(frame_sizes):
			w = size.width
			h = size.height
			gcd = fractions.gcd(w, h)
			print('\t%d: %-9s (%d:%d)' % (i, '%dx%d' % (w, h), w//gcd, h//gcd))
		@user_input('preferred camera frame size: ')
		def camera_frame_size(x):
			y = int(x)
			if y < 0 or y >= len(frame_sizes):
				raise ValueError('invalid frame size index')
			size = frame_sizes[y]
			return (size.width, size.height)
	
		print('\ncamera frame intervals:')
		intervals = native_capture.intervals(fd, camera_format,
			camera_frame_size[0], camera_frame_size[1])
		for i, interval in enumerate(intervals):
			print('\t%d: %d/%d' % (i, interval.numerator, interval.denominator))
		@user_input('preferred camera frame interval: ')
		def camera_frame_interval(x):
			y = int(x)
			if y < 0 or y >= len(intervals):
				raise ValueError('invalid interval index')
			interval = intervals[y]
			return (interval.numerator, interval.denominator)
	
	@user_input('\nframe capture delay (seconds): ')
	def delay(x):
		return float(x)
	
	config = {
		'screen': {
			'index': screen_index
		},
		'camera': {
			'path': camera_path,
			'format': camera_format,
			'frameSize': camera_frame_size,
			'interval': camera_frame_interval
		},
		'delay': delay
	}
	data = yaml.safe_dump(config)
	f.write(bytes(data, 'utf-8'))

def parse(f):
	return yaml.safe_load(f)

