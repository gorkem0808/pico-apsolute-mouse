# GT ARCADE KONTROL IO BOARD V0.01

Bu proje, TeknoParrot / Far Cry Paradise Lost tarzı arcade lightgun sistemi için hazırlanmış **iki Pico destekli profesyonel kontrol kartı projesidir**.

## Ana hedef

- 2 oyuncu olacak.
- 2 adet Raspberry Pi Pico kullanılacak.
- Her oyuncunun Pico'su ayrı cihaz adıyla görünecek.
- Player 1 ve Player 2 tetik, bomba, reload, coin, start ve nişangah hareketleri birbirine karışmayacak.
- Kalibrasyon TeknoParrot menüsünden değil, bizim PC programımızdan yapılacak.
- GP19'a basınca ilgili oyuncunun kalibrasyon ekranı açılacak.
- Kalibrasyonda mouse ile butona gitmek yok. GP6 ile köşe kaydedilecek.
- GP20 sistem aktif / pasif olacak.

## Klasörler

```text
firmware/  = Pico P1/P2 firmware kaynakları ve GitHub Actions derleme yapısı
pc_app/    = Windows masaüstü kalibrasyon/test programı
docs/      = pin listesi, klavye karşılıkları, yapılacak/yapılmayacaklar
tools/     = kolay başlatma ve başlangıca ekleme BAT dosyaları
```

## Çıkacak UF2 dosyaları

GitHub Actions ile derleme sonunda iki UF2 oluşması hedeflenir:

```text
gt_arcade_kontrol_io_p1_v001.uf2
gt_arcade_kontrol_io_p2_v001.uf2
```

## Önemli not

GP19 ile kalibrasyon ekranının otomatik açılması için PC programı arka planda açık olmalıdır. Windows açılışında otomatik başlatmak için:

```text
tools/BASLANGICA_EKLE.bat
```

çalıştırılabilir.
