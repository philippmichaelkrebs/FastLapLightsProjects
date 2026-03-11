import math

TICKS_SCALE_FACTOR = 8.0
SPEED_CONVERSION_KMH = 3.6
SPEED_CONVERSION_KMH_24 = 24
TRACK_LENGTH_CONSTANT = 150000.0 # 150mm - shorter track min max narrower, longer track speed more accurate

print("Visualize speeds:")
step_size_24 = 5 # km/h in 1/24
steps = 64
start_speed_24 = 30 # km/h in 1/24

for i in range(steps):
	v24 = f'v_24 = {start_speed_24+(step_size_24*i)} km/h'
	ticks_calc = int(SPEED_CONVERSION_KMH*TRACK_LENGTH_CONSTANT/64/((start_speed_24+(step_size_24*i))/SPEED_CONVERSION_KMH_24))
	v24_calc = SPEED_CONVERSION_KMH*TRACK_LENGTH_CONSTANT/64/(int(SPEED_CONVERSION_KMH*TRACK_LENGTH_CONSTANT/64/((start_speed_24+(step_size_24*i))/SPEED_CONVERSION_KMH_24)))*SPEED_CONVERSION_KMH_24
	print(f'{v24} : ticks = {ticks_calc} : ({v24_calc:.2f} km/h)')
	if ticks_calc > (2**16)>>3:
		raise Exception(f"Ticks must be smaller {(2**16)>>3}. Increase start speed") 

print("\n\nLut:\n")
idx_total = 0
idx_2 = 0
lut = f'static uint16_t speedtrap_speed_lut[{steps}]'
lut += '{\n\t'
while (idx_total < steps):
	while (idx_total < steps):
		lut += f'{int(SPEED_CONVERSION_KMH*TRACK_LENGTH_CONSTANT/64/((start_speed_24+(step_size_24*idx_total))/SPEED_CONVERSION_KMH_24))},\t'
		idx_total += 1
		idx_2 += 1
		if idx_2 > 5:
			idx_2 = 0
			lut += '\n\t'
			break

lut += '\n};'
print(lut)
