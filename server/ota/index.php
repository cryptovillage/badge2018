<?php
$update_available = false;
$resp = "{";
// The virgin EFM8 bootloader has a CRC of 16fc
if ($_GET["efm8boot"] == "16fc") {
  $resp .= "\"efm8_bootloader\": {\"url\": \"https://2018.badge.cryptovillage.org/ota/ebac77ca2e8347a92a622ad59ab6995321246d29a9e2c29af6e1d3402de5e1d1.boot\", \"sha256\": \"ebac77ca2e8347a92a622ad59ab6995321246d29a9e2c29af6e1d3402de5e1d1\"}, ";
  $update_available = true;
}
// ebef is the firmware in cpv2018-prod.zip. 7b91 is the firmware in cpv2018-prototest.zip.
if ($_GET["efm8app"] == "ebef" || $_GET["efm8app"] == "7b91") {
  $resp .= "\"efm8_app\": {\"url\": \"https://2018.badge.cryptovillage.org/ota/2000ef3237790ff7c2f081a2e56450eb2838d8b7324400cac92229ea5dafd0cf.boot\", \"sha256\": \"2000ef3237790ff7c2f081a2e56450eb2838d8b7324400cac92229ea5dafd0cf\"}, ";
  $update_available = true;
}
// This just forces an update. Used for testing the OTA process.
//if ($_GET["partlabel"] == "ota0") {
//  $resp .= "\"app\": {\"url\": \"https://2018.badge.cryptovillage.org/ota/9d00d76f643a666d3c4b20819dad4459062f393eb48a6e272830eddfdffce4ac.bin\", \"sha256\": \"9d00d76f643a666d3c4b20819dad4459062f393eb48a6e272830eddfdffce4ac\", \"size\": 984880}, ";
//  $update_available = true;
//}
if ($update_available) {
  $resp .= "\"update_available\": true}";
} else {
  $resp .= "\"update_available\": false}";
}
echo($resp);
?>
