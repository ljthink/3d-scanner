# copyright 2015 by mike lodato (zvxryb@gmail.com)
# this work is subject to the terms of the MIT license

import math
import pyglet

class GrayCode(pyglet.window.Window):
	def __init__(self, *args, **kwargs):
		super(GrayCode, self).__init__(*args, **kwargs)
		
		y0 = 0
		y1 = self.height
		
		self.i = 0
		self.frames = []
		n = int(math.ceil(math.log(self.width, 2)))
		for i in range(n):
			indices  = []
			vertices = []
			colors   = []
			m  = 0
			dx = 2 ** i
			x0 = 0
			for j in range(self.width // dx + 1):
				x1 = x0 + dx
				if (j ^ (j >> 1)) & 1 > 0:
					indices  += (x + m for x in [0, 1, 2, 2, 3, 0])
					vertices += (x0, y0, x1, y0, x1, y1, x0, y1)
					colors   += 12 * (255,)
					m += 4
				x0 = x1
			self.frames.append(pyglet.graphics.vertex_list_indexed(
				m, indices, ('v2i', vertices), ('c3B', colors)))

	def on_key_press(self, symbol, modifiers):
		if symbol == pyglet.window.key.ESCAPE:
			pyglet.app.exit()

	def on_draw(self):
		pyglet.gl.glClearColor(1.0, 0.0, 0.0, 1.0)
		self.clear()
		self.i -= 1
		if self.i < 0:
			self.i = len(self.frames) - 1
		self.frames[self.i].draw(pyglet.gl.GL_TRIANGLES)

