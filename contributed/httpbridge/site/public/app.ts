/*
 * Copyright (c) 2021-2022  Moddable Tech, Inc.
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

import { _model } from "./model.js";

let observer = {
	set: function (obj, prop, value) {
	  console.log(`set prop: ${prop} ${ JSON.stringify(value)}\n`)
	  obj[prop] = value
	  return true;
	}
  };

const model = new Proxy(_model, observer);

class WebConnect extends WebSocket {
  constructor(url) {
    super(url);
    this.onopen = this.onOpen.bind(this)
    this.onmessage = this.onMessage.bind(this)
    this.onclose = this.onClose.bind(this)
    this.onerror = this.onError.bind(this)
  }

  onOpen() {
    this.trace("ws open", "e")
    this.send({ hello: "world" })
  }

  onMessage(event) {
    this.trace(`ws recv: ${event.data}`, "w")
      const data = JSON.parse(event.data)
    this.model( data )
  }

  model(data) {
    if ( data.hasOwnProperty('satisfaction') ) {
      model.satisfaction = data.satisfaction
      document.getElementById('satisfaction').value = data.satisfaction
    }
    if ( data.hasOwnProperty('language') ) {
      model.language = data.language;
      document.getElementById(`language-${data.language}`).checked=true
    }
  }

  onClose(event: CloseEvent) {
    this.trace(`ws close: ${event.code}`, "e")
  }

  onError(event: Event) {
    this.trace(`ws error: ${event.code}`, "e")
  }

  send(msg) {
    if (typeof msg === "object") msg = JSON.stringify(msg)
    this.trace(`ws sent: ${msg}`);
    try {
        super.send(msg);
    }
    catch (e) {
      this.trace(`ws error: ${e}`, "e");
    }
  }

  trace(msg, cls = "i") {
    if (typeof msg === "object") msg = JSON.stringify(msg);
    console.log(msg);
    msg = new Date().toString().substring(16, 24) + ": " + msg;
    document.getElementById("log").innerHTML = `<span class="${cls}">${msg}</span>\n${log.innerHTML}`;
  }

  action(act) {
    this.send({ action: act })
  }
}

class App {
  ws: WebConnect = null

  constructor() {
    const watch = document.querySelectorAll('.e-watch')
    watch.forEach(el => {
      el.addEventListener("change", (e) => {
      let t=e.target
      let packet = {}
      packet[t.name] = t.value
      this.ws.send(packet)
      })
    })

    const actionElements = document.querySelectorAll('.e-action');
    actionElements.forEach(el => {
      el.addEventListener('click', (e) => {
        
        if ( e.target.id === "connect" ) {
          this.connect();
        } else {
          this.ws.action(e.target.id)
        }
      })
    })
    this.connect();
  }
  
  connect() {
    if ( this.ws ) {
      try {
        this.ws.trace(`disconnecting... ${this.ws.readyState}`)
        if ( this.ws.readyState === WebSocket.OPEN ) 
          this.ws.close()
      }
      catch (e) {
      }
      delete this.ws;
    }
    const url = "ws://" + window.location.hostname + "/api"
    this.ws = new WebConnect(url)
    this.ws.trace("connecting...")
    this.ws.model(model)
  }
}

let app = new App();
