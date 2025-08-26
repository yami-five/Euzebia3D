import math
from pathlib import Path

table_size=36000
pi=3.14159265358979323846
pi2=pi*2
resolution=pi2/table_size
scale_factor=1<<12
sin=[]
cos=[]
atan=[]

for i in range (0,table_size):
    angle = i*resolution
    sin.append(int(math.sin(angle)*scale_factor))
    cos.append(int(math.cos(angle)*scale_factor))
    
with open(f"{Path(__file__).resolve().parent.name}/sin_cos_atan.txt",'w') as f:
    f.write("const int sin_table[36000]={")
    for s in sin:
        f.write(f'{s},')
    f.write("};\n")
    
    f.write("const int cos_table[36000]={")
    for c in cos:
        f.write(f'{c},')
    f.write("};\n")
    
    size=70
    max=0
    f.write(f"const int atan_table[{((size+1)*2)*((size+1)*2)}]={{")    
    for a in range (-size,size+1):
        for b in range (-size,size+1):
            atan=round(math.atan2(a,b)*scale_factor)
            if(atan>max):max=atan
            f.write(f'{round(math.atan2(a,b)*scale_factor)},')
    f.write("};")
    print(max)