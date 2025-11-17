-- kalimba / piano demo
-- written by maple "mavica" syrup <maple@maple.pet>

local pkeys = {}

local keymap = {
	"5","6","4","7","3","8","2","9",
	"t","y","r","u","e","i","w","o",
	"g","h","f","j","d","k","s","l",
	"b","n","v","m","c",",","x","."
}

local qwertymap = {
	"2","3","4","5","6","7","8","9",
	"w","e","r","t","y","u","i","o",
	"s","d","f","g","h","j","k","l",
	"x","c","v","b","n","m",",","."
}

for k,v in pairs(keymap) do
	pkeys[v] = k - 1
end

local last = nil
local ch = 0
local octave = 3

local inst_num = 0
local insts = {}
local inst_names = {}

for k,v in pairs(sound.presets) do
	table.insert(insts, v)
	table.insert(inst_names, k)
end

local function noteString(note)
	local notes = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"}
	return notes[(note % 12)+1] .. math.floor(note / 12)
end

local function redraw()
	term.setCursorPos(1,1)
	term.write("Piano / Kalimba demo\n\n")
	for j = 0, 24, 8 do
		for i = 1, 8 do
			if qwertymap[i+j] == last then term.write("\27[7m") end
			term.write(qwertymap[i+j].."\27[m ")
		end
		term.write("\n")
	end
	term.write("\n")
	if pkeys[last] then term.write(noteString(pkeys[last] + octave * 12)) end
	term.write("\n\n")
	term.write("Octave: " .. octave .. " | Instrument: " .. inst_names[inst_num+1] .. "\n")
	term.write("/ \\ - Change octave\n[ ] - Change instrument\nEsc - Quit\n")
end

term.clear()
while true do
	redraw()

	local state, _, code = keys.wait(false, false)

	if state == keys.states.pressed and code ~= last then
		if pkeys[code] then
			last = code
			sound.play(ch, pkeys[code] + octave * 12, insts[inst_num+1])
			ch = (ch+1) % 4
		elseif code == "[" then
			inst_num = (inst_num - 1) % #insts
		elseif code == "]" then
			inst_num = (inst_num + 1) % #insts
		elseif code == "/" and octave > 0 then
			octave = (octave - 1)
		elseif code == "\\" and octave < 6 then
			octave = (octave + 1)
		elseif code == keys.esc then
			break
		end
	elseif state == keys.states.released then
		last = nil
	end
end
