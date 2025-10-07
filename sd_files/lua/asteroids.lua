local poly_ship = {{0, -10}, {5, 5}, {0, 0}, {-5, 5}, {0, -10}}
local poly_thrust = {{-2.5, 2,5}, {0, 4}, {2.5, 2.5}}
local poly_shot = {{0, 0}, {0, -5}}
local poly_rock = {{-1,0},{-0.8,-0.6},{-0.4,-0.5},
{0,-1},{0.5,-0.9},{0.3,-0.4},{0.8,-0.5},
{1,0},{0.7,0.4},{0.7,0.6},{0.3,0.8},{0,0.7},
{-0.3,0.9},{-0.7,0.8},{-0.7,0.5},{-1,0}}

local bg = colors.fromRGB(0,0,0)
local fg = colors.fromRGB(0,255,0)
local width = 320
local height = 320

local max_shots = 5
local shot_speed = 5
local rock_speed = 0.6
local rock_size = 30
local rock_step = 10

local player = {
	x=width/2,
	y=height/2, 
	accel = 0.06,
	vx=0,
	vy=0,
	ang=0,
	cdown=0,
	inv=0
}

local shots = {}
local rocks = {}
local score = 0
local lives = 3

local last_frame = 0
local game_over = false

function sign(number)
	return (number > 0 and 1) or (number == 0 and 0) or -1
end

function distance(x1, y1, x2, y2)
	return math.sqrt((x2-x1)^2+(y2-y1)^2)
end

function draw_rotated(shape, x, y, ang, scale, color)
	processed = {}

	local sin = math.sin(ang) * scale
	local cos = math.cos(ang) * scale

	for _,v in pairs(shape) do
		local px = x + v[1] * cos - v[2] * sin
		local py = y + v[1] * sin + v[2] * cos
		table.insert(processed,math.floor(px+0.5))
		table.insert(processed,math.floor(py+0.5))
	end

	draw.polygon(processed, color) 
end

function level_init()
	for i=1,4 do
		rock_create(math.random(30,width-30), math.random(30,height-30), math.random()*6.28, 30)
	end
	player.inv = 50
end

function player:fire()
	if #shots < max_shots then
		shot = {
			x = self.x,
			y = self.y,
			ang = self.ang,
			vx = math.sin(self.ang) * shot_speed,
			vy = -math.cos(self.ang) * shot_speed,
		}
		table.insert(shots, shot)
		self.cooldown = 25
	end
end

function player:erase()
	draw_rotated(poly_ship, self.x, self.y, self.ang, 1, bg)
	draw_rotated(poly_thrust, self.x, self.y, self.ang, 1, bg)
end

function player:update()
	local angle_d = (keys.getState(keys.right) and 0.1 or 0) - (keys.getState(keys.left) and 0.1 or 0)

	self.ang = self.ang + angle_d
	if keys.getState(keys.up) then
		self.vx = (self.vx + math.sin(self.ang) * self.accel)
		self.vy = (self.vy - math.cos(self.ang) * self.accel)
	elseif keys.getState(keys.down) then
		self.vx = (self.vx - sign(self.vx) * self.accel)
		self.vy = (self.vy - sign(self.vy) * self.accel)
	end
	self.x = (self.x + self.vx) % width
	self.y = (self.y + self.vy) % height
	
	if self.cdown > 0 then
		self.cdown = self.cdown - 1
	else
		state, _, code = keys.poll()
		if state == keys.states.pressed and code == keys.enter then
			self:fire()
		end
	end
	
	if self.inv > 0 then self.inv = self.inv - 1 end
end

function player:hit()
	if lives == 0 then game_over = true return end
	lives = lives - 1
	self.x = width/2
	self.y = height/2
	self.vx = 0
	self.vy = 0
	self.ang = 0
	self.inv = 50
end

function player:draw()
	if math.floor(self.inv / 3) % 2 == 0 then
		draw_rotated(poly_ship, self.x, self.y, self.ang, 1, fg)
	end
	if keys.getState(keys.up) then
		draw_rotated(poly_thrust, self.x, self.y, self.ang, 1, fg)
	end
end

function shot_erase(self)
	draw_rotated(poly_shot, self.x, self.y, self.ang, 1, bg)
end

function shot_update(self, k)
	self.x = self.x + self.vx
	self.y = self.y + self.vy
	
	if self.x > width or self.x < 0 or self.y > height or self.y < 0 then
		shot_erase(self)
		table.remove(shots,k)
	else
		for rk,v in pairs(rocks) do
			if distance(self.x, self.y, v.x, v.y) < v.scale then
				rock_hit(v,rk,self,k)
			end
		end
	end
end

function shot_draw(self)
	draw_rotated(poly_shot, self.x, self.y, self.ang, 1, fg)
end

function rock_erase(self)
	draw_rotated(poly_rock, self.x, self.y, self.ang, self.scale, bg)
end

function rock_create(x, y, ang, size)
	rock = {
		x = x,
		y = y,
		ang = ang,
		vx = math.sin(ang) * rock_speed,
		vy = -math.cos(ang) * rock_speed,
		scale = size
	}
	table.insert(rocks, rock)
end

function rock_update(self, k)
	self.x = (self.x + self.vx) % width
	self.y = (self.y + self.vy) % height
	
	if player.inv == 0 and distance(self.x, self.y, player.x, player.y) < self.scale then
		player:hit()
	end
end

function rock_hit(self, k, shot, sk)
	rock_erase(self)
	score = score + ((rock_size + rock_step) - self.scale)
	rock_speed = rock_speed + 0.06
	if self.scale > rock_step then
		rock_create(self.x, self.y, shot.ang - math.random()*3.14, self.scale-rock_step)
		rock_create(self.x, self.y, shot.ang + math.random()*3.14, self.scale-rock_step)
	end
	table.remove(rocks, k)
	table.remove(shots, sk)
	if #rocks == 0 then
		rock_speed = rock_speed - 0.6
		score = score + 500
		level_init()
	end
end

function rock_draw(self)
	draw_rotated(poly_rock, self.x, self.y, self.ang, self.scale, fg)
end

function hud_draw()
	draw.text(15,15,fg,bg,"       ")
	draw.text(15,15,fg,bg,score) 
	for i = 0, 3 do
		local color = i < lives and fg or bg
		draw_rotated(poly_ship, width - 20 - 20*i, 20, 0, 1, color)
	end
end

--draw.enableBuffer(true)
draw.rectFill(0, 0, width, height, bg)
level_init()

while true do
	if keys.getState(keys.esc) or game_over then break end
	
	local now = os.clock()
	if now >= last_frame + 0.04 then
		last_frame = now
		
		-- erase
		player:erase()
		for _,v in pairs(shots) do shot_erase(v) end
		for _,v in pairs(rocks) do rock_erase(v) end
		
		-- move player
		player:update()
		for k,v in pairs(shots) do shot_update(v,k) end
		for k,v in pairs(rocks) do rock_update(v,k) end
		
		--draw.clear()
		hud_draw()
		player:draw()
		for _,v in pairs(shots) do shot_draw(v) end
		for _,v in pairs(rocks) do rock_draw(v) end
		--draw.blitBuffer()
		
		collectgarbage()
	end
end

draw.enableBuffer(false)
if game_over then
	term.clear()
	print("game over!")
	print("final score: "..score)
end
