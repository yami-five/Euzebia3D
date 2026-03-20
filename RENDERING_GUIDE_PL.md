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
