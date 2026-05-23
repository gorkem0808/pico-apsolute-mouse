# GT Arcade Pico Pro Calibration V3

Bu paket TeknoParrot / Paradise Lost tarzı 2 oyunculu potansiyometreli arcade silah sistemi içindir.

## Sistem

- 2 oyuncu için 2 adet Raspberry Pi Pico kullanılır.
- Pico 1 cihaz adı: `GT Arcade Pico P1 Pro Gun`
- Pico 2 cihaz adı: `GT Arcade Pico P2 Pro Gun`
- Her Pico hem HID absolute mouse hem de keyboard/serial cihazı gibi çalışır.
- PC kalibrasyon programı açıkken GP19 butonuna basınca kalibrasyon ekranı otomatik açılır.
- Kalibrasyon 4 köşedir. Orta nokta yoktur.

## Pin listesi

| Görev | Pin |
|---|---|
| X potans orta uç | GP26 |
| Y potans orta uç | GP27 |
| Nişangah aktif | GP20 |
| Tetik | GP7 |
| Jarjör değiştir / Reload | GP8 |
| Bomba + Menü Select / Enter | GP6 |
| Menü yukarı | GP2 |
| Menü aşağı | GP3 |
| Menü sol | GP4 |
| Menü sağ | GP5 |
| Coin / Kredi | GP17 |
| Start | GP18 |
| Test / Kalibrasyon | GP19 |

Bütün butonların bağlantısı aynıdır:

```text
GP pini → buton → GND
```

Potans bağlantısı:

```text
Potans orta uç → GP26 veya GP27
Potans dış uç  → 3V3
Potans diğer dış uç → GND
```

## Klavye karşılıkları

### Ortak

| Görev | Klavye / Mouse karşılığı |
|---|---|
| Tetik | Mouse Left Button |
| Bomba / Select | Space + Mouse Right Button |
| Menü yukarı | Up Arrow |
| Menü aşağı | Down Arrow |
| Menü sol | Left Arrow |
| Menü sağ | Right Arrow |
| Test / Servis | NumPad 7 |

### Player 1

| Görev | Karşılık |
|---|---|
| Reload | R |
| Coin | NumPad 5 |
| Start | NumPad 1 |

### Player 2

| Görev | Karşılık |
|---|---|
| Reload | T |
| Coin | NumPad 6 |
| Start | NumPad 2 |

## Kalibrasyon

1. `pc_app/gt_arcade_calibration_v3.py` programını aç.
2. Pico bağlıyken program P1/P2 cihazlarını görecek.
3. Pico üzerinde `GP19` test/kalibrasyon butonuna bas.
4. Program tam ekran kalibrasyon ekranını açar.
5. Sırayla 4 köşe çıkar:
   - Sol üst
   - Sağ üst
   - Sağ alt
   - Sol alt
6. Her köşede silahı hedefe çevir ve `GP6` Bomba/Seç butonuna bas.
7. 4 köşe bitince program ayarı Pico içine kaydeder ve ekran kapanır.

## PC programı

Python ile çalışır. Gerekli modül:

```text
pip install -r pc_app/requirements.txt
```

Sonra:

```text
python pc_app/gt_arcade_calibration_v3.py
```

İleride bu program EXE yapılabilir.

## GitHub Actions ile UF2 üretme

1. Bu klasördeki dosyaları GitHub reposuna yükle.
2. Actions sekmesine gir.
3. `Build GT Arcade Pico Pro V3 UF2` işlemini çalıştır.
4. Artifact içinden şu dosyalar çıkar:

```text
gt_arcade_pico_p1_pro_v3.uf2
gt_arcade_pico_p2_pro_v3.uf2
```

Pico 1'e P1 UF2, Pico 2'ye P2 UF2 atılır.
