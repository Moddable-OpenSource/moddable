@echo off

REM Use like: nrfSerialDeploy COM47 ...\hexfile ...\packagefile

@echo "# Packaging xs_nrf52.hex"
adafruit-nrfutil dfu genpkg --dev-type 0x0052 --application %2 %3
@echo "# Flashing xs_nrf52.hex"
adafruit-nrfutil --verbose dfu serial --package %3 -p %1 -b 115200 --singlebank --touch 1200
