del /q Objects\*

c:\Keil_v5\UV4\uv4 -b Charger6kW_8.release.uvprojx -t "AER07" -o "release-build.log"
bootloader-tools\hex2bin.exe Objects\Aer_07v2-release.hex
bootloader-tools\ba-fw-builder.exe -ib Objects\Aer_07v2-release.bin -t aer07g302 -o aer07g302
bootloader-tools\srec_cat.exe ba.hex -Intel Objects\Aer_07v2-release.bin -Binary -offset 0x08001800 -o aer07g302.release.hex -Intel

pause