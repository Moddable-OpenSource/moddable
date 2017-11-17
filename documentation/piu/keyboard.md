# Moddable Keyboard Module

The Moddable Keyboard Module provides a soft keyboard with a responsive layout. 

The keyboard is implemented as a Piu Port object that automatically fills its parent container, allowing it to reflow in a manner controlled by the application. Important properties of the keyboard are configured using a dictionary passed into the constructor.

Key presses trigger a specified callback function in the application. The style (font and weight) of the keyboard's text are driven by a Piu Style object supplied by the application, allowing the application to use Style templates and determine the appropriate font and weight at runtime.

The keyboard implements an event (`doKeyboardTransitionOut`) that can be triggered to cause the keyboard to transition off-screen. That event takes a callback as an argument — the callback is invoked when the transition completes.

- **Source code:** [`keyboard.js`](../../modules/input/keyboard/keyboard.js)
- **Relevant Example:** [keyboard](../../examples/piu/keyboard/main.js)

#### Module Exports
| Export | Type |  Description |
| --- | --- | --- |
| `Keyboard` | `constructor` | Constructor used to create Keyboard instances. |
| `BACKSPACE` | `string` | Constant used to indicate to callback that the backspace key was pressed. |
| `SUBMIT` | `string` | Constant used to indicate to callback that the submit key was pressed. |

####Constructor Description

###### `Keyboard(dictionary)`

| Argument | Type | Description |
| --- | --- | :--- | 
| `dictionary` | `object` | An object with properties to configure the resulting keyboard. Only parameters specified in the [Dictionary](#keyboard-dictionary) section below will have an effect; other parameters will be ignored.

Returns a `keyboard` instance, a `Port` object that uses an instance of the `KeyboardBehavior` class as its Behavior.

<a id="keyboard-dictionary"></a>
#### Dictionary

| Parameter | Type | Default Value | Description |
| --- | --- | --- | :--- |
| `callback` | `function` | n/a | **Required.** Function to call when a key is pressed. See the [Key Pressed Callback](#key-callback) section below.|
| `style` | `style` | n/a | **Required.** A Piu Style object that will be used for the text on keys. |
| `bgColor` | `string` | `"#5b5b5b"`| The background fill color. |
| `keyColor` | `string` | `"#d8d8d8"`| The color for the character keys when not being pressed. |
| `keyDownColor` | `string` | `"#999999"`| The color for the character keys while they are being pressed. |
| `specialKeyColor` | `string` | `"#999999"`| The color for the special keys (shift, symbol, backspace, and submit) when not being pressed. |
| `keyToggledColor` | `string` | `"#7b7b7b"`| The color for the character keys while they are being pressed. |
| `textColor` | `string` | `"#000000"`| The color for the text on character keys. |
| `specialTextColor` | `string` | `"#ffffff"`| The color for the text on special keys (shift, symbol, backspace, and submit). |
| `submit` | `string` | `"OK"`| String to render on the submit key. |
| `doTransition` | `boolean` | `false`| Whether or not to transition in the keyboard when it is first displayed. |
| `transitionTime` | `number` | `250`| The duration of the keyboard in/out transition in milliseconds. |

<a id="key-callback"></a>
#### Key Pressed Callback

###### `keyCallback(keyPressed)`
The callback function specified in the dictionary will be invoked when keys are pressed. The argument provided to the callback will specify what key was pressed, including special keys.

| Argument | Type | Description |
| --- | --- | --- |
| keyPressed | `string` | In most cases, the string will be the value of the key pressed (e.g. `"a"`, `"3"`, `"$"`). It can also be one of the two constants exported by the module:  `BACKSPACE` or `SUBMIT` which indicate that those keys were pressed on the keyboard.|

#### Event
The keyboard module implements one event: `doKeyboardTransitionOut`. This function can be triggered to cause the keyboard to transition off screen. 

**`doKeyboardTransitionOut(keyboard, onCompleteCallback)`**

| Argument | Type | Description |
| --- | --- | :--- |
| `keyboard` | `keyboard` | The `keyboard` object that received the event. |
| `onCompleteCallback` | `function` | A function to call when the transition is complete. This callback will receive no arguments. |
