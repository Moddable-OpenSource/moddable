/*
 * Copyright (c) 2016-2021 Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

/*
API key and locations (HOME and WORK) must be specified in manifest.json.

To get a Google API key, follow the instructions here: 
https://developers.google.com/maps/documentation/directions/get-api-key

Make sure to follow the instructions in the "Before you begin" section to enable billing 
and the Directions API. Your account won't be charged unless you exceed the free quota,
but be aware that you will be charged if you make too many requests.

For location formatting requirements, see the Google Maps API documentation: 
https://developers.google.com/maps/documentation/directions/get-directions#required-parameters
*/

import config from "mc/config";
import {Request} from "http"
import SecureSocket from "securesocket";

const API_KEY = config.API_KEY;
const HOME = config.HOME;
const WORK = config.WORK;

export function getTimeEstimate(swap = false) {
    if (API_KEY == "API_KEY") {
        application.delegate("onTravelTimeUpdate", "No API key");
        return;
    }
    if (HOME == "HOME_ADDRESS") {
        application.delegate("onTravelTimeUpdate", "Home address not configured");
        return;
    }
    if (WORK == "WORK_ADDRESS") {
        application.delegate("onTravelTimeUpdate", "Work address not configured");
        return;
    }
    let origin, destination;
    if (swap) {
        origin = WORK;
        destination = HOME;
    } else {
        origin = HOME;
        destination = WORK;
    }
    let request = new Request({
        host: "maps.googleapis.com", 
        path: `/maps/api/directions/json?origin=${origin}&destination=${destination}&departure_time=now&mode=DRIVING&key=${API_KEY}`,
        response: String,
        port: 443, 
        Socket: SecureSocket, 
        secure: {protocolVersion: 0x303} 
    });

    request.callback = function(message, value, etc) {
        if (Request.responseComplete == message) {
            let result = JSON.parse(value, ["routes", "legs", "duration", "duration_in_traffic", "text", "value"]);
            let route = result.routes[0];
            if (!route) {
                trace(`No route found\n`);
                application.delegate("onTravelTimeUpdate", null);
            } else {
                let leg = route.legs[0];
                let duration = leg.duration.text;
                let durationInTraffic = leg.duration_in_traffic.text;
                let durationInTrafficValue = leg.duration_in_traffic.value;
                application.delegate("onTravelTimeUpdate", [durationInTraffic, durationInTrafficValue, duration]);
            }
        }
    }
}