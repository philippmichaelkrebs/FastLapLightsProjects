# python color to display
# 16-bit 5-6-5 Color to draw text with
import colorsys

def scale(value, bits):
	# def scale_value(value, in_min, in_max, out_min, out_max):
	in_min = 0;
	in_max = 2**8 - 1
	out_min = 0;
	out_max = (2**bits) - 1
	if in_max == in_min:
		raise ValueError("Input min and max cannot be equal")
	return out_min + (value - in_min) * (out_max - out_min) / (in_max - in_min)

def rgb_to_hub75(r:int,g:int,b:int) -> int:
	'r,g,b in [0,255]'
	rgbhub75 = int(scale(r,5)) << 11
	rgbhub75 |= int(scale(b,6)) << 5
	rgbhub75 |= int(scale(g,5))
	return rgbhub75

def rgb_to_hub75_hex(r:int,g:int,b:int) -> str:
	'r,g,b in [0,255]'
	return f"0x{rgb_to_hub75(r,g,b):04x}"

def hsv_to_hub75_hex(h:int,s:float,v:float) -> str:
	'h in [0,360]; s,v in [0.0 , 1.0]'
	r,g,b = colorsys.hsv_to_rgb(h/360.0,s,v)
	return f"0x{rgb_to_hub75(r*255,g*255,b*255):04x}"

print(rgb_to_hub75_hex(255,0,255))
print(hsv_to_hub75_hex(300,1.0,1.0))