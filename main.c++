// Definisikan pin ESP32 yang terhubung ke modul 74HC595
const int LATCH_PIN = 5;  // STCP / RCLK
const int CLOCK_PIN = 18; // SHCP / SRCLK
const int DATA_PIN = 23;  // DS / SER

// Variabel untuk Non-Blocking Timer (Pengganti delay)
unsigned long motorStartTime = 0;
const unsigned long TIMEOUT_MOTOR = 4000; // Putar motor maksimal 4 detik jika sensor gagal
bool isMotorRunning = false;

void setup() {
  Serial.begin(115200);
  
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);

  // KUNCI UTAMA: Matikan semua relay di detik pertama booting
  stopAllMotors();
  Serial.println("Sistem Siap. Semua relay OFF.");
}

void loop() {
  // Simulasi memicu putaran motor dari Serial Monitor
  // Ketik "23" lalu Enter di Serial Monitor untuk memutar Rak 2, Nomor 3
  if (!isMotorRunning && Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.length() == 2) {
      int rak = String(input[0]).toInt();
      int nomor = String(input[1]).toInt();
      if (rak > 0 && nomor > 0) {
        vendItem(rak, nomor);
      }
    }
  }

  // LOGIKA NON-BLOCKING: Cek apakah motor sudah berputar melebihi batas waktu
  // Nanti, logika drop sensor dimasukkan ke dalam blok pengecekan ini
  if (isMotorRunning) {
    if (millis() - motorStartTime >= TIMEOUT_MOTOR) {
      stopAllMotors();
      Serial.println("Timeout: Motor berhenti karena batas waktu (Barang tersangkut/habis).");
    }
    
    // TODO: Tambahkan kondisi if (digitalRead(DROP_SENSOR_PIN) == LOW) di sini
    // untuk memanggil stopAllMotors() ketika barang terdeteksi jatuh sebelum timeout.
  }
}

// Fungsi utama untuk memutar motor spesifik
void vendItem(int rak, int nomor) {
  // Validasi absolut untuk mencegah error pada matrix
  if (rak < 1 || rak > 6 || nomor < 1 || nomor > 10) {
    Serial.println("ERROR: Target rak atau nomor di luar batas sistem.");
    return;
  }

  /* 
   * Asumsi Pemetaan Bit 74HC595 (16 Channel total dari 2 IC):
   * Bit 0-5  (6 bit)  -> Motor Negative (Rak 1-6)
   * Bit 6-15 (10 bit) -> Motor Positive (Nomor 1-10)
   */
   
  // Geser angka 1 ke posisi bit yang tepat
  uint16_t bitRak = 1 << (rak - 1);
  uint16_t bitNomor = 1 << ((nomor - 1) + 6); 

  // Gabungkan kedua perintah dalam 1 variable 16-bit
  uint16_t shiftData = bitRak | bitNomor;

  Serial.printf("Mengeksekusi Rak: %d, Nomor: %d\n", rak, nomor);
  
  updateShiftRegister(shiftData);
  motorStartTime = millis();
  isMotorRunning = true;
}

void stopAllMotors() {
  updateShiftRegister(0x0000); 
  isMotorRunning = false;
}

void updateShiftRegister(uint16_t data) {
  // PERHATIAN: Jika modul relay milikmu menyala saat diberi sinyal LOW (Active-Low),
  // ubah variabel "data" di bawah ini menjadi inverting bitwise, yaitu: data = ~data;
  
  digitalWrite(LATCH_PIN, LOW);
  
  // Kirim data 16-bit (2 byte) ke shift register
  // High byte dikirim lebih dulu (untuk pin Q8-Q15), disusul low byte (Q0-Q7)
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, highByte(data));
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, lowByte(data));
  
  // Trigger latch untuk mengaplikasikan sinyal ke relay
  digitalWrite(LATCH_PIN, HIGH);
}