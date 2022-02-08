import { WebServer as HTTPServer } from "bridge/webserver";
import { BridgeWebsocket } from "bridge/websocket";
import { BridgeHttpZip  } from "bridge/httpzip";

const http = new HTTPServer({
	port: 80
});

let ws = new BridgeWebsocket('/api');
//let ws = new WebSocketServer('/api');
http.use( ws );
http.use( new BridgeHttpZip('site.zip'));

import { _model } from "model";

let observer = {
	set: function (obj, prop, value) {
	  trace(`set prop: ${prop} ${ JSON.stringify(value)}\n`);
	  obj[prop] = value;
	  return true;
	}
  };

  /*
  class thingy {
	minus() {
		trace(`minus\n`);
	}
  }
  */
  
const model = new Proxy(_model, observer);

ws.callback = function cb(websock,message, value) {
	switch (message) {		
		case BridgeWebsocket.connect:
			break;

		case BridgeWebsocket.handshake:
			websock.broadcast( model )
			break;			

		case BridgeWebsocket.receive:
			try {
				trace(`Main WebSocket receive: ${value}\n`);
				value = JSON.parse(value);
				
				if ( value?.action === 'shutdown'){
					ws.close();
					http.close();
				}
				if ( value?.action === 'minus'){
					model.satisfaction=Math.max(0,model.satisfaction-1)
					value=model;
					model.minus();
				}				
				if ( value?.action === 'plus'){
					model.satisfaction=Math.min(10,model.satisfaction+1)
					value=model;
				}
				if ( value.hasOwnProperty('language') ) {
					model.language = value.language;
				}

				websock.broadcast(value);
			}
			catch (e) {
				trace(`WebSocket parse received data error: ${e}\n`);
			}
	}
}
