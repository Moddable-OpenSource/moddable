/*
 * Copyright 2023 Moddable Tech, Inc.
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
import SecureSocket from "securesocket";
import Timer from "timer";
import Resource from "Resource";

const awsEndpoint = "<replace>";

if ("<replace>" === awsEndpoint)
	throw new Error("You need to set your awsEndpoint!")

class SecureClient extends Client
{
	onReady()
	{
		this.subscribe("moddable/with/aws/#");
		Timer.set(() =>
		{
			var message = Date();
			this.publish("moddable/with/aws/date", message);
			this.publish("moddable/with/aws/math", Math.random());
		}, 0, 1000);
	};

	onMessage(topic, body)
	{
		trace(`received "${topic}": ${String.fromArrayBuffer(body)}\n`);
	};

	onClose()
	{
		trace('lost connection to server\n');
	};
}

new SecureClient ({
	host: awsEndpoint,
	timeout: 60_000,
	port: 443,
	Socket: SecureSocket,
	secure: {
		protocolVersion: 0x0303,
		trace: false,
		applicationLayerProtocolNegotiation: "x-amzn-mqtt-ca",
		certificate: new Resource("CA1-key.der"),
		clientCertificates: [new Resource("device.der")],
		clientKey: new Resource("private-key.der")
	},
});
