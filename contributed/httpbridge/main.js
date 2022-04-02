/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
 * Copyright (c) Wilberforce
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
import { WebServer as HTTPServer } from "bridge/webserver";
import { BridgeWebsocket } from "bridge/websocket";
import { BridgeHttpZip } from "bridge/httpzip";
import Preference from "preference";

const http = new HTTPServer({
  port: 80,
});

let ws = http.use(new BridgeWebsocket("/api"));
http.use(new BridgeHttpZip("site.zip"));

class App {
  #preference_domain = "bridge";
  #model;

  constructor(m) {
    this.#model = m;
  }

  get model() {
    return this.#model;
  }

  set model(m) {
    this.#model = m;
  }

  minus(value) {
    this.model.satisfaction = Math.max(0, this.model.satisfaction - 1);
    return this.model;
  }
  plus(value) {
    this.model.satisfaction = Math.min(10, this.model.satisfaction + 1);
    return this.model;
  }
  shutdown() {
    ws.close();
    http.close();
  }
  language() {
    this.model.language = value.language;
    return this.model;
  }
  restore() {
    let keys = Preference.keys(this.#preference_domain);
    for (let key of keys) {
      let pref_settings = Preference.get(this.#preference_domain, key);
      if (pref_settings) {
        Object.assign(this.model, JSON.parse(pref_settings));
      }
    }
    return this.model;
  }
  save() {
    Preference.set(
      this.#preference_domain,
      "settings",
      JSON.stringify(this.model)
    );
  }
}

import { _model } from "model";
const app = new App({ ..._model });
app.restore();

ws.callback = function cb(websock, message, value) {
  switch (message) {
    case BridgeWebsocket.connect:
      break;

    case BridgeWebsocket.handshake:
      websock.broadcast(app.model);
      break;

    case BridgeWebsocket.receive:
      try {
        trace(`Main WebSocket receive: ${value}\n`);
        value = JSON.parse(value);

        let action = value?.action;

        if (typeof app[action] === "function") {
          value = app[action](value);
        } else {
          if (value.hasOwnProperty('language')) {
            Object.assign(app.model,value)
          } else {
            trace("No matching action found\n");
          }
        }
        if (value) websock.broadcast(value);
      } catch (e) {
        trace(`WebSocket parse received data error: ${e}\n`);
      }
  }
};
