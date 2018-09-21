# Router Example

## Run example with WiFi

1. Copy `.env.example` and call it `.env`
2. Update `.env` with your WiFi credentials
3. Run `source .env` to load the variables into your terminal
4. Load the example onto your device

```
mcconfig -d -m -p esp ssid=$SSID password=$PASSWORD
```

## Example Details

This example is designed to showcase the very basic capability of the router. Currently get and post endpoints can be registered using an express style syntax. A response can then send a response object or string. Response object are automatically converted to JSON.

