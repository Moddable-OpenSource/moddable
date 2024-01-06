## JSONParser Sample-Schedule-Info-App Cookbook Example

This example will send a GET request to the specified URL and parse the JSON data stream using the provided patterns. The parsed data is then displayed in the console. If an error occurs, it will be logged to the console as well.

### Start HTTP Server

```shell
sample-schedule-info-app % python3 -m http.server 8000
```

### Confirm Link

http://localhost:8000/sample-schedule-info.json

### Run Sample App

```text
Logged 11 meetings.
Here are the top 3 for further processing:
1. From Thu Sep 21 2023 08:30:00 GMT+1000 to 09:00:00 GMT+1000
2. From Thu Sep 21 2023 09:00:00 GMT+1000 to 09:30:00 GMT+1000
3. From Thu Sep 21 2023 09:45:00 GMT+1000 to 10:00:00 GMT+1000
```
