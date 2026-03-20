# Euzebia3D - Łopatologiczny Opis Renderingu (PL)

Ten dokument jest dla osoby, która:
- dostaje projekt "po kimś",
- nigdy nie pisała w C pod Raspberry Pi Pico,
- chce zrozumieć "co się dzieje od modelu 3D do piksela na LCD".

Nie musisz znać wszystkich szczegółów matematyki 3D, żeby zrozumieć ten opis.

---

## 1. Co to w ogóle robi?

Projekt renderuje scenę 3D programowo (CPU), bez GPU.

Czyli:
1. bierze model 3D (wierzchołki + trójkąty),
2. obraca/przesuwa go,
3. rzutuje na ekran 2D,
4. zamienia trójkąty na piksele,
5. wysyła cały obraz do wyświetlacza LCD przez DMA.

W tej chwili projekt używa **sortowania trójkątów** (painter's algorithm), a nie z-buffera.

---

## 2. Najważniejsze pliki (mapa projektu)

- `Euzebia3D.c`
  Główna pętla programu, tu widać "co dzieje się co klatkę".
- `libs/renderer/renderer.c`
  Serce renderingu 3D: clipping, culling, sortowanie, rasteryzacja, shading, texturing.
- `libs/cameraFactory/camera.c`
  Kamera: macierz widoku (`vMatrix`) i projekcji (`pMatrix`).
- `libs/meshFactory/meshFactory.c`
  Tworzenie obiektów Mesh z danych zaszytych w projekcie.
- `libs/painter/painter.c`
  Bufor ekranu + wysyłka DMA na LCD.
- `libs/display/display.c`
  Inicjalizacja LCD.
- `libs/hardware/hardware.c`
  Inicjalizacja SPI, GPIO, PWM itd.
- `libs/storage/gfx.c`
  Dane modeli i tekstur wkompilowane do firmware.

---

## 3. Jak startuje program (krok po kroku)

W `main()` (`Euzebia3D.c`) dzieje się to:

1. Ustawienie zegara Pico na 300 MHz.
2. Start warstwy hardware (`init_hardware`).
3. Start LCD (`init_display`).
4. Start paintera (bufor + DMA).
5. Start renderera (`init_renderer`) i ustawienie skali renderingu (`set_scale(1)`).
6. Utworzenie obiektów sceny:
   - 2 meshe (aktualnie 2 kubki),
   - 1 światło punktowe,
   - 1 kamera.
7. Wejście do pętli `while(1)` - to jest renderowanie kolejnych klatek.

---

## 4. Co dzieje się w każdej klatce?

W każdej iteracji pętli:

1. Zmieniane są transformacje (np. obrót obiektów, ruch kamery).
2. Kamera jest aktualizowana (`update_camera`) - przeliczane są macierze.
3. Renderer czyści listę trójkątów sceny (`clean_scene`).
4. Każdy model jest dodawany do sceny (`add_model_to_scene`).
5. Renderer rysuje całą scenę (`render_scene`).
6. Painter wysyła gotowy bufor do LCD (`draw_buffer`).
7. Bufor jest czyszczony kolorem tła pod następną klatkę.

Czyli prosto: **przelicz -> narysuj -> wyślij -> wyczyść -> powtórz**.

---

## 5. Co to jest Mesh, Material, Triangle?

### Mesh
Mesh to obiekt 3D:
- lista wierzchołków,
- lista trójkątów (faces),
- UV (współrzędne tekstury),
- normalne,
- materiał.

### Material
Materiał mówi rendererowi:
- czy używać tekstury (`texture` + `textureSize`),
- czy kolor stały (`diffuse`),
- czy to skybox (`isSkyBox`).

### Triangle
Renderer finalnie pracuje na trójkątach.
Każdy trójkąt ma:
- 3 punkty,
- UV,
- dane światła.

---

## 6. Najważniejsza część: pipeline renderingu 3D

Poniżej najważniejsza droga "od modelu do pikseli":

### Etap A: transformacje modelu

Dla każdego modelu:
- kopiowane są jego dane do buforów roboczych,
- nakładane są transformacje (obrót/przesunięcie/skala).

To dzieje się w `add_model_to_scene`.

### Etap B: kamera (view + projection)

Każdy wierzchołek przechodzi przez:
1. macierz widoku (`vMatrix`) - "jak kamera patrzy",
2. macierz projekcji (`pMatrix`) - "jak zamienić 3D na 2D".

Po tym kroku mamy współrzędne clip-space (`x, y, z, w`).

### Etap C: clipping (near plane)

To jest nowo poprawiony etap.

Zamiast wyrzucać cały trójkąt, gdy część jest za kamerą:
- trójkąt jest przycinany do płaszczyzny near (`z > 0`),
- nowe wierzchołki na krawędziach są interpolowane,
- UV i światło też są interpolowane.

Efekt: mniej "znikających" trójkątów przy kamerze.

### Etap D: triangulacja po clipie

Po clippingu polygon może mieć:
- 3 wierzchołki -> dalej 1 trójkąt,
- 4 wierzchołki -> dzielimy na 2 trójkąty.

### Etap E: perspective divide

Dla każdego wierzchołka:
- `screen_x = x / w`,
- `screen_y = y / w`.

To daje pozycję na ekranie 2D.

### Etap F: back-face culling

Jeśli trójkąt "patrzy tyłem" do kamery, nie rysujemy go.
Oszczędza to czas CPU.

### Etap G: zapis trójkątów do listy sceny

Trójkąty trafiają do statycznej tablicy `scene[]`.
Jest limit (`MAX_TRIANGLES_IN_SCENE = 1500`).

### Etap H: sortowanie trójkątów

Przed rysowaniem trójkąty są sortowane po głębokości (średnie `z`):
- najpierw dalsze,
- potem bliższe.

To zastępuje z-buffer.

### Etap I: rasteryzacja (trójkąt -> piksele)

Każdy trójkąt jest rysowany scanline'owo:
- linia po linii,
- piksel po pikselu.

W środku liczone są współrzędne barycentryczne (`Ba/Bb/Bc`).

### Etap J: kolor piksela

Dla każdego piksela:
1. Pobranie koloru (tekstura albo kolor stały).
2. Shading (światło):
   - interpolacja światła z wierzchołków,
   - clamp (żeby wartości nie "wybuchały").
3. Zapis koloru RGB565 do bufora LCD.

### Etap K: wysyłka całego obrazu na LCD

`painter.draw_buffer()`:
- bierze cały bufor ramki,
- wysyła go DMA, kawałkami, przez SPI na wyświetlacz.

---

## 6A. Matematyka renderera (szczegółowo)

Ta sekcja opisuje dokładniej wzory i operacje używane przez kod.

### 6A.1. Fixed-point (Q20.12)

Projekt używa stałoprzecinkowej reprezentacji liczb:
- `SHIFT_FACTOR = 12`
- `SCALE_FACTOR = 1 << 12 = 4096`

Znaczenie:
- `1.0` to `4096`
- `0.5` to `2048`

Podstawowe działania:
- `fixed = real * 4096`
- `real = fixed / 4096`
- `fixed_mul(a,b) ~= (a*b) >> 12`
- `fixed_div(a,b) ~= (a<<12) / b`

Dlaczego tak:
- na Pico jest to zwykle szybsze i bardziej przewidywalne niż pełny `float` wszędzie,
- łatwiej kontrolować koszt CPU i pamięci.

### 6A.2. Rotacja (kwaterniony)

W `rotate(...)` obrót jest liczony przez kwaternion:
- `theta = w * 2*pi`
- `q = [cos(theta/2), axis * sin(theta/2)]`
- wynik: `v' = q * v * q^-1`

Oś obrotu (`axis`) jest normalizowana przed użyciem.

### 6A.3. Macierz widoku (kamera)

W `camera.c`:
- `forward = normalize(pos - target)`
- `right = normalize(up x forward)` (w kodzie przez `mul_vectors`)
- `up = normalize(forward x right)`

Następnie budowana jest macierz widoku:
- osie kamery trafiają do części rotacyjnej,
- translacja to `-dot(pos, axis)`.

To daje klasyczne przejście świat -> przestrzeń kamery.

### 6A.4. Macierz projekcji perspektywicznej

Kod używa wersji perspektywicznej z `znear`, `zfar`, `aspect`, `tan(fov/2)`.

Wzór logiczny jest klasyczny:
- `p00 = 1/(tan(fov/2)*aspect)`
- `p11 = 1/tan(fov/2)`
- `p22 = -(zf+zn)/(zf-zn)`
- `p23 = -1`
- `p32 = -(2*zf*zn)/(zf-zn)`

W kodzie wszystko jest w fixed-point.

### 6A.5. Clip-space i near clipping

Po `vMatrix` i `pMatrix` każdy wierzchołek ma `(x,y,z,w)` w clip-space.

Warunek wnętrza near-plane:
- punkt jest \"wewnątrz\", gdy `z > 0`.

Dla krawędzi przecinającej near-plane liczony jest punkt przecięcia:
- `t = (znear - za)/(zb - za)` gdzie `znear = 1` (w fixed),
- interpolacja liniowa:
  `P = A + t*(B-A)`.

Interpolowane są jednocześnie:
- `x,y,z,w`,
- `uvx,uvy`,
- `light`.

Algorytm clippingu działa jak Sutherland-Hodgman dla pojedynczego trójkąta:
- wejście: 3 wierzchołki,
- wyjście: 0..4 wierzchołki,
- jeśli 4 wierzchołki, robiony jest podział na 2 trójkąty (triangle fan).

### 6A.6. Perspective divide i przejście na ekran

Dla każdego wierzchołka po clipie:
- `xs = x / w`
- `ys = y / w`

Potem przesunięcie do środka render targetu:
- `screen_x = xs + render_width_half`
- `screen_y = ys + render_height_half`

### 6A.7. Back-face culling

W 2D liczony jest znak \"podwójnego pola\":

`area2 = (bx-ax)*(cy-ay) - (by-ay)*(cx-ax)`

W kodzie trójkąt jest rysowany, gdy `area2 >= 0`.

### 6A.8. Sortowanie trójkątów (zamiast z-buffera)

Każdy trójkąt dostaje klucz głębokości:

`depth = (za + zb + zc)/3`

Następnie `qsort` układa trójkąty od dalszych do bliższych (painter's algorithm).

### 6A.9. Barycentryki w rasteryzacji

W `calc_bar_coords(...)`:
- `Ba`, `Bb` liczone ze wzorów ilorazu wyznaczników,
- `Bc = 1 - Ba - Bb`.

Dla punktu `(x,y)`:
- `Ba = ((by-cy)*(x-cx) + (cx-bx)*(y-cy)) / divider`
- `Bb = ((cy-ay)*(x-cx) + (ax-cx)*(y-cy)) / divider`
- `divider = (by-cy)*(ax-cx) + (cx-bx)*(ay-cy)`

W skanlinii używane są kroki `stepBa`, `stepBb`, żeby nie liczyć pełnego dzielenia dla każdego piksela od zera.

### 6A.10. Interpolacja UV i sampling tekstury

UV na piksel:
- `u = Ba*uA + Bb*uB + Bc*uC`
- `v = Ba*vA + Bb*vB + Bc*vC`

Potem:
- mnożenie przez rozmiar tekstury,
- dodanie pół piksela (`+0.5` w fixed),
- clamp do zakresu `[1, size-2]`.

Sampling:
- pobierane są 4 texele (`c00,c10,c01,c11`),
- wynik to średnia kanałów 2x2 (prosty box filter).

To nie jest pełna bilinear z wagami zależnymi od ułamka UV, tylko równe uśrednianie 4 próbek.

### 6A.11. Oświetlenie

Najpierw (na etapie geometrii) dla wierzchołków:
- `N = normalize(normal)`
- `Ldir = normalize(lightPos - vertexPos)`
- `Li = clamp(dot(N, Ldir), 0, 1)`

Potem (na pikselu):
- `L = Ba*L0 + Bb*L1 + Bc*L2`
- clamp do `[AMBIENT_MIN, 1]`

Dalej:
- mnożenie koloru materiału przez kolor światła (RGB565 kanałami),
- mnożenie przez intensywność światła,
- dodatkowe clampy bezpieczeństwa (`INTENSITY_MAX`, `MAX_LIGHT_FACTOR`),
- clamp końcowy do zakresów RGB565 (`R:0..31`, `G:0..63`, `B:0..31`).

### 6A.12. Skalowanie wyjścia

Renderer może działać w mniejszej rozdzielczości (`render_scale > 1`).

Każdy policzony piksel jest wtedy zapisywany jako blok:
- rozmiar bloku: `output_scale x output_scale`
- to daje tani upscale do LCD.

### 6A.13. Ważne konsekwencje matematyczne obecnego podejścia

- UV są interpolowane afinicznie (bez pełnej perspective-correct interpolation), więc przy mocnej perspektywie tekstura może lekko \"pływać\".
- Sortowanie trójkątów po średnim `z` nie rozwiązuje idealnie przypadków wzajemnego przecinania geometrii.
- Clipping jest obecnie na near-plane, nie na wszystkich 6 płaszczyznach frustum.
- Kod jest celowo zoptymalizowany pod kompromis: jakość vs koszt CPU/RAM na Pico 2.

---

## 7. Dlaczego bez z-buffera?

W tym projekcie z-buffer był testowany, ale:
- były artefakty (obraz wyglądał zaszumiony/niestabilny),
- koszt pamięci był za duży jak na założenia projektu.

Dlatego aktualnie jest sortowanie trójkątów.

Minus: przy przecinających się obiektach mogą pojawić się artefakty kolejności.

---

## 8. Co zostało już usprawnione ostatnio?

1. Ograniczono alokacje per-frame:
   - scratch-buffery są reużywane.
2. Poprawiono clipping near-plane:
   - trójkąty przecinające near plane nie znikają całe,
   - są przycinane i triangulowane.

To poprawia stabilność renderingu i zmniejsza "szarpanie" pamięci.

---

## 9. Co jest celowo "proste" / jeszcze niepełne?

To nie jest silnik AAA, tylko świadomy kompromis pod Pico + demoscenę.

Aktualne ograniczenia:
- brak z-buffera,
- brak pełnego clippingu na wszystkich płaszczyznach frustum,
- limit trójkątów sceny (z powodów pamięci),
- mały zestaw assetów testowych,
- nie wszystkie moduły są używane w `main` (celowo, bo fokus jest na 3D rendererze).

---

## 10. Jak najłatwiej czytać ten kod jako nowa osoba?

Najlepsza kolejność:

1. `Euzebia3D.c`
   Zobacz pętlę klatki.
2. `libs/renderer/IRenderer.h`
   Zobacz API renderera.
3. `libs/renderer/renderer.c`
   Czytaj w kolejności: `add_model_to_scene` -> `render_scene` -> `tri` -> `rasterize`.
4. `libs/cameraFactory/camera.c`
   Zrozum skąd biorą się macierze.
5. `libs/painter/painter.c`
   Zobacz jak piksel trafia na LCD.

---

## 11. Pseudokod jednej klatki (super-prosto)

```text
while (true):
    zaktualizuj animacje obiektów i kamery
    update_camera()

    clean_scene()

    for each mesh:
        transformuj wierzchołki
        policz clip-space
        przytnij trójkąty do near plane
        odrzuć back-face
        dodaj trójkąty do scene[]

    posortuj scene[] po głębokości

    for each triangle in scene[]:
        zrasteryzuj
        dla każdego piksela:
            pobierz kolor (tekstura/flat)
            policz światło
            zapisz do framebuffer

    wyślij framebuffer na LCD przez DMA
    wyczyść framebuffer
```

---

## 12. Słownik (bez akademickiego języka)

- **Wierzchołek (vertex)**: punkt w 3D.
- **Trójkąt (triangle)**: 3 wierzchołki, najmniejsza część modelu.
- **UV**: współrzędne mówiące, skąd wziąć kolor z tekstury.
- **Normalna**: wektor mówiący, "w którą stronę patrzy powierzchnia".
- **Clipping**: przycinanie geometrii do widocznego obszaru.
- **Culling**: odrzucanie trójkątów, których i tak nie zobaczysz.
- **Rasteryzacja**: zamiana trójkąta na piksele.
- **Framebuffer**: bufor obrazu w RAM.
- **DMA**: sprzętowy transfer danych bez obciążania CPU każdą operacją kopiowania.

---

## 13. Jednozdaniowe podsumowanie

Ten projekt to lekki software renderer 3D pod Pico 2: model -> transformacje -> clipping -> sortowanie trójkątów -> rasteryzacja -> bufor -> DMA na LCD.
