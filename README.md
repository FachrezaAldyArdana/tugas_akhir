```
 Sistem Monitoring Listrik & Proteksi Overload Berbasis ESP32 dan Firebase
```

```
Proyek tugas akhir berupa sistem pemantauan konsumsi daya listrik dua jalur
(dual PZEM-004T v3.0) secara real-time berbasis ESP32, terintegrasi dengan
Firebase Realtime Database, kendali relai, serta fitur pemutus otomatis
(overload protection).
```

```
---
```

```
 📌 Fitur Utama
```

```
- Dual Monitoring PZEM-004T v3.0: Membaca parameter kelistrikan secara terpisah
pada 2 jalur beban:
```

- `Tegangan (Voltage - V)` 

- `Arus (Current - A)` 

- `Daya Aktif (Active Power - W)` 

```
  - Total Konsumsi Energi (Total Energy - kWh)
```

```
  - Frekuensi (Frequency - Hz)
```

```
  - Faktor Daya (Power Factor - PF)
- Sistem Proteksi Beban Berlebih (Overload Shutdown): Otomatis mematikan seluruh
relai beban saat daya terdeteksi mencapai $\ge 380\,\text{Watt}$ dan mengirimkan
status peringatan ke Firebase.
```

```
- Kendali Relai Real-Time: Menggunakan Firebase Stream (`beginStream`) untuk
kontrol nyala/mati relai tanpa delay (non-blocking).
```

```
- Pencatat Waktu Presisi (RTC DS3231): Menyertakan timestamp presisi (YYYY-MM-DD
HH:MM:SS) pada setiap pengiriman data untuk keperluan pencatatan data historis.
- Pengiriman Non-Blocking: Pembacaan dan transmisi sensor dilakukan setiap 3
detik menggunakan pewaktuan `millis()`.
```

```
---
```

`Kebutuhan Perangkat Keras (Hardware)` 🛠️� 

`1. ESP32 Development Board (30 Pin / 38 Pin)` 

`2. 2x Sensor PZEM-004T v3.0 + CT (Current Transformer)` 

```
3. Modul RTC DS3231 (I2C)
```

`4. Modul Relai 2-Channel (Active HIGH)` 

`5. Power Supply 5V / Adaptor step-down` 

`6. Beban listrik / kabel instalasi AC` 

```
---
```

```
 📌 Konfigurasi Pinout
```

```
| Komponen | Pin Modul | Pin ESP32 (GPIO) | Keterangan |
| :--- | :--- | :--- | :--- |
| Relay 1 | IN1 | GPIO 5 | Kendali Beban Jalur 1 |
| Relay 2 | IN2 | GPIO 18 | Kendali Beban Jalur 2 |
| PZEM-004T (Unit 1) | TX | GPIO 17 | Serial2 RX |
| | RX | GPIO 16 | Serial2 TX |
| PZEM-004T (Unit 2) | TX | GPIO 33 | HardwareSerial(1) RX |
```

```
| | RX | GPIO 25 | HardwareSerial(1) TX |
| RTC DS3231 | SDA | GPIO 14 | Custom I2C Data |
| | SCL | GPIO 27 | Custom I2C Clock |
```

```
---
```

📚 `Kebutuhan Pustaka (Libraries)` 

```
Pastikan pustaka berikut telah terpasang melalui Library Manager pada Arduino
IDE:
```

```
- [Firebase ESP32 Client](https://github.com/mobizt/Firebase-ESP32) oleh Mobizt
- [PZEM-004T v3.0](https://github.com/mandulaj/PZEM-004T-v30) oleh Jakub Mandula
```

- `[RTClib](https://github.com/adafruit/RTClib) oleh Adafruit` 

- ``Wire.h` dan `WiFi.h` (bawaan board package ESP32)` 

```
---
```

`Struktur Database Firebase Realtime` 🗄️� 

```
Struktur simpul (tree structure) data pada Firebase:
```

```
```text
monitoring_listrik/
├── kontrol_relay/
│   ├── relay1 (0 atau 1)
│   └── relay2 (0 atau 1)
├── status_sistem/
│   ├── kondisi ("AMAN" atau "OVERLOAD_SHUTDOWN")
│   └── esp32_last_seen ("YYYY-MM-DD HH:MM:SS")
├── sensor_pzem1/
│   ├── tegangan (V)
│   ├── arus (A)
│   ├── daya (W)
│   ├── total_kwh (kWh)
│   ├── frekuensi (Hz)
│   └── power_factor
└── sensor_pzem2/
    ├── tegangan (V)
    ├── arus (A)
    ├── daya (W)
    ├── total_kwh (kWh)
    ├── frekuensi (Hz)
    └── power_factor
```
```

```
---
```

⚙️� `Petunjuk Penggunaan & Pemasangan` 

```
1. Klon atau Unduh Repositori:
   ```bash
   git clone https://github.com/FachrezaAldyArdana/tugas_akhir.git
   ```
```

```
2. Buka Proyek: Buka file program (`.ino`) menggunakan Arduino IDE.
3. Konfigurasi Kredensial:
   Sesuaikan parameter berikut pada kode sebelum melakukan flash:
   ```cpp
   define WIFI_SSID "NAMA_WIFI"
   define WIFI_PASSWORD "PASSWORD_WIFI"
   define FIREBASE_HOST "URL_FIREBASE_RTDB"
   define FIREBASE_AUTH "DATABASE_SECRET_TOKEN"
   ```
```

`4. Pilih Board & Port:` 

```
   - Board: `DOIT ESP32 DEVKIT V1` (atau sesuaikan dengan modul ESP32 yang
digunakan).
   - Port: Pilih COM Port yang terdeteksi.
5. Upload: Unggah program ke modul ESP32.
6. Buka Serial Monitor pada baudrate `115200` untuk memantau status inisialisasi
jaringan dan sensor.
```

```
---
```

⚠️� `Catatan Keamanan (Safety Note)` 

```
> PERINGATAN: Proyek ini melibatkan pengukuran tegangan bolak-balik (AC)
bertegangan tinggi (220V). Pastikan seluruh sambungan terminasi kabel AC
```

```
terlindungi dengan baik menggunakan isolator dan terpasang dengan standar
keselamatan listrik yang benar. Hindari menyentuh jalur sensor PZEM dan terminal
relai saat terhubung ke sumber listrik.
```

