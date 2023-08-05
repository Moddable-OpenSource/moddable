## About This Module
This module can connect multiple boards with UART. The great feature of this module is that you can add several commands like JavaScript eventListener that whenever you need that command, you can call that command from board #1 and execute it on board #2. You can better understand what I mean in the [examples](#example).

[@phoddie](https://github.com/phoddie) Thanks for your help with this project, You can see our [conversation](https://github.com/Moddable-OpenSource/moddable/discussions/1180).

## Initializing
```javascript
import Cluster from "cluster";

const $Cluster = new Cluster({
    transmit: 17, // TX
    receive : 16, // RX
    baud    : 115200,
    port    : 2
});
```

## Functions

| Method | Arguments | Description |
| --- | :-- | :-- |
| `$Cluster.bind` | string name, function method | Add a new function to the list of clustering events
| `$Cluster.trigger` | string name, object data | Calling the command from the first board and executing it on other boards and vice versa

## Wiring esp32
> This picture is an example of esp32 wiring, just so you know what to do, this module works for boards that support serial UART.
<img src="https://github.com/salarizadi/moddable-sdk/assets/67143370/aeb040e1-f968-4930-9b4a-077a0741693c" width="250" height="250">

## Example
<a id="example"></a>

First board :
```javascript
import Timer from "timer"
import Cluster from "cluster";

const $Cluster = new Cluster({
    transmit: 17,
    receive : 16,
    baud    : 115200,
    port    : 2
});

$Cluster.bind("core-1", function (data) {
    trace("\nHello from Core-1 : " + JSON.stringify(data))
})

Timer.repeat(() => $Cluster.trigger("core-2", {
    x: 345,
    y: 564
}), 1000)
```

Second board :
```javascript
import Timer from "timer"
import Cluster from "cluster";

const $Cluster = new Cluster({
    transmit: 17,
    receive : 16,
    baud    : 115200,
    port    : 2
});

$Cluster.bind("core-2", function (data) {
    trace("\nHello from Core-2 : " + JSON.stringify(data))
})

Timer.repeat(() => $Cluster.trigger("core-1", {
    x: 23,
    y: 50
}), 1000)
```
