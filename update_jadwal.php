<?php
//Kode PHP ini berperan sebagai middleware antara web server dan NodeMCU.  Kode ini menerima data jadwal dari server, memvalidasinya, mensterilkan input, membuat perintah, dan menyimpannya ke dalam file yang akan dibaca oleh NodeMCU.  Tujuannya adalah untuk menyediakan mekanisme yang aman dan terstruktur untuk mengatur jadwal pada NodeMCU.
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    error_log("update_jadwal.php diakses via POST");

    $jadwal1 = $_POST['jadwal1'];
    $jadwal2 = $_POST['jadwal2'];
    $jadwal3 = $_POST['jadwal3'];

    error_log("Jadwal 1 diterima: " . $jadwal1);
    error_log("Jadwal 2 diterima: " . $jadwal2);
    error_log("Jadwal 3 diterima: " . $jadwal3);

    // Validasi Format Waktu HH:MM menggunakan DateTime::createFromFormat
    function validateTime($timeString) {
        $dateTime = DateTime::createFromFormat('H:i', $timeString);
        return $dateTime && $dateTime->format('H:i') === $timeString;
    }

    if (!validateTime($jadwal1) || !validateTime($jadwal2) || !validateTime($jadwal3)) {
        http_response_code(400); // Bad Request
        echo json_encode(["status" => "error", "message" => "Format waktu tidak valid."]);
        error_log("Error: Format waktu tidak valid.");
        exit();
    }

    // Sanitasi Input (Untuk keamanan tambahan)
    $jadwal1 = htmlspecialchars($jadwal1);
    $jadwal2 = htmlspecialchars($jadwal2);
    $jadwal3 = htmlspecialchars($jadwal3);

    // Perintah untuk NodeMCU
    $command = "jadwal " . $jadwal1 . " " . $jadwal2 . " " . $jadwal3;

    // Lokasi file command.txt untuk XAMPP di Windows
    $commandFile = 'C:/xampp/htdocs/Adjie/command.txt'; // Path yang benar

    if (file_put_contents($commandFile, $command) !== false) {
        echo json_encode(["status" => "sukses", "message" => "Perintah berhasil dikirim ke NodeMCU.", "command" => $command]);
        error_log("Perintah berhasil dikirim: " . $command);
    } else {
        http_response_code(500);
        echo json_encode(["status" => "error", "message" => "Error: Gagal menulis perintah ke file."]);
        error_log("Error: Gagal menulis perintah ke file: " . $commandFile);
    }

} else {
    http_response_code(405); // Method Not Allowed
    echo json_encode(["status" => "error", "message" => "Method tidak diizinkan."]);
    error_log("Error: Method Not Allowed.");
}
?>