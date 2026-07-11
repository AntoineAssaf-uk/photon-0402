cd E:/WMS/Products/Software/OpenPnp/photon-firmware/photon

$env:VERSION_STRING="section7-local"

pio run -e photon-serial

copy "E:\WMS\Products\Software\OpenPnp\Photon-Firmware\photon\.pio\build\photon-serial\firmware.bin"  "E:\WMS\Products\Software\OpenPnp\Photon-Firmware\build-output\photon-0402-section7-local.bin"
