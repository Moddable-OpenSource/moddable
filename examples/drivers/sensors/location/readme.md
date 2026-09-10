# Location examples

| Path | Driver | Notes |
|---|---|---|
| `gps-uart/` | `embedded:sensor/Location/GPS-UART` | UART; NMEA or UBX |
| `gps-i2c/` | `embedded:sensor/Location/GPS-I2C` | I²C (u-blox DDC); NMEA or UBX |
| `ip/` | `embedded:sensor/Location/IP` | HTTPS GeoIP |

Module docs: `modules/drivers/sensors/location/gps-uart/readme.md`, `modules/drivers/sensors/location/gps-i2c/readme.md`, `modules/drivers/sensors/location/ip/readme.md`.

---

## GPS UART

```sh
cd $MODDABLE/examples/drivers/sensors/location/gps-uart
mcconfig -d -m -p esp32/moddable_six
```

### Config (`mc/config`)

```json
"config": {
	"gps": {
		"protocol": "nmea",
		"baud": 9600
	}
}
```

| Key | Values | Default |
|---|---|---|
| `gps.protocol` | `"nmea"` \| `"ubx"` | `"nmea"` |
| `gps.baud` | number | `9600` |

Override on the command line, for example:

```sh
mcconfig -d -m -p esp32/moddable_six gps.protocol=ubx
```

**Wiring (typical):** module TX → MCU RX (`device.Serial.default`), common GND, 3.3 V (check module).

On `onSample`, `sample()` returns a fix object or `undefined` (including **fix lost**). First fix can take minutes (sky view, cold start); the example does not set `timeout`.

---

## GPS I²C

```sh
cd $MODDABLE/examples/drivers/sensors/location/gps-i2c
mcconfig -d -m -p esp32/moddable_six
```

### Config (`mc/config`)

```json
"config": {
	"gps": {
		"protocol": "nmea"
	}
}
```

| Key | Values | Default |
|---|---|---|
| `gps.protocol` | `"nmea"` \| `"ubx"` | `"nmea"` |

```sh
mcconfig -d -m -p esp32/moddable_six gps.protocol=ubx
```

**Wiring (typical):** u-blox DDC on `device.I2C.default`, address `0x42`.

On `onSample`, `sample()` returns a fix object or `undefined` (including **fix lost**). First fix can take minutes (sky view, cold start); the example does not set `timeout`.

---

## IP

```sh
cd $MODDABLE/examples/drivers/sensors/location/ip
mcconfig -d -m -p esp32/moddable_six
```

Requires Wi‑Fi (or other) connectivity **before** the example runs. Includes:

- `examples/manifest_net.json`
- `examples/io/tcp/httpsclient/manifest_httpsclient.json`
- CA resource `ca236` for TLS

Uses `device.network.https.client` toward `ipwho.is`. One sample per instance lifetime.
