## Test apps for RTC

`setTime` - sets the time to current time.

`showTime` - prints the time reported by the RTC.

`test` - try a number of different values

### To try different RTC drivers:

You will need to change the `main.js` and `manfest.json` to include the driver for your device.

| Class | Module Specifier |
| :---: | :--- |
| `DS1307` | `embedded:peripherals/RTC-MaximIntegrated/DS1307`
| `DS3231` | `embedded:peripherals/RTC-MaximIntegrated/DS3231`
| `RV3028` | `embedded:peripherals/RTC-MicroCrystal/RV3028`
| `PCF8523` | `embedded:peripherals/RTC-NXP/PCF8523`
| `PCF8563` | `embedded:peripherals/RTC-NXP/PCF8563`
| `MCP7940` | `embedded:peripherals/RTC-Microchip/MCP7940`


in `main.js`, change the `import RTC` to match your device. For example:

```
import RTC from "embedded:peripherals/RTC-MaximIntegrated/DS1307";
```

in the `manifest.json` file, change the `include` for your device. For example:

```
	"include": [
		"$(MODDABLE)/modules/drivers/peripherals/ds1307/manifest.json"	
	]
```
