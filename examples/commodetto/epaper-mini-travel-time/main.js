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

import config from "mc/config";
import {Request} from "http"
import SecureSocket from "securesocket";
import parseBMF from "commodetto/parseBMF";
import parseRLE from "commodetto/parseRLE";
import Poco from "commodetto/Poco";
import Resource from "Resource";
import Timer from "timer";

let render = new Poco(screen, { displayListLength: 2048, rotation: config.rotation });

let black = render.makeColor(0, 0, 0);
let white = render.makeColor(255, 255, 255);

let OpenSans18 = parseBMF(new Resource("OpenSans-Regular-18.bf4"));
let OpenSans35 = parseBMF(new Resource("OpenSans-Regular-35.bf4"));

let arrow = parseRLE(new Resource("arrow-alpha.bm4"));

function getTimeEstimate() {
    const API_KEY = config.API_KEY;
    const HOME = config.HOME;
    const WORK = config.WORK;

    if (API_KEY == "API_KEY") {
        trace("No API key\n");
        return;
    }
    if (HOME == "HOME_ADDRESS") {
        trace("Home address not configured\n");
        return;
    }
    if (WORK == "WORK_ADDRESS") {
        trace("Work address not configured\n");
        return;
    }

    let request = new Request({
        host: "maps.googleapis.com", 
        path: `/maps/api/directions/json?origin=${HOME}&destination=${WORK}&departure_time=now&mode=DRIVING&key=${API_KEY}`,
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
                updateDisplay(null);
            } else {
                let leg = route.legs[0];
                let duration = leg.duration.text;
                let durationInTraffic = leg.duration_in_traffic.text;
                let durationInTrafficValue = leg.duration_in_traffic.value;
                updateDisplay([durationInTraffic, durationInTrafficValue, duration]);
            }
        }
    }
}

function updateDisplay(time) {
    let travelTime="", durationInTraffic="", traffic="";
    if (time === null) {
       travelTime = "No route found";
    } else if (typeof time === "string") {
       travelTime = time;
    } else {
        durationInTraffic = time[0];
        let duration = time[2];
        travelTime = durationInTraffic;
        let diff = durationInTraffic/duration;
        if (diff >= 1.4)
            traffic = "Traffic is heavy";
        else if (diff >= 1.25)
            traffic = "Traffic is light";
        else
            traffic = "No traffic";
    }

    render.begin();
        render.fillRectangle(white, 0, 0, render.width, render.height);
        render.fillRectangle(black, 35, 10, 71, 29);
        render.fillRectangle(black, 151, 10, 64, 29);
        render.drawGray(arrow, black, 116, 14, 0, 0, 25, 22);
        render.drawText("Home", OpenSans18, white, 45, 10);
        render.drawText("Work", OpenSans18, white, 161, 10);
        render.drawText(travelTime, OpenSans35, black, (render.width - render.getTextWidth(travelTime, OpenSans35)) >> 1, 40);
        render.drawText(traffic, OpenSans18, black, (render.width - render.getTextWidth(traffic, OpenSans18)) >> 1, 88);
    render.end();
}

getTimeEstimate();
Timer.repeat(() => {
    getTimeEstimate();
}, 1000*60*10);
