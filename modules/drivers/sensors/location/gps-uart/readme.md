# GPS-UART — `embedded:sensor/Location/GPS-UART`

GNSS over UART. Conforms to the [Sensor Class Pattern](https://419.ecma-international.org/#sensor-class-pattern) and the [Location](https://419.ecma-international.org/#sensor-location) sensor class.

The serial connection and the sentence/frame parser are passed in. This module does not import an IO class or a protocol.

```js
import GPS from "embedded:sensor/Location/GPS-UART";
import NMEA from "data/location/nmea";
// import UBX from "data/location/ubx";

const gps = new GPS({
	sensor: {
		...device.Serial.default,
		io: device.io.Serial,
		baud: 9600
	},
	protocol: NMEA,
	onSample() {
		const s = gps.sample();		// consume-once
		if (!s)
			return;					// undefined ⇒ no new fix, or fix lost
		trace(s.latitude, s.longitude, "\n");
	},
	onError(e) {
		trace(e, "\n");
	}
});
```

### Constructor options

| Property | Description |
|---|---|
| `sensor` | Serial constructor options with an `io` constructor. Required. Default `baud` 9600. The driver overrides `format`, `target`, and `onReadable`. |
| `protocol` | Parser constructor (`data/location/nmea`, `data/location/ubx`, or another with the same surface). Required. |
| `onSample` | Invoked when a sample is available, and once when a fix is lost. Retrieve it with `sample()`. Optional. |
| `onError` | Invoked on a non-recoverable failure. The instance is closed; only `close()` should be called. Optional. |
| `target` | Opaque value assigned to `this.target`. Optional. |

### Protocol

A protocol is a constructor with:

| Member | Role |
|---|---|
| `new Protocol(onParsed)` | Incremental parser. `onParsed` receives each complete sentence or frame. |
| `push(buffer \| byte)` | Feed UART bytes. |
| `location(parsed)` | Location-oriented update, or `undefined`. `{ fix: false }` after a valid fix means the fix was lost. |
| `identification` | Object used by the sensor `identification` getter. |
| `setup()` | Optional. Returns command frames written after open (UBX enables NAV-PVT). |

### `configure`

| Property | Description |
|---|---|
| `timeout` | Milliseconds to wait for the first published fix before invoking `onError` and closing. Optional; default is no timeout. |
| `maximumAge` | Milliseconds. `sample()` drops a pending sample older than this. Optional; `0` (default) does not age-filter. |
| `enableHighAccuracy` | Accepted for Location API alignment; currently a no-op. Unknown properties are ignored. |

`timeout` applies only until the first fix. After that, loss of fix is reported through `onSample` (`sample()` returns `undefined`), not `onError`.

`enableHighAccuracy` would be protocol-specific (UBX CFG messages; NMEA vendor `$PMTK` / `$PCAS` and similar). Until then it is silently ignored.

### `sample`

Returns a fresh object, or `undefined` if no new reading is available (not yet arrived, already consumed, aged out, or fix lost).

| Property | Description |
|---|---|
| `latitude` | Decimal degrees, WGS84. Required. |
| `longitude` | Decimal degrees, WGS84. Required. |
| `accuracy` | Estimated radius in meters. Optional. |
| `altitude` | Meters above the WGS84 ellipsoid. Optional. |
| `altitudeAccuracy` | Estimated accuracy of `altitude` in meters. Optional. |
| `heading` | Direction of travel in degrees clockwise from true north, `[0, 360)`. Optional. |
| `speed` | Horizontal velocity in meters per second. Optional. |
| `timestamp` | ECMAScript time value of the fix. Optional. |

NMEA fills `accuracy` from HDOP, `speed` from RMC/VTG (knots converted to m/s), and `heading` from course. UBX NAV-PVT provides `accuracy`, `altitudeAccuracy`, `speed`, and `heading` directly.

### Fix lost

When the protocol reports `fix: false` after a valid fix (NMEA void/no-fix sentences, or UBX NAV-PVT without a fix), the driver clears cached coordinates and calls `onSample` once. `sample()` then returns `undefined`.

### `close`

Releases the serial connection and timeout timer immediately. Safe to call more than once. After `close()`, `sample()`, `configure()`, and `identification` throw. `[Symbol.dispose]` is an alias for `close()`.

### `identification`

Taken from the protocol, for example:

```js
{
	model: "NMEA-0183 GNSS",
	classification: "Location",
	description: "Generic NMEA-0183 GNSS receiver"
}
```

### Build

Include `$(MODULES)/drivers/sensors/location/gps-uart/manifest.json` and one or both parsers:

- `$(MODULES)/data/location/nmea/manifest.json`
- `$(MODULES)/data/location/ubx/manifest.json`

Example: `examples/drivers/sensors/location/gps-uart`.
