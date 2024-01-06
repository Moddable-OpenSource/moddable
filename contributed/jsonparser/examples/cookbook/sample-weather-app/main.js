// Copyright (c) 2024 Mark Wharton
// https://opensource.org/license/mit/

import { JSONParser } from "jsonparser";
import { Request } from "http";

// with thanks to moddable/examples/network/http/httpgetjson/main.js
const APPID = "94de4cda19a2ba07d3fa6450eb80f091";
const zip = "94025";
const country = "us";

let request = new Request({ host: "api.openweathermap.org",
                            path: `/data/2.5/weather?zip=${zip},${country}&appid=${APPID}&units=imperial` });

request.callback = function(message)
{
    if (Request.status === message) {
        const keys = ["main", "name", "temp", "weather"];
        this.parser = new JSONParser({ keys });
    }
    else if (Request.responseFragment === message) {
        const string = this.read(String);
        this.parser.receive(string);
    }
    else if (Request.responseComplete === message) {
        let result = this.parser.status;
        if (result === JSONParser.success) {
            const value = this.parser.root.value;
            trace(`The temperature in ${value.name} is ${value.main.temp} F.\n`);
            trace(`The weather condition is ${value.weather[0].main}.\n`);
        }
        else {
            trace(`result: ${result}\n`);
        }
        this.parser.close();
    }
}
