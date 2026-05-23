# Pin ve Klavye Tablosu

## Her Pico için pin listesi

| Görev | Pico pini | Bağlantı |
|---|---:|---|
| X potans | GP26 | Potans orta uç |
| Y potans | GP27 | Potans orta uç |
| Sistem aktif / pasif | GP20 | Buton → GND |
| Tetik | GP7 | Buton → GND |
| Jarjör değiştir / Reload | GP8 | Buton → GND |
| Bomba + Menü Select + Kalibrasyon Kaydet | GP6 | Buton → GND |
| Menü yukarı | GP2 | Buton → GND |
| Menü aşağı | GP3 | Buton → GND |
| Menü sol | GP4 | Buton → GND |
| Menü sağ | GP5 | Buton → GND |
| Coin / Kredi | GP17 | Buton → GND |
| Start | GP18 | Buton → GND |
| Test / Kalibrasyon çağır | GP19 | Buton → GND |

## Potans bağlantısı

```text
Potans orta uç  → GP26 veya GP27
Potans dış uç   → 3V3
Potans diğer dış uç → GND
```

Öneri:

```text
GP26 ile GND arasına 100nF kondansatör
GP27 ile GND arasına 100nF kondansatör
```

## Klavye / mouse karşılıkları

| Görev | P1 karşılığı | P2 karşılığı |
|---|---|---|
| Tetik | Mouse Left | Mouse Left / P2 cihazı |
| Bomba / Select | Space | Right Ctrl |
| Reload | R | T |
| Menü yukarı | Up Arrow | Up Arrow |
| Menü aşağı | Down Arrow | Down Arrow |
| Menü sol | Left Arrow | Left Arrow |
| Menü sağ | Right Arrow | Right Arrow |
| Coin | NumPad 5 | NumPad 6 |
| Start | NumPad 1 | NumPad 2 |
| Test / Servis | NumPad 7 | NumPad 8 |

Not: TeknoParrot'ta en güvenli seçim cihaz bazlı atamadır. P1 ve P2 ayrı HID cihazı olarak seçilmelidir.
