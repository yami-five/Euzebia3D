from PIL import Image
from pathlib import Path
file_name="win95_texture"
image = Image.open(fr"{Path(__file__).resolve().parent.name}/../assets/models/{file_name}.bmp")

def rgb_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

(img_x,img_y)=image.size
print(img_x*img_y)
converted_img=""
for y in range (img_y):
    for x in range (img_x-1,-1,-1):
        (r,g,b)=image.getpixel((x, y))
        # r = max(0, min(255, r))
        # g = max(0, min(255, g))
        # b = max(0, min(255, b))
        # r5 = r >> 3
        # g6 = g >> 2
        # b5 = b >> 3
        # r5 = (r*31)//255
        # g6 = (g*63)//255
        # b5 = (b*31)//255
        # rgb565 = (r5 << 11) | (g6 << 5) | b5
        # high_byte = f"0x{((rgb565 >> 8) & 0xFF):02X}"
        # low_byte = f"0x{(rgb565 & 0xFF):02X}"
        rgb565=rgb_to_rgb565(r, g, b)
        converted_img+=f"{str(rgb565)},"
        # converted_img=high_byte+","+low_byte+","+converted_img

f = open(f"{Path(__file__).resolve().parent.name}/img_converted.txt", "w", encoding="utf-8")
f.write(f"static const uint16_t {file_name}[{img_x*img_y}]={{{converted_img[:-1]}}};") 
f.close()