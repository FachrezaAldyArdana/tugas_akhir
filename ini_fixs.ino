#include <WiFi.h>
#include <FirebaseESP32.h>
#include <PZEM004Tv30.h>
#include <Wire.h>
#include "RTClib.h"

// 1. Konfigurasi Wi-Fi
#define WIFI_SSID "FnR"
#define WIFI_PASSWORD "rasahguyaguyu"

// 2. Konfigurasi Firebase
#define FIREBASE_HOST "https://monitoring-listrik-450-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "iXaoGTGAqoPpFqGFqISDIawZ326CSxh3JjIVAElY"

// 3. Definisi PIN Relay
const int PIN_RELAY_1 = 5;
const int PIN_RELAY_2 = 18;

// 4. Konfigurasi Serial untuk 2 PZEM
#define PZEM_RX_PIN 17
#define PZEM_TX_PIN 16
#define PZEM2_RX_PIN 33
#define PZEM2_TX_PIN 25

// Port Serial untuk PZEM
PZEM004Tv30 pzem(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);
HardwareSerial SerialPZEM2(1); 
PZEM004Tv30 pzem2(SerialPZEM2, PZEM2_RX_PIN, PZEM2_TX_PIN);

// 5. Kustomisasi PIN I2C untuk DS3231
#define RTC_SDA_PIN 14
#define RTC_SCL_PIN 27
RTC_DS3231 rtc;

// Deklarasi Objek Firebase
FirebaseData firebaseStream; 
FirebaseData firebaseSet;    
FirebaseConfig config;
FirebaseAuth auth;

// Variabel manajemen waktu non-blocking
unsigned long waktuSebelumnya = 0;
const long intervalKirim = 3000; 

// Status Relay di memori ESP32 (1 = ON, 0 = OFF)
int statusRelay1 = 0;
int statusRelay2 = 0;

void setup() {
  Serial.begin(115200);

  // Inisialisasi I2C kustom untuk RTC DS3231 (Sudah aman di SDA: 14, SCL: 27)
  Wire.begin(RTC_SDA_PIN, RTC_SCL_PIN);
  if (!rtc.begin()) {
    Serial.println("RTC DS3231 Tidak Ditemukan!");
  }

  if (rtc.lostPower()) {
    Serial.println("RTC sync waktu dengan laptop...");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Set PIN Relay sebagai OUTPUT
  pinMode(PIN_RELAY_1, OUTPUT);
  pinMode(PIN_RELAY_2, OUTPUT);
  digitalWrite(PIN_RELAY_1, LOW); 
  digitalWrite(PIN_RELAY_2, LOW);

  // Koneksi Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan ke Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Terhubung!");

  // Konfigurasi Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Mulai streaming kontrol dari root agar lebih fleksibel dan stabil
  if (!Firebase.beginStream(firebaseStream, "/monitoring_listrik")) {
    Serial.println("Gagal streaming database: " + firebaseStream.errorReason());
  }

  Serial.println("\n[Sistem] Menunggu 3 detik agar sensor PZEM stabil...");
  delay(3000); 
  Serial.println("[Sistem] PZEM Stabil. Masuk ke Mode Monitoring.");

}

void loop() {

/ =====================================================
  // CEK KONTROL RELAY REAL-TIME VIA STREAM (FIX TABRAKAN DATA)
  // =====================================================
  if (!Firebase.readStream(firebaseStream)) {
    Serial.println("Gagal membaca stream: " + firebaseStream.errorReason());
  }

  if (firebaseStream.streamTimeout()) {
    Serial.println("Stream timeout, resume streaming...");
  }

  // Pastikan data tersedia DAN tipe datanya adalah integer (0 atau 1 dari HP)
  if (firebaseStream.streamAvailable() && firebaseStream.dataType() == "int") {
    String path = firebaseStream.dataPath();
    int value = firebaseStream.intData();

    if (path.indexOf("/kontrol_relay/relay1") != -1) {
      statusRelay1 = value;
      digitalWrite(PIN_RELAY_1, (statusRelay1 == 1) ? HIGH : LOW);
      Serial.print("Relay 1 Berhasil Diubah -> ");
      Serial.println((statusRelay1 == 1) ? "ON" : "OFF");
    }
    
    if (path.indexOf("/kontrol_relay/relay2") != -1) {
      statusRelay2 = value;
      digitalWrite(PIN_RELAY_2, (statusRelay2 == 1) ? HIGH : LOW);
      Serial.print("Relay 2 Berhasil Diubah -> ");
      Serial.println((statusRelay2 == 1) ? "ON" : "OFF");
    }
  }
  // =====================================================
  // BACA SENSOR & LOGIKA PENGAMAN OVERLOAD
  // =====================================================
  unsigned long waktuSekarang = millis();
  
  if (waktuSekarang - waktuSebelumnya >= intervalKirim) {
    waktuSebelumnya = waktuSekarang; 

    // Baca sensor PZEM 1 & 2
    float v1 = pzem.voltage(); float a1 = pzem.current(); float p1 = pzem.power();
    float e1 = pzem.energy(); float f1 = pzem.frequency(); float pf1 = pzem.pf();

    float v2 = pzem2.voltage(); float a2 = pzem2.current(); float p2 = pzem2.power();
    float e2 = pzem2.energy(); float f2 = pzem2.frequency(); float pf2 = pzem2.pf();

    // Proteksi anti-NaN
    if (isnan(v1)) v1 = 0.0; if (isnan(a1)) a1 = 0.0; if (isnan(p1)) p1 = 0.0;
    if (isnan(v2)) v2 = 0.0; if (isnan(a2)) a2 = 0.0; if (isnan(p2)) p2 = 0.0;

    // 🔥 LOGIKA PROTEKSI HARDWARE OVERLOAD (>= 380W) 🔥
    if (p1 >= 380.0 || p2 >= 380.0) {
      Serial.println("\n⚠️ OVERLOAD DETECTED! Mematikan semua sistem...");
      
      digitalWrite(PIN_RELAY_1, LOW); statusRelay1 = 0;
      digitalWrite(PIN_RELAY_2, LOW); statusRelay2 = 0;
      
      // Update balik ke Firebase
      Firebase.setInt(firebaseSet, "/monitoring_listrik/kontrol_relay/relay1", 0);
      Firebase.setInt(firebaseSet, "/monitoring_listrik/kontrol_relay/relay2", 0);
      Firebase.setString(firebaseSet, "/monitoring_listrik/status_sistem/kondisi", "OVERLOAD_SHUTDOWN");
    } else {
      // Pastikan status kembali AMAN jika normal
      Firebase.setString(firebaseSet, "/monitoring_listrik/status_sistem/kondisi", "AMAN");
    }

    // Ambil Jam dari RTC DS3231 untuk Timestamp data historis harian
    DateTime now = rtc.now();
    char timestampBuffer[25];
    sprintf(timestampBuffer, "%04d-%02d-%02d %02d:%02d:%02d", 
            now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

    // --- SEND DATA TO FIREBASE ---
    Firebase.setFloat(firebaseSet, "/monitoring_listrik/sensor_pzem1/tegangan", v1);
    Firebase.setFloat(firebaseSet, "/monitoring_listrik/sensor_pzem1/arus", a1);
    Firebase.setFloat(firebaseSet, "/monitoring_listrik/sensor_pzem1/daya", p1);
    Firebase.setFloat(firebaseSet, "/monitoring_listrik/sensor_pzem1/total_kwh", e1); 
    Firebase.setFloat(firebaseSet, "/monitoring_listrik/sensor_pzem1/frekuensi", f1);
    Firebase.setFloat(firebaseSet, "/monitoring_listrik/sensor_pzem1/power_factor", pf1); 

    Firebase.setFloat(firebaseSet, "/monitoring_listrik/sensor_pzem2/tegangan", v2);
    Firebase.setFloat(firebaseSet, "/monitoring_listrik/sensor_pzem2/arus", a2);
    Firebase.setFloat(firebaseSet, "/monitoring_listrik/sensor_pzem2/daya", p2);
    Firebase.setFloat(firebaseSet, "/monitoring_listrik/sensor_pzem2/total_kwh", e2);
    Firebase.setFloat(firebaseSet, "/monitoring_listrik/sensor_pzem2/frekuensi", f2);
    Firebase.setFloat(firebaseSet, "/monitoring_listrik/sensor_pzem2/power_factor", pf2);

    Firebase.setString(firebaseSet, "/monitoring_listrik/status_sistem/esp32_last_seen", timestampBuffer);
    Serial.print("Data Terkirim RTC: "); Serial.println(timestampBuffer);
  }
}