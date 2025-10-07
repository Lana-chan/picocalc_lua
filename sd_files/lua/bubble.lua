local w = 320
local n = 150
local step=2
local r = math.pi*2/n
local x,y,t = 0,0,0

draw.enableBuffer(true)
while true do
	if keys.getState(keys.esc) then break end
	draw.clear()
	for i = 0, n-1, step do
		for c = 0, n-1, step do
			local u = math.sin(i+y)+math.sin(r*i+x)
			local v = math.cos(i+y)+math.cos(r*i+x)
			x = u + t
			y = v
			draw.point(u*n/2 + w/2, y*n/2 + w/2, 
				colors.fromRGB(math.floor(i/n*255),
					math.floor(c/n*255),168))
		end
	end
	t=t+0.1
	draw.blitBuffer()
end
draw.enableBuffer(false)
