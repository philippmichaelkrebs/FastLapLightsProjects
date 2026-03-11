from PIL import Image

# Load image
img = Image.open("Logo_Carrera_smallpng.bmp")
img = img.convert("RGB")

width, height = img.size
pixels = list(img.getdata())

def rgb_to_custom_16bit(r, g, b):
    # Convert 8-bit to 5/6/5 bits
    r5 = r >> 3        # 5 bits
    b6 = b >> 2        # 6 bits
    g5 = g >> 3        # 5 bits

    # Pack as: rrrrr bbbbbb ggggg
    value = (r5 << 11) | (b6 << 5) | g5
    return value

# Create 16-bit list
pixel_16bit_list = [rgb_to_custom_16bit(r, g, b) for (r, g, b) in pixels]

#print(len(pixel_16bit_list))  # Should be 2048 (64 * 32)
#print(pixel_16bit_list)

def to_hex4(value):
    return f"0x{value:04x}"

pixel_16bit_list_hex = [to_hex4(i) for i in pixel_16bit_list]
#print(pixel_16bit_list_hex)

string = f'static uint16_t your_static_variable[{len(pixel_16bit_list)}]'
string += '{\n\t'
idx = 0
for i in range(len(pixel_16bit_list_hex)):
    string += f'{pixel_16bit_list_hex[i]}, '
    idx += 1
    if idx > 10:
        idx = 0
        string += '\n\t'
string += '\n};'
print(string)