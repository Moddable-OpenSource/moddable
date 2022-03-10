This folder contains a host and mods that demonstrate how to build BLE HID Keyboards and Media Controllers with the Moddable SDK. 

For a high-level overview, refer to [this blog post](https://blog.moddable.com/blog/ble-keyboards-media/). This document is documentation for using the BLE HID Host to make new keyboard and media controller apps.

## Using the BLE HID Host

BLE HID support is implemented as a host — a Moddable SDK application that expects to be extended by a [mod](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/xs/mods.md). The BLE HID Host has built-in support for creating a BLE Server, managing connections to a BLE central, and generating HID input reports. But, the BLE HID Host has only a minimal UI built in to it that informs the user that they need to install a mod.

When run, the BLE HID Host checks for the presence of a mod with a module specifier `"UI"`. If it finds such a module, it looks for an export named `container` in that module (which should be a [Piu Template](https://github.com/Moddable-OpenSource/moddable/blob/public/documentation/piu/piu.md#templates)), instantiates an new instance of that Template, and adds it to the application. The mod then has total control over the look and feel of the application while leaving the BLE management to the host.

Of course, the UI mod must be able to request that input events (e.g. keystrokes and button presses) be sent to the connected BLE Central. The BLE HID Host implements three Piu events on the `application` object for this purpose, described in the API documentation below.

The UI mod also needs to know when a BLE Central has connected and disconnected. These notifications are sent to the UI mod container as events, also described below.

### Current BLE HID Host Capabilities

An HID peripheral must describe its own input capabilities to the BLE central it is connected to. In the current implementation of the BLE HID Host, these capabilities are fixed to:
 - a standard 101 English language keyboard with basic ASCII characters and 8 modifiers
 - a fixed set of 8 "Consumer Device" media controls

The implemented HID Report Descriptor is provided for reference at the end of this document.

## BLE HID Host API 

### Host Events 

Three events are available on the BLE HID Host's `application` object and can be triggered by the UI mod to send keystrokes/button presses to a connected BLE Central:

**`doKeyDown(application, options)`**
**`doKeyUp(application, options)`**
**`doKeyTap(application, options)`**

| Argument | Type | Description |
| --- | --- | :--- |
| `application` | `application` |  The `application` object that triggered the event
| `options` | `object` | An object with properties to indicate what keys should be sent to the BLE Central as described below.

The `doKeyDown` and `doKeyUp` events send a notification to the BLE Central that a key has been depressed and released, respectively. The `doKeyTap` event is provided as a convenience and simply sends the key down and key up events sequentially.

#### Options Object

| Property | Type | Description |
| --- | --- | :--- |
| `character` | `string` | A single-character string indicating an ASCII character to send via a keyboard report to the BLE Central. (Optional)
| `modifiers` | `number` | A number with a standard keyboard 8-bit modifier mask. Can be constructed as a bitwise OR of the options listed below. (Optional)
| `hidCode`   | `object` | An object that specifies an HID code point to send to the BLE Central by usage page and usage ID, as described below. (Optional)

At least one of `character` or `hidCode` must be included in the options object, or execution of the event will have no effect.

##### hidCode Object

| Property | Type | Description |
| --- | --- | :--- |
| `page` | `number` | The Usage Page of the key press to report to the BLE Central.
| `id`   | `number` | The Usage ID of the key press to report to the BLE Central.

> Note: For Usage Pages and IDs, see the [USB HID Usage Tables document](https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf).

### Mod Events

The mod container may include two events that will be triggered by the host to indicate that a BLE Central has connected or disconnected:

**`onKeyboardBound(container)`**
**`onKeyboardUnbound(container)`**

| Argument | Type | Description |
| --- | --- | :--- |
| `container` | `container` |  The `container` object that triggered the event

It is recommended that UI mods use these event triggers to update the state of the displayed user interface to indicate whether or not controls can currently be used.

### HID Constants Provided to Mods

The BLE HID Host provides an object as an argument when constructing instances of the UI mod container. This object contains useful constants for the keyboard modifiers and media key HID code points supported by the host:

| Property | Type | Description |
| --- | --- | :--- |
| `hidKeys` | `object` | Dictionary of HID Consumer Device code points supported by the host as described below
| `modifiers` | `object` | Dictionary of keyboard modifiers supported by the host as described below

#### hidKeys Object

| Property | Type | Description |
| --- | --- | :--- |
| `VOLUME_UP` | `object` | Object with one property `HID` that indicates the HID code point for **Volume Up** (plus additional state that maps the HID code to the internal report representation)
| `VOLUME_DOWN` | `object` | Object with one property `HID` that indicates the HID code point for **Volume Down** (plus additional state that maps the HID code to the internal report representation)
| `MUTE` | `object` | Object with one property `HID` that indicates the HID code point for **Mute** (plus additional state that maps the HID code to the internal report representation)
| `BACK` | `object` | Object with one property `HID` that indicates the HID code point for **Previous Track** (plus additional state that maps the HID code to the internal report representation)
| `FORWARD` | `object` | Object with one property `HID` that indicates the HID code point for **Next Track** (plus additional state that maps the HID code to the internal report representation)
| `PLAY` | `object` | Object with one property `HID` that indicates the HID code point for **Play** (plus additional state that maps the HID code to the internal report representation)
| `PLAYPAUSE` | `object` | Object with one property `HID` that indicates the HID code point for **Play/Pause** (plus additional state that maps the HID code to the internal report representation)
| `SHUFFLE` | `object` | Object with one property `HID` that indicates the HID code point for **Shuffle** (plus additional state that maps the HID code to the internal report representation)

#### modifiers Object

| Property | Type | Description |
| --- | --- | :--- |
| `LEFT_CONTROL` | `number` | Bitmask for indicating that **Left Control** is held down during a key press
| `LEFT_SHIFT` | `number` | Bitmask for indicating that **Left Shift** is held down during a key press
| `LEFT_ALT` | `number` | Bitmask for indicating that **Left Alt** is held down during a key press
| `LEFT_GUI` | `number` | Bitmask for indicating that **Left GUI** is held down during a key press
| `RIGHT_CONTROL` | `number` | Bitmask for indicating that **Right Control** is held down during a key press
| `RIGHT_SHIFT` | `number` | Bitmask for indicating that **Right Shift** is held down during a key press
| `RIGHT_ALT` | `number` | Bitmask for indicating that **Right Alt** is held down during a key press
| `RIGHT_GUI` | `number` | Bitmask for indicating that **Right GUI** is held down during a key press

## Code Examples

Tell the BLE Central that the key `L` is currently held down (automatically sends `l` + `Left Shift`):

```javascript
application.delegate("doKeyDown", {character: "L"});
```

Tell the BLE Central that the key `r` has been released:

```javascript
application.delegate("doKeyUp", {character: "r"});
```

Send a quick tap of `Control-Alt-q` to the BLE Central:

```javascript
application.delegate("doKeyTap", {character: "q", modifiers: MODIFIERS.LEFT_CONTROL | MODIFIERS.LEFT_ALT});
```

Send a quick tap of the `Mute` HID Consumer Device media key to the BLE Central:

```javascript
application.delegate("doKeyTap", {hidCode: HIDKEYS.MUTE.HID});
```

## HID Report Descriptor 

Experienced HID developers may want to review the HID Report Descriptor used by the BLE HID Host. The report follows (annotations courtesy of [https://eleccelerator.com/usbdescreqparser/](https://eleccelerator.com/usbdescreqparser/)):

```c
0x05, 0x01,        // Usage Page (Generic Desktop Controls)
0x09, 0x06,        // Usage (Keyboard)
0xA1, 0x01,        // Collection (Application)
0x85, 0x01,        //   Report ID (1)
0x05, 0x07,        //   Usage Page (Keyboard/Keypad)
0x19, 0xE0,        //   Usage Minimum (0xE0)
0x29, 0xE7,        //   Usage Maximum (0xE7)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x01,        //   Logical Maximum (1)
0x75, 0x01,        //   Report Size (1)
0x95, 0x08,        //   Report Count (8)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x95, 0x01,        //   Report Count (1)
0x75, 0x08,        //   Report Size (8)
0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x95, 0x05,        //   Report Count (5)
0x75, 0x01,        //   Report Size (1)
0x05, 0x08,        //   Usage Page (LEDs)
0x19, 0x01,        //   Usage Minimum (Num Lock)
0x29, 0x05,        //   Usage Maximum (Kana)
0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0x95, 0x01,        //   Report Count (1)
0x75, 0x03,        //   Report Size (3)
0x91, 0x01,        //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
0x95, 0x06,        //   Report Count (6)
0x75, 0x08,        //   Report Size (8)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x65,        //   Logical Maximum (101)
0x05, 0x07,        //   Usage Page (Keyboard/Keypad)
0x19, 0x00,        //   Usage Minimum (0x00)
0x29, 0x65,        //   Usage Maximum (0x65)
0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              // End Collection
0x05, 0x0C,        // Usage Page (Consumer)
0x09, 0x01,        // Usage (Consumer Control)
0xA1, 0x01,        // Collection (Application)
0x85, 0x02,        //   Report ID (2)
0x05, 0x0C,        //   Usage Page (Consumer)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x01,        //   Logical Maximum (1)
0x75, 0x01,        //   Report Size (1)
0x95, 0x08,        //   Report Count (8)
0x09, 0xE9,        //   Usage (Volume Increment)
0x09, 0xEA,        //   Usage (Volume Decrement)
0x09, 0xE2,        //   Usage (Mute)
0x09, 0xB6,        //   Usage (Previous Track)
0x09, 0xB5,        //   Usage (Next Track)
0x09, 0xB0,        //   Usage (Play)
0x09, 0xCD,        //   Usage (Play/Pause)
0x09, 0xB9,        //   Usage (Shuffle)
0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              // End Collection
```