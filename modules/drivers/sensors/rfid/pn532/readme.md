# PN532 NFC / RFID (I2C)

ECMA-419 sensor driver for the NXP **PN532** over **I2C**, including generic Elechouse-style "NFC RFID Module V3" kits.

| | |
|---|---|
| Module | `embedded:sensor/RFID/PN532` |
| Default I2C address | `0x24` (7-bit; some kits print `0x48`) |
| Protocol (v1) | ISO14443 Type A UID + NDEF (if present) + MIFARE Classic block IO |

The driver is **host-agnostic**. It talks a framed command protocol, so the caller must pass `device.io.I2C` (not SMBus).

## Hardware (generic V3 kit)

Set the interface **before** applying power. Silkscreen varies; confirm I2C, not HSU.

| Mode | SW1 (SET0) | SW2 (SET1) |
|---|---|---|
| HSU (UART) | 0 | 0 |
| **I2C** | **1** | **0** |
| SPI | 0 | 1 |

### Moddable Six

Wire to `device.i2c.default` — the STEMMA QT / Qwiic jack **or** header `SDA IO4` / `SCL IO5`. The GT911 touch controller is already on this bus at `0x14` / `0x5D`; `0x24` does not collide.

| PN532 | Moddable Six |
|---|---|
| VCC | **3.3 V** (not VIN 5V) |
| GND | GND |
| SDA | IO4 |
| SCL | IO5 |
| RSTPD_N | optional Digital |
| IRQ | optional Digital — command ready (active low), not card present |

Header I2C has no onboard pull-ups. Prefer the Qwiic connector, or a kit that already has 4.7k–10k pull-ups.

Prefer bank-card-sized Type A tags. Many keyfobs are **125 kHz** and will never be seen.

## Usage

```js
import Timer from "timer";
import PN532 from "embedded:sensor/RFID/PN532";

const rfid = new PN532({
	sensor: {
		...device.i2c.default,
		address: 0x24,
		hz: 100_000,
		timeout: 1000
	}
	// interrupt: { io: device.io.Digital, pin: /* IRQ GPIO */ },
});
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

Constructor options: `sensor`, `reset`, `interrupt`, `target`. `configure({ antenna })` turns the field on or off.

`interrupt` is optional. When provided, the driver uses the IRQ pin (active low) instead of polling the I²C RDY byte after each command. It is not a card-present interrupt; the application still calls `sample()`.

The I2C `timeout` default of 1000 ms covers PN532 clock stretching during wakeup / SAM configuration.

## Sample

```js
{ present: true, uid: Uint8Array, uidHex: "04a1...", sak: number, type: string }
{ present: false }
```

`sample()` returns a new edge or `undefined` if nothing changed. `{ present: false }` means the tag left. `uidHex` is `uid.toHex()`. Polling does not read NDEF.

On-demand (re-lists the PICC). Type 2 (NTAG / Ultralight), MIFARE Classic (NFC Forum key `D3F7D3F7D3F7` or factory `FF…`), and Type 4 (ISO-DEP) are tried from the SAK. Well-known URI (`U`) and text (`T`) records are decoded; `ndef.text` is the first of those:

```js
const ndef = rfid.readNDEF();	// undefined if none
```

## NDEF write (Type 2 / Classic)

Type 2 (NTAG / Ultralight, SAK `0x00`) and MIFARE Classic (1K `0x08`, Mini `0x09`, 4K `0x18`). Type 4 write is not implemented.

Type 2: the tag must already have a Capability Container (`E1` on page 3). Lock bytes and the CC are not written.

Classic: writes the TLV to **sector 1 data blocks 4–6** (48 bytes). Tries NFC Forum key `D3F7D3F7D3F7` then factory `FFFFFFFFFFFF`. Does not format MAD / sector 0 / trailers. Use a disposable card.

```js
rfid.writeNDEF({uri: "https://www.moddable.com"});
// or: rfid.writeNDEF({text: "hello", language: "en"});
const back = rfid.readNDEF();
```

`writeNDEF()` returns the message it just read back.

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
| `start({ block, key?, keyType? })` | Opens the listed PICC crypto session. `key` is a 6-byte `Uint8Array`. |
| `readBlock(block)` | Returns `Uint8Array` of 16 bytes. |
| `writeBlock(block, data)` | `data` is a `Uint8Array` of 16 bytes. |
| `stop()` | `InRelease`. |

**Safety:** Do not write block **0** (manufacturer) or **sector trailers** (blocks 3, 7, 11, ...) unless you understand access bits — a bad trailer can brick the sector. Use a disposable test card.

MIFARE Ultralight 4-byte pages are used by `writeNDEF()`, not by `writeBlock()`.

## Examples

| Path | Platform |
|---|---|
| `examples/drivers/sensors/rfid/ui` | screen + LED; PN532, MFRC522 or ST25R3916 |
| `examples/drivers/sensors/rfid/log` | debugger only |
| `examples/drivers/sensors/rfid/write` | write one URI |

The Moddable Six example sets `config.led.rainbow` to `false` and uses `led.color = {r, g, b}` for status.
