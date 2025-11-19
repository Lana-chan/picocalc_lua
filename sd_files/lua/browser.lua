local current_dir = "/"
local cursor_y = 1
local width, height = term.getSize()

function string:endswith(suffix)
	return self:sub(-#suffix) == suffix
end

term.clear()
while true do
	local dir = fs.list(current_dir)
	term.setCursorPos(1,1)

	table.sort(dir, function(a, b) return string.upper(a.name) < string.upper(b.name) end)
	local folders = {}
	local files = {}
	for _,v in pairs(dir) do
		if v.isDir then table.insert(folders, v)
		else table.insert(files, v) end
	end
	table.sort(files, function(a, b) return a.name < b.name end)
	dir = folders
	for _,v in ipairs(files) do
		table.insert(dir, v)
	end

	if current_dir > "/" then
		table.insert(dir, 1, {name="..", isDir=true})
	end
	
	local off = 0
	if #dir > height-2 then
		off = math.floor(cursor_y / #dir * (#dir - height + 3))
	end
	
	for y = 1, height - 2 do
		local yoff = y + off
		local v = dir[yoff]
		if v then
			local str = ""
			if v.isDir then str = str .. "\27[93m"
			elseif v.name:endswith(".lua") then str = str .. "\27[96m"
			else str = str .. "\27[97m" end
			if yoff == cursor_y then str = str .. "\27[7m" end
			term.write(str .. v.name .. "\27[m\27[K\n")
		else
			term.write("\27[K\n")
		end
	end

	term.write("\n\27[93mCurrent directory: " .. current_dir .. "\27[m\27[K")

	local state, _, code = keys.wait(true, true)

	if code == keys.up then
		cursor_y = ((cursor_y - 2) % #dir) + 1
	elseif code == keys.down then
		cursor_y = ((cursor_y) % #dir) + 1
	elseif code == keys.enter then
		if dir[cursor_y].isDir then
			if dir[cursor_y].name == ".." then
				current_dir = current_dir:match("(.*/).*/") or '/'
			else
				current_dir = current_dir .. dir[cursor_y].name .. '/'
			end
			cursor_y = 1
		elseif dir[cursor_y].name:endswith(".lua") then
			edit(current_dir .. dir[cursor_y].name)
		end
	elseif code == keys.esc then
		break
	end
end
term.clear()