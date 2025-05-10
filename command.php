<?php
if (file_exists('adjie/command.txt')) {
  $command = file_get_contents('adjie/command.txt');

  // Hapus isi file setelah membaca
  file_put_contents('adjie/command.txt', ''); // Menulis string kosong ke file

  echo $command;
} else {
  echo ""; // Mengembalikan string kosong jika tidak ada perintah
}
?>