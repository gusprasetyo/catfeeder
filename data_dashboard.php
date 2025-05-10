<?php
include 'koneksi.php';

$query = "SELECT beratSekarang, statusMakan, jamTerakhirMakan, Jadwal1, Jadwal2, Jadwal3 FROM datadashboard ORDER BY id DESC LIMIT 1";
$result = mysqli_query($koneksi, $query);

if ($result) {
  if (mysqli_num_rows($result) > 0) {
    $row = mysqli_fetch_assoc($result);
    $data = array(
      'beratSekarang' => $row['beratSekarang'],
      'statusMakan' => $row['statusMakan'],
      'jamTerakhirMakan' => $row['jamTerakhirMakan'],
      'Jadwal1' => $row['Jadwal1'],
      'Jadwal2' => $row['Jadwal2'],
      'Jadwal3' => $row['Jadwal3']
    );
    echo json_encode($data);
  } else {
    echo json_encode(array()); // Mengembalikan array kosong jika tidak ada data
  }
} else {
  http_response_code(500); // Mengirim kode error 500 jika query gagal
  echo json_encode(array('error' => 'Error executing query: ' . mysqli_error($koneksi)));
}

mysqli_close($koneksi);
?>