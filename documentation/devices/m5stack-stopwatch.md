# M5StopWatch
Copyright 2026 Moddable Tech, Inc.<BR>
Revised: November 23, 2026

Use the `esp32/m5stack_stopwatch` target for the M5StopWatch. The ESP32-S3 board has 16 MB of flash, 8 MB of PSRAM, and a 466 × 466 round AMOLED touchscreen.

## Build and deploy

Set up the [ESP32 toolchain](./esp32.md). From the SDK checkout, build the hardware example without writing to the device.

```sh
cd "$MODDABLE/examples/devices/m5stack-stopwatch"
mcconfig -m -p esp32/m5stack_stopwatch -t build
```

Standard deployment writes the SDK bootloader, partition table, and application. The factory firmware can use a different partition layout. If you need to preserve existing firmware or data, back up the flash and verify the device's partition table before you write to it. For an application-only update, write the built application to the verified active application slot after checking that the image fits.

On a device you intend to provision with the SDK partition table, connect a USB Type-C data cable. Hold the power button until the green LED blinks, then release it to enter download mode. Run the deployment command and press the power button to restart the board.

```sh
mcconfig -m -p esp32/m5stack_stopwatch -t deploy
```

## Verify the hardware

1. Check that the screen shows `M5StopWatch` and an M5PM1 power-source value.
2. Touch the screen and check that yellow marks appear under your finger.
3. Press the blue B button. Check that the background changes and the marks clear.
4. Press the yellow A button and speak while the screen reads `Recording`. Listen for a short tone followed by your recording on the speaker and check that the screen reports a microphone peak. Repeat the check to verify that the app can reopen both audio directions.
5. With USB disconnected, double-press the power button to turn the device off. Press it once to boot the example again. [M5StopWatch power and download controls](https://docs.m5stack.com/en/core/StopWatch#power-onoff)

## Device interfaces

| Hardware | Interface |
| --- | --- |
| CO5300 display | `screen`, RGB565BE |
| Touch controller | `device.sensor.Touch` |
| A and B buttons | `device.peripheral.button.A`, `device.peripheral.button.B` |
| M5PM1 power controller | `power.status` |
| ES8311 audio | `device.audio.Input`, `device.audio.Output`, `device.audio.Output.Async` |

The audio endpoints use 16 kHz, mono, 16-bit PCM. Close the active endpoint before opening the other direction. Use these board endpoints for the built-in microphone and speaker. They initialize the codec and control the amplifier.
