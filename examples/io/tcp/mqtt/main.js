import * as mqtt from "mqtt";
import data from "data";

function print(...args) {
	args = args.map(item => item instanceof ArrayBuffer ? String.fromArrayBuffer(item) : String(item));
	trace(args.join(" ") + "\n");
}

const client = mqtt.connect('mqtt://test.mosquitto.org')
client.subscribe( "xs/test", { qos: 2 }, (err, granted) => { 
	print("subscribed", err, granted[0].topic, granted[0].qos);
	client.publish('xs/test', "zero", { qos:0 }, err => { print("published zero", err) });
	client.publish('xs/test', "one", { qos:1 }, err => { print("published one", err) });
	client.publish('xs/test', "two", { qos:2}, err => { print("published two", err) });
	client.publish('xs/test', data, { qos:1 }, err => { print("published data", err) });
});

client.on('close', function () {
	print("close");
})
client.on('connect', function () {
	print("connect");
})
client.on('end', function (err) {
	print("end", err);
})
client.on('error', function (err) {
	print("error", err);
})
client.on('message', function (topic, message) {
	print("message", topic, message);
})
client.on('offline', function () {
	print("offline");
})
client.on('reconnect', function () {
	print("reconnect");
})
