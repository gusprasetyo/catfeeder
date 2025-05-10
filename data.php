<?php
include 'koneksi.php';
//Fungsi kode PHP ini adalah untuk menyimpan data yang dikirim melalui metode HTTP POST ke dalam database MySQL.
if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $jsonData = file_get_contents('php://input');
    $data = json_decode($jsonData, true);

    if ($data === null) {
        http_response_code(400);
        echo "Invalid JSON data.";
        exit();
    }

    $beratSekarang = $data['beratSekarang'];
    $statusMakan = $data['statusMakan'];
    $waktuMakan = $data['waktuMakan'];
    $jadwal1 = $data['jadwal1'];
    $jadwal2 = $data['jadwal2'];
    $jadwal3 = $data['jadwal3'];


    // Simpan data ke database
    $query = "INSERT INTO datadashboard (beratSekarang, statusMakan, jamTerakhirMakan, Jadwal1, Jadwal2, Jadwal3) VALUES ('$beratSekarang', '$statusMakan', '$waktuMakan', '$jadwal1', '$jadwal2', '$jadwal3')";

    if (mysqli_query($koneksi, $query)) {
        echo "Data saved successfully.";
    } else {
        http_response_code(500);
        echo "Error: " . $query . "<br>" . mysqli_error($koneksi);
    }

    mysqli_close($koneksi);
} else {
    http_response_code(405); // Method Not Allowed
    echo "Method not allowed.";
}
?>