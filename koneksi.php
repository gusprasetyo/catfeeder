<?php
$host = "localhost"; // Ganti dengan host database Anda
$user = "root"; // Ganti dengan username database Anda
$password = ""; // Ganti dengan password database Anda
$database = "iot";

$koneksi = mysqli_connect($host, $user, $password, $database);

if (mysqli_connect_errno()) {
  echo "Koneksi database gagal: " . mysqli_connect_error();
  exit();
}
?>