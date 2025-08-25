vertices=[]
faces=[]
vt=[]
uv=[]
normals=[]
vn=[]
fileName="mug"
with open(f"assets/models/{fileName}.obj") as f:
    while line:=f.readline():
        if line[0:2]=='v ':
            vertex=line.split()
            vertices.append(f'{round(float(vertex[1]),2)}f')
            vertices.append(f'{round(float(vertex[2]),2)}f')
            vertices.append(f'{round(float(vertex[3]),2)}f')
        elif line[0]=='f':
            face=line.split()
            faces.append(int(face[1].split('/')[0])-1)
            uv.append(int(face[1].split('/')[1])-1)
            normals.append(int(face[1].split('/')[2])-1)
            faces.append(int(face[2].split('/')[0])-1)
            uv.append(int(face[2].split('/')[1])-1)
            normals.append(int(face[2].split('/')[2])-1)
            faces.append(int(face[3].split('/')[0])-1)
            uv.append(int(face[3].split('/')[1])-1)
            normals.append(int(face[3].split('/')[2])-1)
        elif line[0:2]=='vn':
            vn.append(f'{round(float(line.split()[1]),2)}f')
            vn.append(f'{round(float(line.split()[2]),2)}f')
            vn.append(f'{round(float(line.split()[3]),2)}f')
        elif line[0:2]=='vt':
            vt.append(f'{round(float(line.split()[1]),2)}f')
            vt.append(f'{round(float(line.split()[2]),2)}f')
print(fileName)
text=""
for x in vertices:
    text+=f'{x},'
print(f"const float {fileName}Vertices[{str(len(vertices))}]={{{str(text[:-1])}}};")
text=""
for x in faces:
    text+=f'{x},'
print(f"const uint16_t {fileName}Faces[{str(len(faces))}] = {{{str(text[:-1])}}};")
text=""
for x in vt:
    text+=f'{x},'
print(f"const float {fileName}TextureCoords[{str(len(vt))}] = {{{str(text[:-1])}}};")
text=""
for x in uv:
    text+=f'{x},'
print(f"const uint16_t {fileName}UV[{str(len(faces))}] = {{{str(text[:-1])}}};")
text=""
for x in vn:
    text+=f'{x},'
print(f"const float {fileName}VN[{str(len(vn))}] = {{{str(text[:-1])}}};")
text=""
for x in normals:
    text+=f'{x},'
print(f"const uint16_t {fileName}Normals[{str(len(faces))}] = {{{str(text[:-1])}}};")

print(f"vertices={str(len(vertices)//3)}")
print(f"faces={str(len(faces)//3)}")
print(f"vtn={str(len(vt)//2)}")
print(f'nn={str(len(vn)//3)}')