# Location examples

| Path | Driver | Notes |
|---|---|---|
| `ip/` | `embedded:sensor/Location/IP` | HTTPS GeoIP |

Module docs: `modules/drivers/sensors/location/readme.md`.

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
