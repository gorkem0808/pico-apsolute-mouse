# GT ARCADE KONTROL IO BOARD V0.01 - Olması ve Olmaması Gerekenler

## Olması gerekenler

1. **2 oyuncu için 2 Pico kullanılacak.**
   - Pico 1 = Player 1
   - Pico 2 = Player 2

2. **Cihaz adları ayrı olacak.**
   - P1: `GT Arcade Kontrol IO P1 V001`
   - P2: `GT Arcade Kontrol IO P2 V001`

3. **GP20 sistem aktif / pasif olacak.**
   - GP20 basılı değilse silah hareketi ve oyun butonları pasif olur.
   - GP20 basılıysa potans, tetik, bomba ve reload çalışır.

4. **Kalibrasyon 4 köşe olacak.**
   - Sol üst
   - Sağ üst
   - Sağ alt
   - Sol alt

5. **Orta nokta kalibrasyonu olmayacak.**
   - Orta değer gerekiyorsa yazılım kendi hesaplayacak.

6. **Kalibrasyon sırasında mouse ile butona tıklamak olmayacak.**
   - Hedef köşeye silah çevrilir.
   - GP6'ya basılır.
   - Nokta kaydedilir.

7. **GP19 kalibrasyon / test çağırma butonu olacak.**
   - Hangi Pico'da GP19'a basılırsa program o oyuncunun ekranını açacak.

8. **GP6 hem bomba hem de menü seç / köşe kaydet olacak.**
   - Oyunda: bomba / grenade
   - Menüde: select / enter
   - Kalibrasyonda: köşeyi kaydet

9. **Buton test ekranında pin numarası yazmayacak.**
   - Programda görev isimleri görünecek: TETİK, START, COIN, vb.

10. **Titreşimi azalt menüsü sade olacak.**
    - Tek sürgü olacak: Az → Çok
    - Deadzone / smoothing gibi teknik detaylar kullanıcıya gösterilmeyecek.

11. **İvme olmayacak.**
    - Potans durunca nişangah duracak.
    - Akma, kayma, momentum olmayacak.

12. **P1 ve P2 testleri aynı PC programında olacak.**
    - P1 kalibrasyon
    - P2 kalibrasyon
    - P1/P2 buton testi
    - P1/P2 canlı nişangah testi
    - P1/P2 titreşim azaltma

13. **TeknoParrot'ta P1 ve P2 ayrı seçilecek.**
    - P1 Light Gun = P1 cihazı
    - P2 Light Gun = P2 cihazı

14. **Klavye karşılıkları not dosyasında yazacak.**
    - Program ekranında pin yazmayacak ama teknik notta pin ve klavye karşılığı olacak.

## Olmaması gerekenler

1. **Tek Pico ile iki oyuncu yapılmayacak.**
   - 4 potans için tek Pico doğrudan yeterli değil.
   - Karışma ve kalibrasyon sorunları olur.

2. **Kalibrasyonda orta nokta istenmeyecek.**

3. **Kalibrasyonda imleci Kaydet butonuna götürmek olmayacak.**

4. **İvme / momentum olmayacak.**
   - Nişangah hedefi geçip akmayacak.

5. **Programda GP2, GP3 gibi pin isimleri ana ekranda yazmayacak.**
   - Kullanıcı görev isimlerini görecek.

6. **P1 ve P2 tuşları çakışmayacak.**
   - Cihaz adları ve atamalar ayrı olacak.

7. **GP20 devre dışıyken tetik/bomba/reload çalışmayacak.**
   - Güvenlik ve arcade kontrol mantığı için GP20 ana aktif/pasif olacak.

8. **TeknoParrot servis kalibrasyonuna mecbur kalınmayacak.**
   - Ana kalibrasyon PC programı üzerinden yapılacak.
