import { WebServer as HTTPServer } from "bridge/webserver";
import { BridgeWebsocket } from "bridge/websocket";
import { BridgeHttpZip } from "bridge/httpzip";
import Preference from "preference";

const http = new HTTPServer({
  port: 80,
});

let ws = new BridgeWebsocket("/api");
http.use(ws);
http.use(new BridgeHttpZip("site.zip"));

import { _model } from "model";

const preference_domain = "bridge";

class App {
  minus(value) {
    model.satisfaction = Math.max(0, model.satisfaction - 1);
    return model;
  }
  plus(value) {
    model.satisfaction = Math.min(10, model.satisfaction + 1);
    return model;
  }
  shutdown() {
    ws.close();
    http.close();
  }
  language() {
    model.language = value.language;
    return model;
  }
  restore() {
    let keys = Preference.keys(preference_domain);
    trace(`${keys}`);
    for (let key of keys) {
      trace(`${key}: ${Preference.get(preference_domain, key)}\n`);

      let pref_settings = Preference.get(preference_domain, key);
      if (pref_settings) {
        Object.assign(model, JSON.parse(pref_settings));
      }
      trace(`restore ${key}: `, pref_settings, "\n");
    }
    return model;
  }
  save() {
    Preference.set(preference_domain, "settings", JSON.stringify(model));
  }
}

let model = { ..._model };
const app = new App();
app.restore()

ws.callback = function cb(websock, message, value) {
  switch (message) {
    case BridgeWebsocket.connect:
      break;

    case BridgeWebsocket.handshake:
      websock.broadcast(model);
      break;

    case BridgeWebsocket.receive:
      try {
        trace(`Main WebSocket receive: ${value}\n`);
        value = JSON.parse(value);

        let action = value?.action;

        if (typeof app[action] === "function") {
          value = app[action](value);
        }
        if (value) websock.broadcast(value);
      } catch (e) {
        trace(`WebSocket parse received data error: ${e}\n`);
      }
  }
};
