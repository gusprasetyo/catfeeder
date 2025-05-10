#include <ESP8266WiFi.h> //  library untuk konektivitas WiFi pada ESP8266.
#include <ESP8266HTTPClient.h> //  library untuk membuat permintaan HTTP pada ESP8266.
#include <EEPROM.h> //  library untuk membaca dan menulis ke EEPROM (Electrically Erasable Programmable Read-Only Memory).
#include <Servo.h> // library untuk mengontrol servo motor.
#include <HX711.h> //  library untuk antarmuka dengan sensor load cell HX711.
#include <Wire.h> //  library untuk komunikasi I2C.
#include <RTClib.h> //  library untuk berinteraksi dengan modul Real-Time Clock (RTC).

// --- Konfigurasi koneksi jaringan ---
const char* ssid = "V23";       
const char* password = "123456789"; 
const int httpPort = 8008;      

// --- Alamat IP server XAMPP (tersimpan di EEPROM) ---
struct ServerConfig { // Mendefinisikan struktur bernama ServerConfig untuk menyimpan konfigurasi server.
    char serverIP[16]; // Array karakter untuk menyimpan alamat IP server (maksimal 15 karakter + null terminator).
    int serverPort;    // Variabel integer untuk menyimpan nomor port server (tidak digunakan dalam kode ini, tetapi tetap ada dalam struktur).
};

// --- Pin yang Digunakan ---
#define TRIG_PIN 12 // Mendefinisikan TRIG_PIN sebagai pin 12, digunakan untuk sensor ultrasonik.
#define ECHO_PIN 13 // Mendefinisikan ECHO_PIN sebagai pin 13, digunakan untuk sensor ultrasonik.
#define LOADCELL_DOUT_PIN 4 // Mendefinisikan LOADCELL_DOUT_PIN sebagai pin 4, digunakan untuk pin data keluar dari load cell.
#define LOADCELL_SCK_PIN 0 // Mendefinisikan LOADCELL_SCK_PIN sebagai pin 0, digunakan untuk pin clock dari load cell.
#define SERVO_PIN 5 // Mendefinisikan SERVO_PIN sebagai pin 5, digunakan untuk mengontrol servo motor.

// --- Objek Servo, Load Cell, dan RTC ---
Servo myservo; // Membuat objek Servo bernama myservo.
HX711 scale; // Membuat objek HX711 bernama scale untuk load cell.
RTC_DS3231 rtc; // Membuat objek RTC_DS3231 bernama rtc untuk modul RTC.

// --- Variabel untuk Logika Pemberian Pakan ---
float beratAwal = 0.0; // Variabel untuk menyimpan berat awal pakan.
float beratSekarang = 0.0; // Variabel untuk menyimpan berat pakan saat ini.
float beratKeluar = 0.0; // Variabel untuk menyimpan berat pakan yang telah dikeluarkan.
float beratTarget = 39.0;   // Variabel untuk menyimpan berat target pakan yang akan dikeluarkan, diinisialisasi dengan nilai awal 30.0 (dapat diubah).
float toleransiBerat = 2.5;   // Variabel untuk menyimpan toleransi berat pakan, diinisialisasi dengan nilai awal 2.5 (dapat diubah).
bool isFeeding = false; // Variabel boolean untuk menandai apakah proses pemberian pakan sedang berlangsung atau tidak.

// --- Kalibrasi Load Cell ---
float calibration_factor = -253.09; // Variabel untuk menyimpan faktor kalibrasi load cell, didapat dari hasil kalibrasi

// --- Optimasi Pembacaan Load Cell ---
const int numReadings = 5;          // Jumlah pembacaan yang akan diambil untuk dirata-ratakan.
const int stableReadingsRequired = 2; // Jumlah pembacaan stabil yang dibutuhkan sebelum dianggap valid.
float readings[numReadings];         // Array untuk menyimpan nilai pembacaan dari load cell.
int readIndex = 0;                  // Indeks untuk melacak posisi pembacaan saat ini dalam array readings.
float total = 0;                    // Variabel untuk mengakumulasi total nilai pembacaan.
int stableReadingsCount = 0;         // Counter untuk menghitung jumlah pembacaan stabil berturut-turut.
float lastStableReading = 0;         // Variabel untuk menyimpan nilai pembacaan stabil terakhir.

// --- Untuk Kontrol Servo ---
int servoOpenPos = 50;             // Variabel untuk menyimpan posisi sudut servo saat terbuka.
int servoClosePos = 0;              // Variabel untuk menyimpan posisi sudut servo saat tertutup.
const int servoMoveDelay = 15;      // Variabel untuk menyimpan delay (dalam ms) antar pergerakan servo.
const int servoMoveIncrement = 1;   // Variabel untuk menyimpan increment sudut servo setiap pergerakan.

// --- Variabel untuk Jadwal Pemberian Makan ---
int feedingTimes[][2] = { // Array dua dimensi untuk menyimpan jadwal pemberian pakan (jam, menit).
    {7, 0},   // Jadwal pertama: 07:00
    {12, 0},  // Jadwal kedua: 12:00
    {19, 0}   // Jadwal ketiga: 19:00
};
const int numFeedingTimes = sizeof(feedingTimes) / sizeof(feedingTimes[0]); // Menghitung jumlah jadwal pemberian pakan berdasarkan ukuran array feedingTimes.

// --- Variabel untuk Status Makan dan Waktu Terakhir Makan ---
bool isCatEating = false; // Variabel boolean untuk menandai apakah kucing sedang makan atau tidak.
String lastFeedingTime = "Belum terdeteksi"; // Variabel string untuk menyimpan waktu terakhir kali kucing terdeteksi makan.

// --- Variabel untuk Reconnect ---
unsigned long lastReconnectAttempt = 0; // Variabel untuk menyimpan waktu terakhir kali upaya rekoneksi dilakukan.
int reconnectCount = 0; // Variabel untuk menghitung berapa kali upaya rekoneksi telah dilakukan.

// --- Variabel untuk Jeda Proses ---
unsigned long feedingStartTime = 0; // Variabel untuk menyimpan waktu dimulainya proses pemberian pakan.

// --- Variabel untuk Menyimpan Status Tare ---
bool isTareDone = false; // Flag untuk menandai apakah fungsi tare sudah dilakukan setelah startup.

// --- Fungsi-fungsi ---

// Membaca konfigurasi server dari EEPROM
ServerConfig readServerConfig() {
    ServerConfig config; // Membuat instance dari struct ServerConfig.
    EEPROM.begin(sizeof(ServerConfig)); // Memulai EEPROM dengan ukuran yang cukup untuk menyimpan struct ServerConfig.
    EEPROM.get(0, config); // Membaca data dari EEPROM ke dalam variabel config.
    EEPROM.end(); // Mengakhiri penggunaan EEPROM.
    return config; // Mengembalikan data konfigurasi server yang telah dibaca.
}

// Menyimpan konfigurasi server ke EEPROM
void saveServerConfig(const ServerConfig& config) {
    EEPROM.begin(sizeof(ServerConfig)); // Memulai EEPROM dengan ukuran yang cukup untuk menyimpan struct ServerConfig.
    EEPROM.put(0, config); // Menyimpan data dari variabel config ke EEPROM.
    EEPROM.commit(); // Menyimpan perubahan ke EEPROM secara permanen.
    EEPROM.end(); 
}

// Menghubungkan ke Wi-Fi
void connectToWiFi() {
    Serial.print("Menghubungkan ke "); 
    Serial.println(ssid); 

    WiFi.persistent(false); // Menonaktifkan penyimpanan konfigurasi WiFi secara permanen di flash memory.
    WiFi.begin(ssid, password); // Memulai koneksi ke jaringan WiFi dengan SSID dan password yang telah ditentukan.

    int retryCount = 0; // Inisialisasi variabel untuk menghitung jumlah percobaan koneksi.
    while (WiFi.status() != WL_CONNECTED && retryCount < 10) { // Melakukan loop selama status WiFi belum terhubung dan jumlah percobaan kurang dari 10.
        delay(500); // Menunggu selama 500ms.
        Serial.print("."); // Menampilkan titik di Serial Monitor sebagai indikator proses koneksi.
        retryCount++; // Menambahkan jumlah percobaan.
    }

    if (WiFi.status() == WL_CONNECTED) { // Memeriksa apakah WiFi telah terhubung.
        Serial.println(""); 
        Serial.println("Terhubung ke WiFi"); // Menampilkan pesan "Terhubung ke WiFi" di Serial Monitor.
        Serial.print("Alamat IP: "); // Menampilkan pesan "Alamat IP: " di Serial Monitor.
        Serial.println(WiFi.localIP()); // Menampilkan alamat IP yang diperoleh di Serial Monitor.

        WiFi.setSleepMode(WIFI_NONE_SLEEP); // Menonaktifkan mode sleep WiFi untuk koneksi yang lebih stabil.
        WiFi.setAutoReconnect(true); // Mengaktifkan fitur auto reconnect agar ESP8266 otomatis terhubung kembali ke WiFi jika koneksi terputus.

        reconnectCount = 0; // Mereset jumlah percobaan rekoneksi menjadi 0.
    } else {
        Serial.println("\nGagal terhubung ke WiFi setelah beberapa kali percobaan"); // Menampilkan pesan kegagalan koneksi di Serial Monitor.
    }
}

// Mengecek dan memastikan koneksi Wi-Fi
void ensureWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) { // Memeriksa apakah status WiFi tidak terhubung.
        Serial.println("Koneksi Wi-Fi terputus. Menghubungkan kembali..."); // Menampilkan pesan bahwa koneksi terputus dan akan mencoba menghubungkan kembali.
        connectToWiFi(); // Memanggil fungsi connectToWiFi() untuk menghubungkan kembali ke WiFi.
        lastReconnectAttempt = millis(); // Menyimpan waktu terakhir kali upaya rekoneksi dilakukan.
        reconnectCount++; // Menambahkan jumlah percobaan rekoneksi.
        Serial.print("Percobaan reconnect ke-"); // Menampilkan pesan "Percobaan reconnect ke-" di Serial Monitor.
        Serial.print(reconnectCount); // Menampilkan jumlah percobaan rekoneksi di Serial Monitor.
        Serial.print(" pada "); // Menampilkan pesan " pada " di Serial Monitor.
        Serial.println(lastReconnectAttempt); // Menampilkan waktu terakhir kali upaya rekoneksi dilakukan di Serial Monitor.
    }
}

// Memulai proses pemberian makan
void startFeeding() {
    beratAwal = getStableWeight(); // Mengambil berat awal pakan menggunakan fungsi getStableWeight() dan menyimpannya ke variabel beratAwal.
    feedingStartTime = millis(); // Mencatat waktu mulai pemberian pakan dan menyimpannya ke variabel feedingStartTime.
    Serial.println("Membuka servo perlahan..."); // Menampilkan pesan "Membuka servo perlahan..." di Serial Monitor.
    openServoSlowly(); // Membuka servo secara perlahan dengan memanggil fungsi openServoSlowly().
    isFeeding = true; // Mengubah status isFeeding menjadi true, menandakan bahwa proses pemberian pakan sedang berlangsung.
    Serial.println("Servo terbuka. Mulai memberi makan."); // Menampilkan pesan "Servo terbuka. Mulai memberi makan." di Serial Monitor.
}

// Menghentikan proses pemberian makan
void stopFeeding() {
    Serial.println("Menutup servo..."); // Menampilkan pesan "Menutup servo..." di Serial Monitor.
    closeServo(); // Menutup servo dengan memanggil fungsi closeServo().
    isFeeding = false; // Mengubah status isFeeding menjadi false, menandakan bahwa proses pemberian pakan telah selesai.
    Serial.print("Servo tertutup. Pakan yang keluar: "); // Menampilkan pesan "Servo tertutup. Pakan yang keluar: " di Serial Monitor.
    Serial.print(beratKeluar); // Menampilkan jumlah pakan yang telah dikeluarkan di Serial Monitor.
    Serial.println(" gram."); // Menampilkan satuan "gram" di Serial Monitor.
}

// Fungsi pembacaan berat yang dioptimalkan
float getStableWeight() {
    total = 0; // Mereset nilai total menjadi 0.
    readIndex = 0; // Mereset indeks pembacaan menjadi 0.
    stableReadingsCount = 0; // Mereset hitungan pembacaan stabil menjadi 0.

    // Isi array readings dengan nilai awal
    for (int i = 0; i < numReadings; i++) { // Melakukan loop sebanyak numReadings.
        readings[i] = scale.get_units(); // Mengambil data berat dari load cell dan menyimpannya ke dalam array readings.
        total += readings[i]; // Menambahkan nilai pembacaan ke total.
    }
    lastStableReading = total / numReadings; // Menghitung rata-rata pembacaan awal dan menyimpannya sebagai lastStableReading.

    while (stableReadingsCount < stableReadingsRequired) { // Melakukan loop selama jumlah pembacaan stabil kurang dari stableReadingsRequired.
        // Geser nilai dalam array (circular buffer)
        total -= readings[readIndex]; // Mengurangkan nilai pembacaan yang akan diganti dari total.
        readings[readIndex] = scale.get_units(); // Mengambil data berat dari load cell dan menyimpannya ke dalam array readings pada posisi readIndex.
        total += readings[readIndex]; // Menambahkan nilai pembacaan baru ke total.
        readIndex = (readIndex + 1) % numReadings; // Menambah indeks pembacaan, jika mencapai batas array, kembali ke 0 (circular buffer).

        float currentReading = total / numReadings; // Menghitung rata-rata pembacaan saat ini.

        // Cek kestabilan yang dipercepat
        if (abs(currentReading - lastStableReading) < 0.5) { // Memeriksa apakah selisih absolut antara pembacaan saat ini dan pembacaan stabil terakhir kurang dari 0.5.
            stableReadingsCount++; // Jika ya, tambahkan hitungan pembacaan stabil.
        } else {
            stableReadingsCount = 0; // Jika tidak, reset hitungan pembacaan stabil menjadi 0.
        }

        lastStableReading = currentReading; // Menyimpan pembacaan saat ini sebagai pembacaan stabil terakhir.
    }

    return lastStableReading; // Mengembalikan nilai pembacaan stabil terakhir.
}

// Membuka servo secara perlahan
void openServoSlowly() {
    Serial.println("openServoSlowly() dijalankan"); // Menampilkan pesan "openServoSlowly() dijalankan" di Serial Monitor.
    for (int pos = servoClosePos; pos <= servoOpenPos; pos += servoMoveIncrement) { // Melakukan loop dari posisi servo tertutup ke posisi servo terbuka dengan increment yang telah ditentukan.
        myservo.write(pos); // Mengatur posisi servo ke nilai pos saat ini.
        delay(servoMoveDelay); // Menunggu selama waktu yang ditentukan oleh servoMoveDelay.
    }
}

// Menutup servo dengan cepat
void closeServo() {
    Serial.println("closeServo() dijalankan"); // Menampilkan pesan "closeServo() dijalankan" di Serial Monitor.
    myservo.write(servoClosePos); // Mengatur posisi servo ke posisi tertutup.
}

// Membaca jarak dari sensor ultrasonik
float getUltrasonicDistance() {
    digitalWrite(TRIG_PIN, LOW); // Mengatur pin TRIG_PIN ke LOW.
    delayMicroseconds(2); // Menunggu selama 2 mikrodetik.
    digitalWrite(TRIG_PIN, HIGH); // Mengatur pin TRIG_PIN ke HIGH.
    delayMicroseconds(10); // Menunggu selama 10 mikrodetik.
    digitalWrite(TRIG_PIN, LOW); // Mengatur pin TRIG_PIN ke LOW.

    long duration = pulseIn(ECHO_PIN, HIGH); // Mengukur durasi pulsa HIGH pada pin ECHO_PIN.
    float distance = duration * 0.034 / 2; // Menghitung jarak berdasarkan durasi pulsa.

    return distance; // Mengembalikan nilai jarak yang telah dihitung.
}

// Memproses perintah jadwal yang diterima dari server
void processScheduleCommand(String command) {
    Serial.print("Menerima perintah jadwal: "); // Menampilkan pesan "Menerima perintah jadwal: " di Serial Monitor.
    Serial.println(command); // Menampilkan perintah jadwal yang diterima di Serial Monitor.

    // Parsing command untuk mendapatkan jadwal baru
    // Format command: "jadwal1 HH:MM jadwal2 HH:MM jadwal3 HH:MM"
    int firstSpaceIndex = command.indexOf(' '); // Mencari indeks spasi pertama dalam perintah.
    int secondSpaceIndex = command.indexOf(' ', firstSpaceIndex + 1); // Mencari indeks spasi kedua dalam perintah.

    if (firstSpaceIndex == -1 || secondSpaceIndex == -1) { // Memeriksa apakah format perintah tidak valid.
        Serial.println("Format perintah jadwal salah."); // Menampilkan pesan kesalahan format di Serial Monitor.
        return; // Keluar dari fungsi jika format perintah salah.
    }

    String jadwal1 = command.substring(0, firstSpaceIndex); // Memisahkan jadwal pertama dari perintah.
    String jadwal2 = command.substring(firstSpaceIndex + 1, secondSpaceIndex); // Memisahkan jadwal kedua dari perintah.
    String jadwal3 = command.substring(secondSpaceIndex + 1); // Memisahkan jadwal ketiga dari perintah.

    // Fungsi untuk mengurai waktu dari string HH:MM
    auto parseTime = [](String timeStr, int& hour, int& minute) {
        int colonIndex = timeStr.indexOf(':'); // Mencari indeks tanda titik dua (:) dalam string waktu.
        if (colonIndex != -1) { // Memeriksa apakah tanda titik dua ditemukan.
            hour = timeStr.substring(0, colonIndex).toInt(); // Mengambil jam dari string waktu.
            minute = timeStr.substring(colonIndex + 1).toInt(); // Mengambil menit dari string waktu.
        }
    };

    // Update feedingTimes
    parseTime(jadwal1, feedingTimes[0][0], feedingTimes[0][1]); // Memperbarui jadwal pertama dalam array feedingTimes.
    parseTime(jadwal2, feedingTimes[1][0], feedingTimes[1][1]); // Memperbarui jadwal kedua dalam array feedingTimes.
    parseTime(jadwal3, feedingTimes[2][0], feedingTimes[2][1]); // Memperbarui jadwal ketiga dalam array feedingTimes.

    Serial.print("Jadwal baru: "); // Menampilkan pesan "Jadwal baru: " di Serial Monitor.
    Serial.print(feedingTimes[0][0]); Serial.print(":"); Serial.print(feedingTimes[0][1]); Serial.print(" "); // Menampilkan jadwal pertama yang baru di Serial Monitor.
    Serial.print(feedingTimes[1][0]); Serial.print(":"); Serial.print(feedingTimes[1][1]); Serial.print(" "); // Menampilkan jadwal kedua yang baru di Serial Monitor.
    Serial.print(feedingTimes[2][0]); Serial.print(":"); Serial.println(feedingTimes[2][1]); // Menampilkan jadwal ketiga yang baru di Serial Monitor.
}

// Mendeteksi kucing makan
void detectCat() {
    float distance = getUltrasonicDistance(); // Memanggil fungsi getUltrasonicDistance() untuk mendapatkan jarak dari sensor ultrasonik.
    //Serial.print("Jarak: "); // Baris ini dikomentari, tidak akan dieksekusi.
    //Serial.print(distance); // Baris ini dikomentari, tidak akan dieksekusi.
    //Serial.println(" cm"); // Baris ini dikomentari, tidak akan dieksekusi.

    float detectionThreshold = 15.0; // Menentukan ambang batas jarak untuk mendeteksi kucing (dalam cm).
    if (distance < detectionThreshold) { // Memeriksa apakah jarak yang terdeteksi kurang dari ambang batas.
        if (!isCatEating) { // Memeriksa apakah kucing belum terdeteksi sedang makan sebelumnya.
            isCatEating = true; // Mengubah status isCatEating menjadi true, menandakan kucing sedang makan.
            DateTime now = rtc.now(); // Mengambil waktu saat ini dari RTC.
            lastFeedingTime = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()); // Mencatat waktu terakhir kali kucing terdeteksi makan.
            Serial.println("Kucing terdeteksi sedang makan."); // Menampilkan pesan "Kucing terdeteksi sedang makan." di Serial Monitor.
        }
    } else {
        if (isCatEating) { // Memeriksa apakah kucing sebelumnya terdeteksi sedang makan.
            isCatEating = false; // Mengubah status isCatEating menjadi false, menandakan kucing sudah selesai makan.
            Serial.println("Kucing selesai makan."); // Menampilkan pesan "Kucing selesai makan." di Serial Monitor.
        }
    }
}

// Menjalankan fungsi tare
void tareScale() {
    Serial.println("Tare scale..."); // Menampilkan pesan "Tare scale..." di Serial Monitor.
    scale.tare(); // Melakukan proses tare pada load cell.
    Serial.println("Tare selesai."); // Menampilkan pesan "Tare selesai." di Serial Monitor.
    isTareDone = true; // Mengatur flag isTareDone menjadi true, menandakan bahwa fungsi tare telah dijalankan.
}

// Menangani input dari Serial Monitor
void handleSerialInput() {
    if (Serial.available() > 0) { // Memeriksa apakah ada data yang tersedia di Serial Monitor.
        String command = Serial.readStringUntil('\n'); // Membaca data dari Serial Monitor hingga karakter newline ('\n') dan menyimpannya ke dalam variabel command.
        command.trim(); // Menghapus spasi di awal dan akhir string command.

        if (command == "c") { // Memeriksa apakah command yang diterima adalah "c".
            Serial.print("IP NodeMCU: "); // Menampilkan pesan "IP NodeMCU: " di Serial Monitor.
            Serial.println(WiFi.localIP()); // Menampilkan alamat IP NodeMCU di Serial Monitor.
        } else if (command == "cs") { // Memeriksa apakah command yang diterima adalah "cs".
            Serial.println("Masukkan IP server baru:"); // Menampilkan pesan "Masukkan IP server baru:" di Serial Monitor.
            while (Serial.available() == 0) {} // Menunggu hingga data tersedia di Serial Monitor.
            String newIP = Serial.readStringUntil('\n'); // Membaca data dari Serial Monitor hingga karakter newline ('\n') dan menyimpannya ke dalam variabel newIP.
            newIP.trim(); // Menghapus spasi di awal dan akhir string newIP.
            Serial.print("IP server diubah ke: "); // Menampilkan pesan "IP server diubah ke: " di Serial Monitor.
            Serial.println(newIP); // Menampilkan IP server yang baru di Serial Monitor.

            ServerConfig config = readServerConfig(); // Membaca konfigurasi server dari EEPROM.
            strcpy(config.serverIP, newIP.c_str()); // Menyalin IP server yang baru ke dalam struktur config.
            saveServerConfig(config); // Menyimpan konfigurasi server yang baru ke EEPROM.

        } else if (command == "cp") { // Memeriksa apakah command yang diterima adalah "cp".
            Serial.println("Masukkan port server baru:"); // Menampilkan pesan "Masukkan port server baru:" di Serial Monitor.
            while (Serial.available() == 0) {} // Menunggu hingga data tersedia di Serial Monitor.
            int newPort = Serial.parseInt(); // Membaca data integer dari Serial Monitor dan menyimpannya ke dalam variabel newPort.
            Serial.print("Port server diubah ke: "); // Menampilkan pesan "Port server diubah ke: " di Serial Monitor.
            Serial.println(newPort); // Menampilkan port server yang baru di Serial Monitor.

            ServerConfig config = readServerConfig(); // Membaca konfigurasi server dari EEPROM.
            config.serverPort = newPort; // Mengubah port server dalam struktur config.
            saveServerConfig(config); // Menyimpan konfigurasi server yang baru ke EEPROM.
        } else if (command.startsWith("nt=")) { // Memeriksa apakah command yang diterima dimulai dengan "nt=".
            // Mengubah nilai toleransi berat
            String valStr = command.substring(3); // Mengambil nilai toleransi berat dari command.
            toleransiBerat = valStr.toFloat(); // Mengubah nilai toleransi berat.
            Serial.print("Nilai toleransi diubah ke: "); // Menampilkan pesan "Nilai toleransi diubah ke: " di Serial Monitor.
            Serial.println(toleransiBerat); // Menampilkan nilai toleransi berat yang baru di Serial Monitor.
        } else if (command.startsWith("nb=")) { // Memeriksa apakah command yang diterima dimulai dengan "nb=".
            // Mengubah berat awal (berat target)
            String valStr = command.substring(3); // Mengambil nilai berat target dari command.
            beratTarget = valStr.toFloat(); // Mengubah nilai berat target.
            Serial.print("Nilai berat target diubah ke: "); // Menampilkan pesan "Nilai berat target diubah ke: " di Serial Monitor.
            Serial.println(beratTarget); // Menampilkan nilai berat target yang baru di Serial Monitor.
        } else if (command == "t") { // Memeriksa apakah command yang diterima adalah "t".
            // Menjalankan fungsi tare
            tareScale(); // Memanggil fungsi tareScale() untuk melakukan tare pada load cell.
        } else if (command.startsWith("ns=")) { // Memeriksa apakah command yang diterima dimulai dengan "ns=".
            // Mengubah sudut servo open
            String valStr = command.substring(3); // Mengambil nilai sudut servo open dari command.
            servoOpenPos = valStr.toInt(); // Mengubah nilai sudut servo open.
            Serial.print("Sudut servo open diubah ke: "); // Menampilkan pesan "Sudut servo open diubah ke: " di Serial Monitor.
            Serial.println(servoOpenPos); // Menampilkan nilai sudut servo open yang baru di Serial Monitor.
        } else if (command == "b") { // Memeriksa apakah command yang diterima adalah "b".
            // Menjalankan fungsi pemberian pakan manual dengan takaran
            if (!isFeeding) { // Memeriksa apakah proses pemberian pakan tidak sedang berlangsung.
                Serial.println("Memberi pakan secara manual..."); // Menampilkan pesan "Memberi pakan secara manual..." di Serial Monitor.
                beratAwal = getStableWeight(); // Mengambil berat awal sebelum membuka servo.
                openServoSlowly(); // Membuka servo secara perlahan.
                isFeeding = true; // Mengubah status isFeeding menjadi true.
            } else {
                Serial.println("Pemberian pakan sedang berlangsung."); // Menampilkan pesan "Pemberian pakan sedang berlangsung." di Serial Monitor.
            }
        }
    }
}

// --- Fungsi setup() dan loop() ---

void setup() {
    Serial.begin(9600); // Memulai komunikasi serial dengan baud rate 9600.

    // Inisialisasi Servo
    myservo.attach(SERVO_PIN); // Menghubungkan objek servo ke pin servo yang telah ditentukan.
    myservo.write(servoClosePos); // Mengatur servo ke posisi awal (tertutup).

    // Inisialisasi Sensor Ultrasonik
    pinMode(TRIG_PIN, OUTPUT); // Mengatur pin TRIG_PIN sebagai output.
    pinMode(ECHO_PIN, INPUT); // Mengatur pin ECHO_PIN sebagai input.

    // Inisialisasi Load Cell
    Serial.println("Inisialisasi Load Cell"); // Menampilkan pesan "Inisialisasi Load Cell" di Serial Monitor.
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN); // Memulai komunikasi dengan load cell.
    scale.set_scale(calibration_factor); // Mengatur faktor kalibrasi load cell.
    // scale.tare(); // Jangan langsung tare di setup, dilakukan nanti setelah pengecekan isTareDone.

    // Inisialisasi RTC
    Wire.begin(14, 2); // Memulai komunikasi I2C pada pin SDA = 14 dan SCL = 2.
    if (!rtc.begin()) { // Memeriksa apakah RTC terhubung dan berfungsi dengan baik.
        Serial.println("RTC tidak ditemukan!"); // Menampilkan pesan "RTC tidak ditemukan!" di Serial Monitor.
        while (1); // Program akan berhenti di sini jika RTC tidak ditemukan.
    }

    if (rtc.lostPower() || !rtc.now().isValid()) { // Memeriksa apakah RTC kehilangan daya atau waktu tidak valid.
        Serial.println("RTC kehilangan power atau waktu tidak valid, menyesuaikan waktu..."); // Menampilkan pesan di Serial Monitor.
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Mengatur waktu RTC berdasarkan waktu kompilasi program.
    }

    // Inisialisasi array pembacaan untuk rata-rata
    for (int i = 0; i < numReadings; i++) { // Melakukan loop sebanyak numReadings.
        readings[i] = 0; // Menginisialisasi setiap elemen array readings dengan 0.
    }

    // Baca konfigurasi server dari EEPROM
    ServerConfig serverConfig = readServerConfig(); // Membaca konfigurasi server dari EEPROM.

    // Koneksi ke Wi-Fi
    connectToWiFi(); // Menghubungkan NodeMCU ke jaringan WiFi.

    // Cek apakah tare sudah dilakukan sebelumnya
    EEPROM.begin(1); // Memulai EEPROM dengan ukuran 1 byte.
    isTareDone = EEPROM.read(0); // Membaca nilai isTareDone dari EEPROM.
    EEPROM.end(); // Mengakhiri penggunaan EEPROM.

    if (!isTareDone) { // Memeriksa apakah tare belum pernah dilakukan.
        tareScale(); // Melakukan tare pada load cell.
        EEPROM.begin(1); // Memulai EEPROM dengan ukuran 1 byte.
        EEPROM.write(0, isTareDone); // Menyimpan nilai isTareDone ke EEPROM.
        EEPROM.commit(); // Menyimpan perubahan ke EEPROM secara permanen.
        EEPROM.end(); // Mengakhiri penggunaan EEPROM.
    }

    Serial.println("Setup selesai."); // Menampilkan pesan "Setup selesai." di Serial Monitor.
}

void loop() {
    // **Penjedaan Proses Selama Pemberian Pakan**
    if (isFeeding) { // Memeriksa apakah proses pemberian pakan sedang berlangsung.
        // Hanya proses pembacaan load cell yang aktif
        beratSekarang = scale.get_units(); // Membaca berat pakan saat ini dari load cell.
        beratKeluar = beratAwal - beratSekarang; // Menghitung berat pakan yang telah dikeluarkan.

        Serial.print("Berat Awal: "); // Menampilkan pesan "Berat Awal: " di Serial Monitor.
        Serial.print(beratAwal); // Menampilkan berat awal di Serial Monitor.
        Serial.print(", Berat Sekarang: "); // Menampilkan pesan ", Berat Sekarang: " di Serial Monitor.
        Serial.print(beratSekarang); // Menampilkan berat sekarang di Serial Monitor.
        Serial.print(", Berat Keluar: "); // Menampilkan pesan ", Berat Keluar: " di Serial Monitor.
        Serial.print(beratKeluar); // Menampilkan berat yang telah dikeluarkan di Serial Monitor.
        Serial.print(", Target: "); // Menampilkan pesan ", Target: " di Serial Monitor.
        Serial.print(beratTarget - toleransiBerat); // Menampilkan berat target dikurangi toleransi di Serial Monitor.
        Serial.println(" gram"); // Menampilkan satuan "gram" di Serial Monitor.

        if (beratKeluar >= beratTarget - toleransiBerat - 2) { // Memeriksa apakah berat yang dikeluarkan sudah mencapai target.
            stopFeeding(); // Menghentikan proses pemberian pakan.
        }

        delay(20); // Menunggu selama 20ms untuk menjaga stabilitas sistem.

    } else {
        // Semua proses lain dijalankan di sini

        // Pastikan koneksi Wi-Fi tetap terjaga
        ensureWiFiConnection(); // Memastikan koneksi WiFi tetap terhubung.

        // Baca waktu dari RTC
        DateTime now = rtc.now(); // Mengambil waktu saat ini dari RTC.

        // Cek apakah sudah waktunya memberi makan
        static bool feedingTriggered[numFeedingTimes] = {false}; // Array untuk menandai apakah pemberian pakan sudah dipicu untuk setiap jadwal.

        for (int i = 0; i < numFeedingTimes; i++) { // Melakukan loop untuk setiap jadwal pemberian pakan.
            if (now.hour() == feedingTimes[i][0] && now.minute() == feedingTimes[i][1] && !feedingTriggered[i]) { // Memeriksa apakah waktu saat ini sesuai dengan jadwal dan belum dipicu.
                startFeeding(); // Memulai proses pemberian pakan.
                feedingTriggered[i] = true; // Menandai bahwa jadwal ini sudah dipicu.
            } else if (!(now.hour() == feedingTimes[i][0] && now.minute() == feedingTimes[i][1])) { // Memeriksa apakah waktu saat ini tidak sesuai dengan jadwal.
                feedingTriggered[i] = false; // Mereset penanda untuk jadwal ini.
            }
        }

        // --- Mengirim data ke server setiap 5 detik ---
        static unsigned long lastSendTime = 0; // Variabel untuk menyimpan waktu terakhir kali data dikirim ke server.
        if (millis() - lastSendTime >= 5000) { // Memeriksa apakah sudah 5 detik sejak pengiriman data terakhir.
            ServerConfig serverConfig = readServerConfig(); // Membaca konfigurasi server dari EEPROM.
            sendDataToServer(serverConfig.serverIP); // Mengirim data ke server.
            handleIncomingCommands(serverConfig.serverIP); // Meminta dan memproses perintah dari server.
            lastSendTime = millis(); // Memperbarui waktu terakhir kali data dikirim ke server.
        }

        // Deteksi kucing makan
        detectCat(); // Memanggil fungsi detectCat() untuk mendeteksi apakah kucing sedang makan.

        // Handle input dari Serial Monitor
        handleSerialInput(); // Memanggil fungsi handleSerialInput() untuk memproses input dari Serial Monitor.

        delay(20); // Menunggu selama 20ms untuk menjaga stabilitas sistem.
    }
}

// Fungsi untuk mengirim data ke server
void sendDataToServer(const char* serverIP) {
    if (!isFeeding) { // Memeriksa apakah proses pemberian pakan tidak sedang berlangsung.
        WiFiClient client; // Membuat objek WiFiClient.
        HTTPClient http; // Membuat objek HTTPClient.

        String url = "http://" + String(serverIP) + ":" + httpPort + "/adjie/data.php"; // Membuat URL untuk mengirim data.
        http.begin(client, url); // Memulai koneksi HTTP ke URL yang ditentukan.
        http.addHeader("Content-Type", "application/json"); // Menambahkan header "Content-Type: application/json" ke permintaan HTTP.

        String jsonData = "{"; // Memulai string JSON.
        jsonData += "\"beratSekarang\":" + String(getStableWeight()) + ","; // Menambahkan data berat sekarang ke string JSON.
        jsonData += "\"statusMakan\":\"" + String(isCatEating ? "Kucing sedang makan" : "Kucing tidak makan") + "\","; // Menambahkan data status makan ke string JSON.
        jsonData += "\"waktuMakan\":\"" + lastFeedingTime + "\","; // Menambahkan data waktu makan terakhir ke string JSON.

        // Tambahkan data jadwal ke JSON
        jsonData += "\"jadwal1\":\"" + String(feedingTimes[0][0]) + ":" + (feedingTimes[0][1] < 10 ? "0" : "") + String(feedingTimes[0][1]) + "\","; //
        jsonData += "\"jadwal2\":\"" + String(feedingTimes[1][0]) + ":" + (feedingTimes[1][1] < 10 ? "0" : "") + String(feedingTimes[1][1]) + "\","; // Menambahkan data jadwal kedua ke string JSON.
        jsonData += "\"jadwal3\":\"" + String(feedingTimes[2][0]) + ":" + (feedingTimes[2][1] < 10 ? "0" : "") + String(feedingTimes[2][1]) + "\""; // Menambahkan data jadwal ketiga ke string JSON.
        jsonData += "}\n"; // Menutup string JSON.

        Serial.print("Mengirim data ke server: "); // Menampilkan pesan "Mengirim data ke server: " di Serial Monitor.
        Serial.println(jsonData); // Menampilkan data JSON yang akan dikirim di Serial Monitor.

        int httpResponseCode = http.POST(jsonData); // Mengirim data JSON ke server menggunakan metode POST dan menyimpan kode respons HTTP ke dalam variabel httpResponseCode.

        if (httpResponseCode > 0) { // Memeriksa apakah kode respons HTTP lebih dari 0 (berhasil).
            String response = http.getString(); // Mengambil respons dari server dan menyimpannya ke dalam variabel response.
            Serial.print("HTTP Response code: "); // Menampilkan pesan "HTTP Response code: " di Serial Monitor.
            Serial.println(httpResponseCode); // Menampilkan kode respons HTTP di Serial Monitor.
            Serial.print("Response: "); // Menampilkan pesan "Response: " di Serial Monitor.
            Serial.println(response); // Menampilkan respons dari server di Serial Monitor.
        } else {
            Serial.print("Error sending POST request. HTTP response code: "); // Menampilkan pesan error di Serial Monitor.
            Serial.println(httpResponseCode); // Menampilkan kode respons HTTP di Serial Monitor.
        }

        http.end(); // Menutup koneksi HTTP.
    }
}

// Fungsi untuk secara berkala meminta perintah dari server
void handleIncomingCommands(const char* serverIP) {
    if (!isFeeding) { // Memeriksa apakah proses pemberian pakan tidak sedang berlangsung.
        WiFiClient client; // Membuat objek WiFiClient.
        HTTPClient http; // Membuat objek HTTPClient.

        String commandUrl = "http://" + String(serverIP) + ":" + httpPort + "/adjie/command.php"; // Membuat URL untuk meminta perintah.
        http.begin(client, commandUrl); // Memulai koneksi HTTP ke URL yang ditentukan.
        int httpResponseCode = http.GET(); // Mengirim permintaan GET ke server dan menyimpan kode respons HTTP ke dalam variabel httpResponseCode.

        if (httpResponseCode == HTTP_CODE_OK) { // Memeriksa apakah kode respons HTTP adalah HTTP_CODE_OK (200 OK).
            String payload = http.getString(); // Mengambil respons dari server (perintah) dan menyimpannya ke dalam variabel payload.
            Serial.print("Payload received from server: "); // Menampilkan pesan "Payload received from server: " di Serial Monitor.
            Serial.println(payload); // Menampilkan perintah yang diterima dari server di Serial Monitor.

            // Menghapus karakter baris baru atau karakter pengembalian yang tidak diinginkan
            payload.trim(); // Menghapus spasi di awal dan akhir string payload.

            if (payload.startsWith("jadwal")) { // Memeriksa apakah perintah yang diterima dimulai dengan "jadwal".
                processScheduleCommand(payload.substring(7)); // Memproses perintah untuk mengubah jadwal pemberian pakan.
                delay(2000);
            } else {
                Serial.println("Perintah tidak dikenal"); // Menampilkan pesan "Perintah tidak dikenal" di Serial Monitor.
            }
        } else {
            Serial.print("Error on HTTP request for commands. HTTP response code: "); // Menampilkan pesan error di Serial Monitor.
            Serial.println(httpResponseCode); // Menampilkan kode respons HTTP di Serial Monitor.
        }

        http.end(); // Menutup koneksi HTTP.
    }
}