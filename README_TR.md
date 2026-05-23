# Pico TinyUSB HID Absolute Mouse - TeknoParrot Paradise Lost

Bu proje Raspberry Pi Pico'yu USB HID Absolute Mouse gibi gösterir.
Amaç: potansiyometreli sabit silahı Windows/TeknoParrot içinde mouse/lightgun gibi kullanmak.

## Pin bağlantıları

| Görev | Pico pini |
|---|---|
| X potans orta uç | GP26 |
| Y potans orta uç | GP27 |
| Potans dış uç 1 | 3V3 |
| Potans dış uç 2 | GND |
| Aktif/Pasif anahtar | GP20 -> GND olunca aktif |
| Tetik / sol tık | GP14 -> buton -> GND |
| 2. buton / sağ tık | GP15 -> buton -> GND |

Potans önerisi: 10K linear potans.

## UF2 nasıl alınır?

En kolay yol GitHub Actions:

1. GitHub'da yeni boş repository aç.
2. Bu ZIP içindeki tüm dosyaları repository içine yükle.
3. GitHub sayfasında `Actions` sekmesine gir.
4. `Build Pico UF2` işini çalıştır.
5. İş bitince `Artifacts` altında `pico_absolute_mouse_uf2` çıkacak.
6. İndir, ZIP'i aç, içinden `pico_absolute_mouse.uf2` dosyasını al.
7. Pico'nun BOOTSEL tuşuna basılı tutup USB'ye tak.
8. Açılan `RPI-RP2` diskine UF2 dosyasını kopyala.

## Not

Bu paket sahte UF2 içermez. Gerçek UF2, GitHub Actions tarafından derlenir.
