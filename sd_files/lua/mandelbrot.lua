-- adapted from https://rosettacode.org/wiki/Mandelbrot_set#Lua

local maxIterations = 10
local minX, maxX, minY, maxY = -2.5, 2.5, -2.5, 2.5
local miX, mxX, miY, mxY
local wid, hei = 320, 320
function remap( x, t1, t2, s1, s2 )
	local f = ( x - t1 ) / ( t2 - t1 )
	local g = f * ( s2 - s1 ) + s1
	return g;
end
function drawMandelbrot()
	local pts, a, as, za, b, bs, zb, cnt, clr = {}
	for j = 0, hei - 1 do
		for i = 0, wid - 1 do
			a = remap( i, 0, wid, minX, maxX )
			b = remap( j, 0, hei, minY, maxY )
			cnt = 0; za = a; zb = b
			while( cnt < maxIterations ) do
				as = a * a - b * b; bs = 2 * a * b
				a = za + as; b = zb + bs
				if math.abs( a ) + math.abs( b ) > 16 then break end
				cnt = cnt + 1
			end
			if cnt == maxIterations then clr = 0
			else clr = math.floor(remap( cnt, 0, maxIterations, 0, 255 ))
			end
			draw.point(i, j, colors.fromRGB(clr,clr,clr))
		end
	end
end

drawMandelbrot()
