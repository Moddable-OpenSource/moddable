# ECMA-419 FT6206 Touch Driver
Updated: November 16, 2022

This directory contains two separate implementations of the FT6206 touch driver. Both conform to the [Sensor Class Pattern](https://419.ecma-international.org/#-sensor-class-pattern) of ECMA-419 and the [Touch](https://419.ecma-international.org/#-sensor-classes-touch) sensor class.

The implementation in ft6206.js uses synchronous SMBus I/O. This should work on all ECMA-419 deployments that support SMBus. The implementation in ft6206_async.js uses asynchronous SMBus I/O, introduced as part of ECMA-419 2nd Edition. It only works on ECMA-419 deployments that provide the `SMBus.Async` constructor.

The asynchronous implementation has the advantage of never significantly blocking the main event loop. The synchronous implementation has the advantage of being smaller and using less RAM at runtime.


To use the synchronous implementation in projects include the `manifest.json` in this directory; to use the asynchronous implementation, include `manifest_async.json`.
