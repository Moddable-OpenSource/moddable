# ST25R3916 NFC / RFID (I2C)

ECMA-419 sensor driver for the ST **ST25R3916** / **ST25R3917** over **I2C**.

| | |
|---|---|
| Module | `embedded:sensor/RFID/ST25R3916` |
| Default I2C address | `0x50` (7-bit) |
| Protocol (v1) | ISO14443 Type A UID + Type 2 NDEF (NTAG / Ultralight) |

The driver is **host-agnostic**. Use `device.io.SMBus`. No IRQ pin is required: interrupt status is polled on I2C, matching [pguyot/st25r391x](https://github.com/pguyot/st25r391x) (Linux, I2C-only).

MIFARE Classic `start` / sector IO is **not** implemented. This frontend has no Crypto1 engine; M5Unit-NFC does Classic in software.

## Hardware

### M5Stack StackChan (built-in)

[M5 StackChan pin map](https://docs.m5stack.com/en/StackChan): body I2C is CoreS3 `G12` SDA / `G11` SCL (`device.i2c.internal`). NFC is **ST25R3916-AQWT** at **0x50**, next to Si12T (`0x68`) and the PY32 expander (`0x6F`).

The host provider maps `device.sensor.RFID` to this bus.

### M5Stack Unit NFC (SKU U216)

Grove I2C, same chip and address. M5 default clock is **400 kHz**. Software I2C is too slow for RF timing (M5 note on NessoN1 port B) -- use hardware I2C.

### Identity

Register `0x3F`: type bits `[7:3] == 0x05` (ST25R3916/7). Linux checks the exact byte `0x2A` (rev 2); this driver accepts any revision with that type, as M5Unit-NFC does.

## Usage

```js
import Timer from "timer";
import ST25R3916 from "embedded:sensor/RFID/ST25R3916";

const rfid = new ST25R3916({
	sensor: {
		...device.i2c.default,
		io: device.io.SMBus,
		address: 0x50,
		hz: 400_000
	}
});
Timer.repeat(() => {
	const s = rfid.sample();
	if (s?.present)
		trace(s.uidHex, "\n");
	else if (s)
		trace("removed\n");
}, 250);
```

On StackChan:

```js
import Timer from "timer";

const rfid = new device.sensor.RFID;
Timer.repeat(() => {
	const s = rfid.sample();
	if (s?.present)
		trace(s.uidHex, "\n");
}, 250);
```

Constructor options: `sensor`, `reset`, `target`. `configure({ antenna })` turns the field on or off. There is no card-present interrupt; the application calls `sample()`.

## Sample

```js
{ present: true, uid: Uint8Array, uidHex: "04a1...", sak: number, type: string }
{ present: false }
```

`sample()` returns a new edge or `undefined` if nothing changed. `{ present: false }` means the tag left. `uidHex` is `uid.toHex()`.

```js
const ndef = rfid.readNDEF();	// undefined if none
```

## NDEF write (Type 2)

NTAG / Ultralight only (SAK `0x00`). The tag must already have a Capability Container (`E1` on page 3). Lock bytes and the CC are not written.

```js
rfid.writeNDEF({uri: "https://www.moddable.com"});
```

## Configuration

```js
rfid.configure({antenna: false});	// field off (oscillator stays ready until close)
rfid.configuration;					// { antenna }
```

## I2C command map

From the ST25R3916 datasheet, used the same way by M5Unit-NFC and the Linux driver:

| First byte | Meaning |
|---|---|
| `00xxxxxx` | write Space A register |
| `01xxxxxx` | read Space A register |
| `0x80` | load FIFO |
| `0x9F` | read FIFO |
| `11xxxxxx` | direct command (`sendByte`) |
| `0xFB` | Space B access (next byte is the B register) |
| `0xFC` | test-register access |

Init: `Set default` (`0xC0`), test write `04 10` (overheat), IO config, identity check, oscillator, `Adjust regulators`, ISO14443A analog setup, field on.

Poll: `Transmit REQA`/`WUPA`, anticollision (`antcl` + TX without CRC), SELECT with CRC, optional Type 2 READ.

## Examples

| Path | Platform |
|---|---|
| `examples/drivers/sensors/rfid/ui` | screen + LED; host RFID / PN532 / MFRC522 / ST25R3916 |
| `examples/drivers/sensors/rfid/log` | debugger only |
| `examples/drivers/sensors/rfid/write` | write one URI |

## References

- [M5Unit-NFC](https://github.com/m5stack/M5Unit-NFC) (Unit NFC U216, I2C `0x50`, 400 kHz)
- [pguyot/st25r391x](https://github.com/pguyot/st25r391x) (Linux I2C, no IRQ)
- [M5 StackChan](https://docs.m5stack.com/en/StackChan) pin map
