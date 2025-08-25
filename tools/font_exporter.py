from PIL import Image
from pathlib import Path
image = Image.open(fr"{Path(__file__).resolve().parent.name}/../assets/letters.bmp")

def rgb_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

(img_x,img_y)=image.size
print(img_x*img_y)
new_img = Image.new("RGB",image.size)
converted_img=""
for y in range (0,img_y,16):
    for x in range (0,img_x,16):
        sprite = image.crop((0,y,16,y+16))
        new_img.paste(sprite.rotate(90).transpose(Image.Transpose.FLIP_TOP_BOTTOM),(0,y))
for y in range (img_y):
    for x in range (img_x):
        (r,g,b)=new_img.getpixel((x, y))     
        rgb565=rgb_to_rgb565(r, g, b)
        converted_img+=f"{str(rgb565)}," 
# new_img.save("test.bmp")
f = open(f"{Path(__file__).resolve().parent.name}/../assets/font_converted.txt", "w", encoding="utf-8")
f.write(converted_img[:-1]) 
f.close()