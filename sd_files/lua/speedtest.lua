-- drawing functions speed test

local function randomCircle()
	local x = math.random(0,319)
	local y = math.random(0,319)
	local r = math.random(20,50)
	local c = colors.fromHSV(math.random(0,255),255,255)
	draw.circleFill(x,y,r,c)
end

local sprites = draw.loadBMPSprites("lua/boxworld.bmp", 20, 20)

local function randomSprite()
	local x = math.random(-29, 319)
	local y = math.random(-29, 319)
	local id = math.random(0,13)
	sprites:blit(x,y,id)
end

local start, dur0, dur1, dur2
local testDraw = randomCircle
--local testDraw = randomSprite

draw.clear()
start = os.clock()
for i=1,1000 do
	testDraw()
end
dur0 = os.clock()-start

draw.enableBuffer(true)
draw.clear()
start = os.clock()
for i=1,1000 do
	testDraw()
end
draw.blitBuffer()
dur1 = os.clock()-start
draw.enableBuffer(false)

draw.enableBuffer(2)
draw.clear()
start = os.clock()
for i=1,1000 do
	testDraw()
end
draw.blitBuffer()
dur2 = os.clock()-start
draw.enableBuffer(false)

print("direct: "..dur0)
print("psram: "..dur1)
print("ram: "..dur2)
