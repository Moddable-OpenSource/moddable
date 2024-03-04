import EventEmitter from "@moddable/eventemitter3";

const ee = new EventEmitter;
ee.on("boo", arg => trace(`event "boo" with argument ${arg}\n`)); 
ee.emit("boo", 2);
