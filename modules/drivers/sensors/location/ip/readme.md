# Location sensors

ECMA-419 [Location](https://419.ecma-international.org/#sensor-location) drivers.

| Module | Path | Role |
|---|---|---|
| `embedded:sensor/Location/IP` | `ip/` | City-scale fix from public IP (HTTP/HTTPS) |

---

## IP — `embedded:sensor/Location/IP`

One-shot GeoIP over HTTP(S). Conforms to the [Sensor Class Pattern](https://419.ecma-international.org/#sensor-class-pattern) and the [Location](https://419.ecma-international.org/#sensor-location) sensor class.

There is no GNSS hardware. **The application must establish network connectivity before constructing the instance.** The constructor starts a single request; `onSample` fires once if the lookup succeeds.

The HTTP client is passed in. This module does not import a network class.

```js
import IP from "embedded:sensor/Location/IP";

const loc = new IP({
	http: {
		...device.network.https.client,
		host: "ipwho.is"
	},
	onSample() {
		const s = loc.sample();		// consume-once
		if (s)
			trace(s.latitude, s.longitude, "\n");
	},
	onError(e) {
		trace(e, "\n");
	}
});
```

If `http` is omitted, the constructor uses `device.network.https.client` targeting `ipwho.is`. That still requires the HTTPS client (and a CA bundle) in the application build. Pass `device.network.http.client` instead when TLS is not wanted.

### Constructor options

| Property | Description |
|---|---|
| `http` | HTTP or HTTPS client options object with an `io` constructor. Optional; see default above. `host` and `path` may be overridden. Unknown properties are ignored. |
| `onSample` | Invoked when a sample is available. Retrieve it with `sample()`. Optional. |
| `onError` | Invoked on a non-recoverable failure. The instance is closed; only `close()` should be called. Optional. |
| `target` | Opaque value assigned to `this.target`. Optional. |

### `configure`

`configure()` is required by the Sensor Class Pattern. This driver has no settings — IP is a single fetch for the life of the instance. Unknown properties are ignored.

### `sample`

Returns a fresh object, or `undefined` if no new reading is available (not yet arrived, or already consumed).

| Property | Description |
|---|---|
| `latitude` | Decimal degrees, WGS84. Required. |
| `longitude` | Decimal degrees, WGS84. Required. |
| `accuracy` | Estimated radius in meters. Optional. When the service omits it, the driver uses 25 000 (city scale). |
| `altitude` | Meters above the WGS84 ellipsoid. Optional; only if the service provides it. |
| `timestamp` | ECMAScript time value of the lookup. Optional. |
| `timezone` | UTC offset in seconds, when the service provides it. Implementation extension. |
| `dst` | `true` if daylight saving is in effect. Implementation extension. |

`heading`, `speed`, and `altitudeAccuracy` are not produced.

### `close`

Releases the HTTP client immediately. Safe to call more than once. After `close()`, `sample()`, `configure()`, and `identification` throw. `[Symbol.dispose]` is an alias for `close()`.

### `identification`

```js
{
	model: "IP Geolocation",
	classification: "Location",
	description: "Location from public IP (GeoIP); city-scale accuracy"
}
```

### Response shape

The default path requests only the fields this driver reads (`success`, `latitude`, `longitude`, `accuracy`, `altitude`, `timezone`). A compatible endpoint should return JSON of the form:

```json
{
	"success": true,
	"latitude": 37.39,
	"longitude": -122.08,
	"accuracy": 25000,
	"timezone": { "offset": -28800, "is_dst": true }
}
```

`success: false`, missing coordinates, or a non-2xx HTTP status invokes `onError`.

### Build

Include `$(MODULES)/drivers/sensors/location/ip/manifest.json`.

The application must also include an HTTP or HTTPS client (for example `modules/io/https/client`) and, for TLS, a CA resource such as `$(MODULES)/crypt/data/ca236`.

Example: `examples/drivers/sensors/location/ip`.
