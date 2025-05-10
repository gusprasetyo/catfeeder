Program ini adalah kode untuk perangkat pemberi makan hewan peliharaan otomatis yang dikendalikan oleh mikrokontroler ESP8266. Perangkat ini memiliki kemampuan untuk terhubung ke jaringan WiFi, berkomunikasi dengan server, dan mengontrol berbagai komponen perangkat keras untuk mengeluarkan pakan secara terjadwal atau manual.

Fungsionalitas Utama:
Konektivitas WiFi:

Program ini menghubungkan perangkat ESP8266 ke jaringan WiFi yang telah ditentukan (SSID "V23" dengan kata sandi "123456789" dalam kode ini).    
Jika koneksi terputus, program akan mencoba untuk menghubungkan kembali secara otomatis.    
Komunikasi dengan Server:

Pengiriman Data: Secara berkala (setiap 5 detik), program mengirimkan data ke server melalui HTTP POST. Data yang dikirim meliputi berat pakan saat ini di wadah, status apakah kucing sedang makan atau tidak, waktu terakhir kucing terdeteksi makan, dan jadwal pemberian pakan yang aktif.    
Penerimaan Perintah: Program juga meminta perintah dari server melalui HTTP GET. Saat ini, perintah yang didukung adalah untuk memperbarui jadwal pemberian pakan.    
Konfigurasi Server: Alamat IP dan port server dapat dikonfigurasi melalui Serial Monitor dan disimpan di EEPROM (memori internal ESP8266 yang datanya tidak hilang saat listrik padam).    
Kontrol Perangkat Keras:

Motor Servo: Mengontrol motor servo untuk membuka dan menutup katup dispenser pakan.  Servo akan membuka secara perlahan.    
Sensor Berat (Load Cell HX711): Mengukur berat pakan di dalam wadah menggunakan sensor load cell yang terhubung ke modul HX711.  Program memiliki logika untuk mendapatkan pembacaan berat yang stabil dan fungsi tare (mengenolkan timbangan).    
Sensor Ultrasonik: Mendeteksi keberadaan hewan peliharaan (kucing) di dekat tempat makan berdasarkan jarak.    
Jam Real-Time (RTC DS3231): Menggunakan modul RTC untuk menjaga waktu secara akurat, yang penting untuk fungsi penjadwalan pemberian pakan.    
Logika Pemberian Pakan:

Pemberian Pakan Terjadwal: Pakan akan dikeluarkan secara otomatis pada waktu-waktu yang telah ditentukan (misalnya, pukul 07:00, 12:00, dan 19:00).  Jadwal ini dapat diubah melalui perintah dari server.    
Pengeluaran Berdasarkan Berat: Saat memberi pakan, program akan mengeluarkan sejumlah pakan hingga mencapai target berat yang ditentukan (dikurangi toleransi).    
Pemberian Pakan Manual: Pengguna dapat memicu pemberian pakan secara manual melalui perintah yang dikirimkan lewat Serial Monitor.    
Deteksi Kucing Makan: Program akan mencatat dan melaporkan ke server ketika sensor ultrasonik mendeteksi kucing sedang berada di dekat tempat makan.    
Konfigurasi melalui Serial Monitor:

Menampilkan alamat IP perangkat ESP8266 di jaringan.    
Mengubah alamat IP dan port server.    
Menyesuaikan toleransi berat dan target berat pakan yang dikeluarkan.    
Melakukan kalibrasi ulang (tare) pada sensor berat.    
Mengubah posisi sudut buka motor servo.    
Memicu pemberian pakan manual.    
Penggunaan EEPROM:

Menyimpan konfigurasi server (IP dan port) agar tidak hilang saat perangkat dimatikan atau di-restart.    
Menyimpan status apakah fungsi tare pada load cell sudah pernah dilakukan atau belum.    
Penanganan Kesalahan dan Ketahanan:

Secara aktif mencoba menyambung kembali ke WiFi jika koneksi terputus.    
Jika RTC kehilangan daya atau waktunya tidak valid, program akan mengatur waktu RTC berdasarkan waktu saat program dikompilasi.    
Struktur Program:
setup(): Fungsi ini dijalankan sekali saat ESP8266 pertama kali dinyalakan atau di-reset. Fungsinya adalah untuk menginisialisasi semua komponen perangkat keras (komunikasi serial, motor servo, sensor ultrasonik, sensor berat, RTC), membaca konfigurasi server dari EEPROM, menghubungkan ke WiFi, dan melakukan tare awal jika diperlukan.    
loop(): Fungsi ini dijalankan berulang-ulang setelah setup() selesai.
Jika proses pemberian pakan sedang berlangsung (isFeeding bernilai true), program akan fokus memantau berat pakan yang keluar dan menghentikan motor servo jika target berat sudah tercapai.    
Jika tidak sedang memberi pakan, program akan menjalankan tugas-tugas lain seperti memastikan koneksi WiFi, memeriksa jadwal pemberian pakan berdasarkan waktu RTC, mengirim data ke server dan menangani perintah yang masuk secara berkala, mendeteksi kucing, dan menangani input dari Serial Monitor.    
# catfeeder
