## Charlieplexer

An example driver for a charlieplexed LED array. Charlieplexing allows individually controlling LEDs with relatively few pins - `N` pins can drive `N * (N - 1)` LEDs! Between each combination of two pins, two LEDs are connected with alternating polarity. Now each LED can be addressed individually given its pair of pins and direction of current.

This example lights each LED in the array for 100ms.

### Example Usage

```js
// pass an array of pins to use
const c = new Charlieplexer([23, 22, 1]);

// lights each LED.
c.scan();
```

