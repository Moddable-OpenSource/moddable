## Test apps for RTC

`setTime` - sets the time to current time.

`showTime` - prints the time reported by the RTC.

`test` - try a number of different values

### To try different RTC drivers:

You will need to change the `main.js` and `manifest.json` to include the driver for your device.

| Class | Module Specifier |
| :---: | :--- |
| `DS1307` | `embedded:RTC/DS1307`
| `DS3231` | `embedded:RTC/DS3231`
| `RV3028` | `embedded:RTC/RV3028`
| `PCF8523` | `embedded:RTC/PCF8523`
| `PCF8563` | `embedded:RTC/PCF8563`
| `MCP7940` | `embedded:RTC/MCP7940`


in `main.js`, change the `import RTC` to match your device. For example:

```
import RTC from "embedded:RTC/DS1307";
```

in the `manifest.json` file, change the `include` for your device. For example:

```
	"include": [
		"$(MODDABLE)/modules/drivers/peripherals/ds1307/manifest.json"	
	]
```
