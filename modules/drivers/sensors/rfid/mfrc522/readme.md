# MFRC522 / WS1850S RFID (I2C)

ECMA-419 sensor driver for NXP **MFRC522**-class readers over **I2C**, including the **WS1850S** used on M5Dial.

| | |
|---|---|
| Module | `embedded:sensor/RFID/MFRC522` |
| Default I2C address | `0x28` |
| Protocol (v1) | ISO14443 Type A UID + NDEF (Type 2 / Classic) + MIFARE Classic block IO |

The driver is **host-agnostic**. Platforms that include the reader wire it as `device.sensor.RFID` (see M5Dial). M5StackChan's built-in NFC is an ST25R3916, not this chip -- see `embedded:sensor/RFID/ST25R3916`.

## M5Dial hardware

| Signal | GPIO | Notes |
|---|---|---|
| SDA | 11 | `device.I2C.internal` |
| SCL | 12 | |
| RST | 8 | **Shared with LCD_RESET** - driver uses **soft reset only** |
| IRQ | 10 | Unused in poll mode |

Prefer bank-card-sized Type A tags; very small stickers couple poorly (M5 docs). Many keyfobs are **125 kHz** and will never be seen.

## Usage (host-provided)

Preferred on M5Dial:

```js
import Timer from "timer";

const rfid = new device.sensor.RFID;
Timer.repeat(() => {
	const s = rfid.sample();
	if (!s)
		return;
	if (s.present)
		trace(s.uidHex, "\n");
	else
		trace("removed\n");
}, 250);
```

There is no card-present interrupt. The application calls `sample()`.

## Usage (explicit construction)

```js
import Timer from "timer";
import MFRC522 from "embedded:sensor/RFID/MFRC522";

const rfid = new MFRC522({
	sensor: {
		...device.I2C.internal,
		io: device.io.SMBus,
		address: 0x28
	}
});
Timer.repeat(() => {
	const s = rfid.sample();
	if (s?.present)
		trace(s.uidHex, "\n");
}, 250);
```

Constructor options: `sensor`, `reset`, `target`. `configure({ antenna })` turns the field on or off.

## Sample

```js
{ present: true, uid: Uint8Array, uidHex: "04a1...", sak: number, type: string }
{ present: false }
```

`sample()` returns a new edge or `undefined` if nothing changed. `{ present: false }` means the tag left; `undefined` means still empty or the same tag still in the field. `uidHex` is `uid.toHex()`.

NDEF is on demand via `readNDEF()` / `writeNDEF({ uri })`.

## Configuration

```js
rfid.configure({antenna: false});	// power-save
rfid.configuration;					// { antenna }
```

## MIFARE Classic read / write

Factory keys are usually `FF FF FF FF FF FF` (Key A). `start` the **sector that contains the block**, then read/write 16-byte `Uint8Array`s.

```js
rfid.start({block: 4});					// default Key A all-FFs
const data = rfid.readBlock(4);			// Uint8Array(16)
rfid.writeBlock(4, new Uint8Array(16));
rfid.stop();
```

| Method | Notes |
|---|---|
| `start({ block, key?, keyType? })` | Opens Crypto1. `key` is a 6-byte `Uint8Array`. `keyType` `"A"` (default) or `"B"`. |
| `readBlock(block)` | Returns `Uint8Array` of 16 bytes. |
| `writeBlock(block, data)` | `data` is a `Uint8Array` of 16 bytes. |
| `stop()` | Clears MFCrypto1. |

**Safety:** Do not write block **0** (manufacturer) or **sector trailers** (blocks 3, 7, 11, ...) unless you understand access bits - a bad trailer can brick the sector. Use a disposable test card.

MIFARE Ultralight (4-byte pages, no Crypto1) is not covered by these methods yet.

## Examples

| Path | Platform |
|---|---|
| `examples/drivers/sensors/rfid/ui` | screen + LED; MFRC522, PN532, or ST25R3916 |
| `examples/drivers/sensors/rfid/log` | debugger only |
| `examples/drivers/sensors/rfid/write` | write one URI |
