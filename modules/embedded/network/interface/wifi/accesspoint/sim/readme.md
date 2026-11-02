# Simulated Wi-Fi Access Point Network Interface
Copyright 2026 Moddable Tech, Inc.<BR>
Revised: August 10, 2026

This is the simulator implementation of the ECMA-419 [Wi-Fi Access Point Network Interface](https://419.ecma-international.org/#network-interface-class-pattern), used when building for `mac`, `lin`, and `win` — including `mcconfig -p sim`. It lets a project exercise its access point flows — bringing the access point up, serving something at its address, reacting to a station joining and leaving — without hardware.

The access point is entirely simulated. Nothing here touches the host computer's Wi-Fi radio, no device can really associate with it, and closing it has no effect on the host's networking.

## Bringing it up

The constructor validates its options exactly as the device implementations do — SSID required and non-empty, a password of at least 8 characters, a known `authentication` value that requires a password unless it is `"none"`, `channel` 1–13, `max` 1–10, `interval` 100–60000 — and only one access point may be open at a time.

`connection` reaches 400 about 250ms after the constructor returns, at which point `onChanged("connection")` is called. That is much faster than real hardware, so the edit-run cycle stays short.

## The simulated station

About two seconds after the access point is running, one station joins and `onConnect()` is called. Its `address` is `undefined` at that moment and is assigned about 250ms later, the same order a real station is admitted and then given an address by DHCP — so a project that reads `station.address` too early fails here the way it would on a device.

The station stays until the project calls `station.close()`, which reports `onDisconnect()`. As on a device, the station instance is readable during `onDisconnect()` and throws afterwards. Closing the access point drops the station without calling `onDisconnect()`.

Only one station is simulated, and no further stations join after it leaves.

## Addresses

The `address` and `MAC` properties report the host computer's real IP and MAC addresses rather than invented ones, so an HTTP server the project starts really is reachable at the address the access point advertises — from a browser on the host, or from another machine on the same network. This is the same choice the [simulated Wi-Fi station interface](../../wifi/sim/readme.md) makes, and it shares that module's native helper for reading the host interface.

The cost is one piece of realism: a real access point is its own gateway on its own subnet, typically at `192.168.4.1`. Here it is wherever the host computer happens to be.

The simulated station is given the host's subnet with a last octet of 200 — host `192.168.4.31` gives a station at `192.168.4.200` — so the two addresses read sensibly together.

`configure({portal})` validates the URL and does nothing else, since there is no DHCP server here to hand it to.
