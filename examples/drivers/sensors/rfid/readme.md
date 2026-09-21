# RFID examples

These examples use the host RFID reader:

```js
const rfid = new device.sensor.RFID;
```

M5Dial provides WS1850S/MFRC522 as `device.sensor.RFID`. StackChan provides ST25R3916.

If the host does not define `device.sensor.RFID`, construct the chip class yourself and include its manifest.

**MFRC522** (I2C `0x28`, SMBus):

```js
import MFRC522 from "embedded:sensor/RFID/MFRC522";

const rfid = new MFRC522({
	sensor: {
		...device.i2c.default,
		io: device.io.SMBus,
		address: 0x28
	}
});
```

Manifest include: `$(MODDABLE)/modules/drivers/sensors/rfid/mfrc522/manifest.json`

**PN532** (I2C `0x24`, not SMBus):

```js
import PN532 from "embedded:sensor/RFID/PN532";

const rfid = new PN532({
	sensor: {
		...device.i2c.default,
		address: 0x24,
		hz: 100_000,
		timeout: 1000
	}
});
```

Manifest include: `$(MODDABLE)/modules/drivers/sensors/rfid/pn532/manifest.json`

**ST25R3916** (I2C `0x50`, SMBus):

```js
import ST25R3916 from "embedded:sensor/RFID/ST25R3916";

const rfid = new ST25R3916({
	sensor: {
		...device.i2c.default,
		io: device.io.SMBus,
		address: 0x50,
		hz: 400_000
	}
});
```

Manifest include: `$(MODDABLE)/modules/drivers/sensors/rfid/st25r3916/manifest.json`

Call `sample()` from a timer in the application. There is no card-present interrupt.

| Path | What it does |
|---|---|
| `log` | UID / NDEF to the debugger |
| `ui` | screen; button A writes a URI |
| `write` | writes one URI to each tag presented |
