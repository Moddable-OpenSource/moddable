# Simulated Wi-Fi Network Interface
Copyright 2026 Moddable Tech, Inc.<BR>
Revised: August 10, 2026

This is the simulator implementation of the ECMA-419 [Wi-Fi Network Interface](https://419.ecma-international.org/#network-interface-class-pattern-wi-fi-network-interface), used when building for `mac`, `lin`, and `win` — including `mcconfig -p sim`. It lets a project exercise its Wi-Fi flows — scanning, connecting, handling a failed connection, reacting to disconnection — without hardware.

The connection is entirely simulated. Nothing here touches the host computer's Wi-Fi radio, and connecting or disconnecting has no effect on the host's networking: sockets work in the simulator whether or not the simulated Wi-Fi is connected. Access point mode is a separate module, simulated in [modules/embedded/network/interface/wifi/accesspoint/sim](../../wifiaccesspoint/sim/readme.md).

## Simulated access points

`scan()` always finds the same fixed list of access points.

| SSID | Channel | Security | RSSI |
| :--- | ---: | :--- | ---: |
| Moddable Guest | 1 | wpa2_psk | -47 |
| 468 8th - slow | 3 | wpa2_psk | -84 |
| JAMS2 | 6 | wpa2_psk | -72 |
| xfinitywifi | 6 | none | -79 |
| baja | 8 | wpa2_psk | -81 |
| baja | 8 | wpa2_psk | -88 |
| Breathe | 9 | wpa3_psk | -85 |
| Breathe-Guest | 9 | none | -84 |
| 468 8th - slow | 10 | wpa2_psk | -23 |
| HOME-608A_2GEXT | 11 | wpa_wpa2_psk | -86 |
| LBBSB | 11 | wpa2_psk | -85 |

Two names appear twice, as they would in a mesh network, so that code which de-duplicates scan results has something to de-duplicate.

## Connecting

`connect()` succeeds only for an SSID in the list above. A secured access point additionally requires a password between 8 and 63 characters. Anything else fails: `connection` advances to 300 and then falls back to 200, exactly as a real failed connection does.

The password `"fail"` is reserved. It always fails, for any access point, so that a project can test how it handles a rejected password separately from an access point that isn't there at all.

That makes every path reachable without editing the module:

```js
wifi.connect({SSID: "Moddable Guest", password: "12345678"});	// connection: 300, 400, then 500
wifi.connect({SSID: "Moddable Guest", password: "fail"});	// connection: 300, then 200
wifi.connect({SSID: "no such network"});			// connection: 300, then 200
```

`connect()` reaches 400 about 250ms after it is called, and 500 about 250ms later. A scan takes about 600ms. These are much faster than real hardware so that the edit-run cycle stays short.

## Addresses

The `address` and `MAC` properties report the host computer's real IP and MAC addresses rather than invented ones, so that code which binds to, advertises, or displays the address gets an address that works.

A consequence: if the host computer has no network interface at all, the simulated connection stops at 400 — connected, but with no address assigned — which is how a real connection behaves when DHCP fails.

`configure()` accepts a static address, which is then reported by `address`. Because the connection is simulated, the static address is only reported, not used for anything: sockets continue to use the host's networking.
