-- partially adapted from https://bisqwit.iki.fi/jutut/kuvat/programming_examples/mandelbrotbtrace.pdf
-- arrow keys, 9 and 0 to control view, escape to quit

local maxIter = 64
local chunk = 8
local center = {-1,0}
local radius = 1
local minX, maxX, minY, maxY
local wid, hei = 320, 320

local function iterate(zr, zi, max)
	local cnt, r, i, r2, i2, ri
	cnt = 0
	r,i = zr,zi
	while cnt < max do
		r2 = r*r; i2 = i*i
		if r2+i2 >= 4 then break end
		ri = r*i
		i = ri+ri + zi
		r = r2-i2 + zr
		cnt = cnt + 1
	end
	return cnt
end

local function is_control_key()
	local state, _, code = keys.peek()
	if state == keys.states.pressed then
		if code == keys.up
			or code == keys.down
			or code == keys.left
			or code == keys.right
			or code == '9'
			or code == '0'
			or code == '['
			or code == ']'
			or code == keys.esc then
				return true
		end
	end
	keys.flush()
	return false
end

local function drawScanlineMandelbrot(max)
	local zr, zi, cnt, clr
	local st = chunk
	local proc = {}
	local stepR = (maxX - minX) / wid
	local stepI = (maxY - minY) / hei
	while st >= 1 do
		for y = 0, hei - 1, st do
			zi = minY + y * stepI
			if proc[(y % (st*2))] then goto next end
			for x = 0, wid - 1 do
				zr = minX + x * stepR
				cnt = iterate(zr, zi, max)
				if cnt == max then clr = 0
				else
					cnt = math.floor(cnt/max*255)
					clr = colors.fromHSV((-cnt-32)%256,255,127+math.floor(cnt/2))
				end
				draw.line(x, y, x, y+st-1, clr)
				if is_control_key() then return end
			end
			::next::
		end
		proc[st%chunk] = true
		st = math.floor(st/2)
	end
end

while true do
	minX, maxX, minY, maxY = center[1]-radius, center[1]+radius, center[2]-radius, center[2]+radius
	drawScanlineMandelbrot(maxIter)
	while true do
		local _, _, code = keys.wait(false, true)
		if code == keys.up then
			center[2] = center[2] - radius * 0.25
			break
		elseif code == keys.down then
			center[2] = center[2] + radius * 0.25
			break
		elseif code == keys.left then
			center[1] = center[1] - radius * 0.25
			break
		elseif code == keys.right then
			center[1] = center[1] + radius * 0.25
			break
		elseif code == '9' then
			radius = radius * 4
			break
		elseif code == '0' then
			radius = radius * 0.25
			break
		elseif code == '[' then
			maxIter = maxIter - 10
			break
		elseif code == ']' then
			maxIter = maxIter + 10
			break
		elseif code == keys.esc then
			goto exit
		end
	end
end
::exit::
