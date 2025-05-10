<?php
//Kode ini digunakan untuk mengambil perintah dari file command.txt, mengirim perintah tersebut ke client yang melakukan request, dan kemudian menghapus perintah dari file  agar tidak dieksekusi berulang-ulang
if (file_exists('C:/xampp/htdocs/Adjie/command.txt')) {
    $command = file_get_contents('C:/xampp/htdocs/Adjie/command.txt');
    echo $command; // Kirim isi file ke client DULU

    // Hapus isi file SETELAH dibaca dan dikirim
    file_put_contents('C:/xampp/htdocs/Adjie/command.txt', '');
} else {
    echo ""; // Mengembalikan string kosong jika tidak ada perintah
}
?>