import { WebServer as HTTPServer } from "bridge/webserver";
import { BridgeWebsocket } from "bridge/websocket";
import { BridgeHttpZip } from "bridge/httpzip";

const http = new HTTPServer({
  port: 80,
});

let ws = new BridgeWebsocket("/api");
http.use(ws);
http.use(new BridgeHttpZip("site.zip"));

import { _model } from "model";

let observer = {
  set: function (obj, prop, value) {
    trace(`set prop: ${prop} ${JSON.stringify(value)}\n`);
    obj[prop] = value;
    return true;
  },
};

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
}

const app = new App();

let model = { ..._model };

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

        websock.broadcast(value);
      } catch (e) {
        trace(`WebSocket parse received data error: ${e}\n`);
      }
  }
};
