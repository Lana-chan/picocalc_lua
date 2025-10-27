def make_c_array(name, data):
	s = dict(sorted(data.items()))
	output = f"{name}[] = {{\n"
	line = "\t"
	for v in s:
		line += f"{s[v]}, "
		if len(line) >= 80:
			output += line[:-1] + "\n"
			line = "\t"
	output += line[:-2] + "\n};"
	return output

to8 = {}
for b in range(32):
	for g in range(64):
		for r in range(32):
			r8 = r >> 3
			g8 = g >> 3
			b8 = b >> 2
			to8[r << 11 | g << 5 | b] = r8 << 6 | g8 << 3 | b8


to16 = {}
for c8 in range(256):
	r = int((c8 >> 6) / 3 * 31)
	g = int(((c8 >> 3) & 0b111) / 7 * 63)
	b = int((c8 & 0b111) / 7 * 31)
	to16[c8] = r << 11 | g << 5 | b

with open("lcd_lut.h", "w") as f:
	f.write("#pragma once\n\n")
	f.write(make_c_array("const uint16_t lcd_to16", to16))
	f.write("\n\n")
	f.write(make_c_array("const uint8_t lcd_to8", to8))

# rrrrrggggggbbbbb
# rrgggbbb