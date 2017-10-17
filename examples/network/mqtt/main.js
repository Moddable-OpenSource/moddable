/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Client from "mqtt";
import Timer from "timer";
import WiFi from "wifi";
import Net from "net";

const WIFI_SSID      = "Playground Lab";
const WIFI_PSK       = "fluvia97@bcn";
const MQTT_SERVER    = "10.1.4.14";
const MQTT_PORT      = 9000;
const MQTT_CLIENT_ID = "XS6-MQTT";
const MQTT_PATH      = "/mqtt";

WiFi.connect({ssid: WIFI_SSID, password: WIFI_PSK});

let mqtt_timer_id = 0;

trace(`waiting for network on ${Net.get("MAC")}...\n`);
Timer.repeat(id => {
        let state = WiFi.status;
        if (state == 5) {
                trace('network up; starting MQTT\n');

                Timer.clear(id);

                let mqtt = new Client({
                        host: MQTT_SERVER,
                        port: MQTT_PORT,
                        path: MQTT_PATH,
                        id: MQTT_CLIENT_ID,
                });
                mqtt.onReady = () => {
                        mqtt.subscribe("/booga");
                        mqtt_timer_id = Timer.repeat((id)=>{mqtt_timer_id = id; mqtt.publish("/booga", "flex")}, 1000);
                };
                mqtt.onMessage = (topic, body) => {
                        body = Client.asString(body);
                        body = "" + body;
                        trace(`received via '${topic}' data '${body}'\n`);
                };
                mqtt.onClose = () => {
                        trace('lost connection to server\n');
                        if (mqtt_timer_id) Timer.clear(mqtt_timer_id);
                };
        }
}, 200);
