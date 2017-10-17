/*
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

/*Websocket Toggle. Pairs with server (resources/server.c) to track state of two switches.
Can be used with webpage (resources/websocketsToggle.html) to manage switches from phone/browser.

Fill in SSID and PSK before running*/

import {
	Skin,
	Content,
	Application,
  Column
} from "piu/MC";

import {Client} from "websocket"
import WiFi from "wifi"

const SSID = "";
const PSK = "";
const WSSERVER = "somethingevil.net"

let ws = undefined;

let wsCallback = function(message, value){
	switch (message) {
		case 1:
			trace("socket connect\n");
			break;

		case 2:
			trace("websocket handshake success\n");
			this.write("GET");
			break;

		case 3:
      trace("websocket got message: " + value + "\n");
      if (value){
        let obj = JSON.parse(value);
        application.distribute("onStatusChanged", obj.state, obj.control);
      }
			break;

		case 4:
			trace("websocket close\n");
			break;
	}
}

let monitor;

function makeMonitor(){
  monitor = new WiFi({ssid: SSID, password: PSK}, message => {
      if ("gotIP" == message){
        trace("got IP\n");
  			ws = new Client({host: WSSERVER, port: 8080});
  			ws.callback = wsCallback;
  		}else if ("disconnect" == message){
  			ws.close();
  			ws = undefined;
  		}
  });
}

const GraySkin = Skin.template({fill:"gray"});
const GreenSkin = Skin.template({fill:"green"});
const RedSkin = Skin.template({fill:"red"});
const WhiteSkin = Skin.template({fill:"white"});

let MainCol = Column.template($ => ({
  left: 0, right: 0, top: 0, bottom: 0,
  Skin: WhiteSkin,
  contents: [
    new Button(0),
    new Button(1)
  ]
}));

let Button = Content.template($ => ({
  left: 10, right: 10, top: 10, bottom: 10,
  Skin: GraySkin, active: true,
  Behavior: class extends Behavior {
    onCreate(content, data){
      this.number = data;
      this.state = 0;
    }
		onStatusChanged(content, status, number) {
      if (number == this.number){
  			if (status == "ON"){
          this.state = 1;
          content.skin = new GreenSkin();
        }else if(status == "OFF"){
          this.state = 0;
          content.skin = new RedSkin();
        }
      }
		}
    
    onTouchEnded(content){
      if (this.state == 0){
        application.distribute("onStatusChanged", "ON", this.number);
        ws.write("ON " + this.number);
      }else if (this.state == 1){
        application.distribute("onStatusChanged", "OFF", this.number);
        ws.write("OFF " + this.number);
      }
    }
	}
}));

export default new Application(null, {
	commandListLength:2048,
	displayListLength:2600,
	touchCount:1,
	Behavior: class extends Behavior {
		onCreate(application) {
      application.add(new MainCol());
      makeMonitor();
		}
	},
});
