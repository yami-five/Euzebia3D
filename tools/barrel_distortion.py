height,width,distortion,result=(320,240,0.1,[])

for y in range(height):
    ny=(2.0 *y/height)-1.0
    for x in range(width):
        nx=((2.0*x/width)-1.0) * (height/width)
        r2=nx*nx+ny*ny
        fx=nx * (1+distortion*r2)
        fy=ny * (1+distortion*r2)
        
        sx=int((fx+1)*0.5*width)
        sy=int((fy+1)*0.5*height)
        
        sx=min(max(sx,0),width-1)
        sy=min(max(sy,0),height-1)
        
        result.append(sy*width+sx)
        
with open("assets/barrel_dist.txt",'w') as file:
    text=""
    for x in result:
        text+=f"{x}, "
    file.write(f"const uint32_t barrel_distortion[{len(result)}]={{{str(text[:-1])}}};")
      