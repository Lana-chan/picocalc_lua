-- Bubble Universe
-- inspired by https://forum.clockworkpi.com/t/bubble-universe/19907

local w = 320
local w2 = w/2
local rad = 70
local n = 20
local nc = 120
local step = 2
local r = math.pi*2/n
local x,y,t = 0,0,6
local u,v,px,py

local res = 100

local sint = {}
for i = 1, math.floor(math.pi*2*res) do
	sint[i] = math.sin(i/res)
end

local function fsin(i)
	return sint[(math.floor(i*res)%#sint)+1]
end

local function fcos(i)
	return sint[((math.floor((math.pi/2-i)*res))%#sint)+1]
end

draw.enableBuffer(2)
while true do
	if keys.getState(keys.esc) then break end
	draw.clear()
	for i = 0, n-1 do
		for c = 0, nc-1, step do
			u = fsin(i+y)+fsin(r*i+x)
			v = fcos(i+y)+fcos(r*i+x)
			x = u + t
			y = v
			px = u * rad + w2
			py = y * rad + w2
			draw.point(px, py,
			--draw.rectFill(px, py, 2, 2,
				colors.fromRGB(math.floor(63+i/n*192),
					math.floor(63+c/nc*192),168))
		end
	end
	t=t+0.02
	draw.blitBuffer()
end
draw.enableBuffer(false)
