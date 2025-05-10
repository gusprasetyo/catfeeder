<?php
include 'koneksi.php';

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    error_log("update_jadwal.php diakses via POST");

    $jadwal1 = $_POST['jadwal1'];
    $jadwal2 = $_POST['jadwal2'];
    $jadwal3 = $_POST['jadwal3'];

    error_log("Jadwal 1 diterima: " . $jadwal1);
    error_log("Jadwal 2 diterima: " . $jadwal2);
    error_log("Jadwal 3 diterima: " . $jadwal3);

    // Validasi Format Waktu HH:MM
    function validateTime($timeString) {
        $time = strtotime($timeString);
        return $time !== false && date('H:i', $time) === $timeString;
    }

    if (!validateTime($jadwal1) || !validateTime($jadwal2) || !validateTime($jadwal3)) {
        http_response_code(400); // Bad Request
        echo "Format waktu tidak valid.";
        error_log("Error: Format waktu tidak valid.");
        exit();
    }

    // Ambil data terbaru untuk mendapatkan id (asumsi Anda punya kolom 'id' auto-increment)
    $querySelect = "SELECT id FROM datadashboard ORDER BY id DESC LIMIT 1";
    $resultSelect = mysqli_query($koneksi, $querySelect);

    if ($resultSelect && mysqli_num_rows($resultSelect) > 0) {
        $row = mysqli_fetch_assoc($resultSelect);
        $id = $row['id'];

        // Query untuk mengupdate data di database menggunakan prepared statement
        $stmt = $koneksi->prepare("UPDATE datadashboard SET Jadwal1 = ?, Jadwal2 = ?, Jadwal3 = ? WHERE id = ?");
        $stmt->bind_param("sssi", $jadwal1, $jadwal2, $jadwal3, $id);

        if ($stmt->execute()) {
            // Perintah untuk NodeMCU
            $command = "jadwal " . $jadwal1 . " " . $jadwal2 . " " . $jadwal3;

            // Simpan perintah ke file
            $commandFile = 'adjie/command.txt';
            if (file_put_contents($commandFile, $command) !== false) {
                echo "Jadwal berhasil diperbarui dan perintah dikirim ke NodeMCU.";
                error_log("Jadwal berhasil diperbarui: " . $command);
            } else {
                http_response_code(500);
                echo "Error: Gagal menulis perintah ke file.";
                error_log("Error: Gagal menulis perintah ke file: " . $commandFile);
            }
        } else {
            http_response_code(500);
            echo "Error: Gagal update jadwal. " . $stmt->error;
            error_log("Error: Gagal update jadwal. " . $stmt->error);
        }
        $stmt->close();
    } else {
        http_response_code(500);
        echo "Error: Gagal mengambil data terbaru.";
        error_log("Error: Gagal mengambil data terbaru.");
    }

    mysqli_close($koneksi);
} else {
    http_response_code(405); // Method Not Allowed
    echo "Method tidak diizinkan.";
    error_log("Error: Method Not Allowed.");
}
?>