# modules/embedded

Implementations of the ECMA-419 (ECMAScript® embedded systems API) modules, organized to mirror the module specifiers:

| Directory | Specifiers |
|---|---|
| `io` | `embedded:io/*` – digital, analog, i2c, smbus, spi, pwm, serial, pulsecount, pulsewidth, audio, bluetoothle, image, socket |
| `network` | `embedded:network/*` – dns, dnssd, http, mqtt, ntp, ws, interface (wifi, ethernet); `https`, `mqtts`, `wss` bundle a client with TLS |
| `storage` | `embedded:storage/*` – files, flash, key-value |
| `provider` | `embedded:provider/builtin` for the simulators |
| `crypt` | `embedded:crypt/*` |
| `system`, `update`, `display` | `embedded:system`, `embedded:update`, `embedded:display/*` |
| `common` | native helpers shared by the implementations |

Manifests reference this tree as `$(EMBEDDED)`, defined by `examples/manifest_base.json`; `$(IO)` is `$(EMBEDDED)/io`.

The modules were previously at `modules/io`. The old manifest paths remain as forwarding manifests that warn `… moved to …`.

The specification is at <https://419.ecma-international.org>. Sensor, RTC, and peripheral drivers are in [modules/drivers](../drivers).
