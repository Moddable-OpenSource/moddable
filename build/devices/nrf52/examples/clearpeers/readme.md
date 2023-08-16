This app clears the BLE state stored in the nRF52 flash. This is useful for testing and when the state somehow gets corrupted.

The BLE state is stored in the virtual flash partition "ble-peers". The location of this partition is determined by the Flash module at $MODDABLE/modules/files/flash/nrf52/flash.c.
