# Home Assistant Examples
Updated June 2, 2026

This directory contains example projects for Home Assistant. The projects are set up to build with [`mcpack`](../readme.md), the Moddable SDK's bridge to npm modules. 

These examples use the official [Home Assistant WebSocket package](https://github.com/home-assistant/home-assistant-js-websocket) [available from npm](https://www.npmjs.com/package/home-assistant-js-websocket). There are many benefits to this choice:

- Using the official library ensures a high degree of compatibility and reliability, while keeping up with the latest changes in the Home Assistant project.
- The WebSocket interface is much lower latency than using HTTP requests (e.g. `fetch()`) for equivalent operations. 
- The WebSocket interface supports lightweight subscriptions to device change events without polling.
- You can tap into the documentation, online knowledge, and examples for the `home-assistant-js-websocket` package. These are great resources for people and their LLM agents alike.

## Examples

- [`discover`](./discover) - Lists all entities in the Home Assistant registry
- [`blink`](./blink) - Toggles a light on and off every 30 seconds using a timer. Subscribes to the light to know its current state.
- [`blink-ts`](./blink-ts) - The same as `blink` but implemented in TypeScript rather than JavaScript
- [`button`](./button) - A variation of `blink` that uses the device's default button to toggle the light on and off rather than a timer.

## Configuring Examples
The examples must be configured for your environment. The configuration is in `main.js` (or `main.ts` for TypeScript examples).

The examples require a Home Assistant long-lived token. That is defined by the `ACCESS_TOKEN` variable near the top of the file.

The examples that control a light require you to select a light. Set the entity name or friendly name in the `LIGHT` variable. Use the Home Assistant user interface or the `discover` example to find the friendly name or entity name of the light to control. Entity names are generally preferred as they are more efficient and less ambiguous.

## Running Examples
To run the JavaScript examples on the simulator, do the following:

```
cd $MODDABLE/examples/package/homeassistant/discover
npm install
mcpack mcconfig -d -m -p sim
```

To run the TypeScript examples on the simulator, do the following:

```
cd $MODDABLE/examples/package/homeassistant/blink-ts
npm install
npm run build && mcpack mcconfig -d -m -p sim
```

See the `mcpack` [readme](../readme.md) for more information on builds, including building for devices.
