local w = 320
local w2 = w/2
local rad = 70
local n = 50
local nc = 100
local step=1
local r = math.pi*2/n
local x,y,t = 0,0,0
local u,v,px,py

draw.enableBuffer(true)
while true do
	if keys.getState(keys.esc) then break end
	draw.clear()
	for i = 0, n-1 do
		for c = 0, nc-1, step do
			u = math.sin(i+y)+math.sin(r*i+x)
			v = math.cos(i+y)+math.cos(r*i+x)
			x = u + t
			y = v
			px = u * rad + w2
			py = y * rad + w2
			draw.point(px, py, 
				colors.fromRGB(math.floor(63+i/n*192),
					math.floor(63+c/nc*192),168))
		end
	end
	t=t+0.03
	draw.blitBuffer()
end
draw.enableBuffer(false)
