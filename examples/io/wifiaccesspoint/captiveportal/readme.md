# Captive Portal
Copyright 2026 Moddable Tech, Inc.<BR>
Revised: August 10, 2026

A captive portal for device provisioning. The device brings up a Wi-Fi access point with a one-time SSID and password, redirects everything to itself with a small DNS server, and serves a page over HTTP. A phone joins the access point, opens the page, picks a network from a scan, and sends its credentials back over a WebSocket. The device connects to that network and reports the result.

The project supplies the pages through `onPage()` and follows the flow through `onStatus()`:

| Phase | Meaning |
| :--- | :--- |
| `initializing` | scanning, before the access point is up |
| `ready` | access point running; the detail carries its SSID and password |
| `connected` | a device joined the access point |
| `waiting` | the device left again |
| `connecting` | credentials received; connecting to that network |
| `provisioned` | connected to the chosen network |
| `failed` | the credentials did not work |

## In the simulator

Building for `mac`, `lin`, or `win` — including `mcconfig -p sim` — substitutes [captiveportal-sim.js](./captiveportal-sim.js) for [captiveportal.js](./captiveportal.js). The project's source does not change.

The simulated portal runs the same flow and reports the same phases, but there is **no HTTP server, no DNS server, and no WebSocket**: none of them would be useful, since no phone can join a simulated access point, and both port 53 and port 80 are privileged on a desktop.

In their place a ghost client stands in for the phone. It arrives as the station the [simulated access point](../../../../modules/embedded/network/interface/wifi/accesspoint/sim/readme.md) produces, then works through what a browser would do, a step at a time:

1. requests `/` — the project's `onPage()` is called exactly as the HTTP server would call it
2. requests a path that does not exist, which a real portal answers with a redirect to itself
3. asks for a Wi-Fi scan, which the [simulated Wi-Fi station interface](../../../../modules/embedded/network/interface/wifi/sim/readme.md) answers
4. chooses the network with the strongest signal and sends credentials, taking the portal through `connecting` to `provisioned`
5. terminates the portal, which closes it and calls `onClose()`

The ghost checks what `onPage()` returns and reports problems to `onError()` — a portal that cannot serve its own home page, or a page missing `content` or `mimeType`. Those are the failures that otherwise appear only as a blank browser on a phone.

The whole flow takes about five seconds.

The ghost always supplies a password the simulated Wi-Fi accepts, so the flow ends in `provisioned`. To see the `failed` phase instead, connect to a network that rejects it — see the reserved password in the [simulated Wi-Fi station interface](../../../../modules/embedded/network/interface/wifi/sim/readme.md).
