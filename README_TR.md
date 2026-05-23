# GT Arcade Pico Pro Calibration V3.2

Bu sürüm sadeleştirilmiş profesyonel sürümdür.

## Ana özellikler

- 2 oyuncu için 2 ayrı Pico desteği
- P1 ve P2 cihaz adları ayrı görünür
- 4 köşe potans kalibrasyonu
- Orta nokta kalibrasyonu yoktur
- Buton test ekranında GP numarası yazmaz, görev isimleri yazar
- **Titreşimi Azalt** menüsü sade sürgülüdür
- İvme ayarı yoktur
- GP20 sistem aktif / pasif olarak çalışır

## UF2 dosyaları

GitHub Actions derleme sonunda şu dosyalar oluşur:

- `gt_arcade_pico_p1_pro_v32.uf2`
- `gt_arcade_pico_p2_pro_v32.uf2`

## GP20 sistem aktif / pasif

GP20 basılı değilse silah kontrolleri pasif olur:

- Potans nişangahı hareket ettirmez
- Tetik çalışmaz
- Bomba çalışmaz
- Reload çalışmaz
- Klavye tuş gönderimi kapalı olur

GP20 basılıyken sistem aktif olur:

- Potans nişangahı hareket ettirir
- GP7 tetik çalışır
- GP8 reload çalışır
- GP6 bomba/select çalışır
- Menü/start/coin tuşları gönderilir

Not: GP19 kalibrasyon sinyali PC programına yine gönderilir. Böylece program açıkken kalibrasyon ekranı açılabilir.

## Pin listesi

- GP26 = X potans
- GP27 = Y potans
- GP20 = Sistem aktif / pasif
- GP7 = Tetik
- GP8 = Jarjör değiştir / Reload
- GP6 = Bomba + Menü Select / Enter
- GP2 = Menü yukarı
- GP3 = Menü aşağı
- GP4 = Menü sol
- GP5 = Menü sağ
- GP17 = Coin / Kredi
- GP18 = Start
- GP19 = Test / Kalibrasyon

## Bağlantı

Bütün butonlar:

`GP pini -> Buton -> GND`

Potans:

- Orta uç -> GP26 veya GP27
- Bir dış uç -> 3V3
- Diğer dış uç -> GND

## PC programını çalıştırma

`pc_app` klasöründe CMD açıp şunu yaz:

```bat
python gt_arcade_calibration_v32.py
```

Olmazsa:

```bat
py gt_arcade_calibration_v32.py
```

## Program bölümleri

1. Potans Kalibrasyonu
2. Canlı Nişangah Testi
3. Buton Testi
4. Titreşimi Azalt

## Titreşimi Azalt

Programda sadece sade bir sürgü vardır:

`Az  ----  Çok`

Sağa artırırsan titreme azalır. Çok artırırsan tepki biraz yumuşar.

## Kalibrasyon

Kalibrasyon 4 köşedir:

1. Sol üst
2. Sağ üst
3. Sağ alt
4. Sol alt

Her köşede silahı hedefe çevirip `KÖŞEYİ KAYDET` veya GP6 ile kayıt yapılır. Sonra `KAYDET VE ÇIK` ile ayar Pico'ya gönderilir ve ekran kapanır.
