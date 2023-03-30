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

import { Request } from "http"
import SecureSocket from "securesocket";
import Resource from "Resource";

const awsEndpoint = "<replace>";

if ("<replace>" === awsEndpoint)
    throw new Error("You need to set your awsEndpoint!")

let request = new Request({
    host: awsEndpoint,
    path: "/topics/moddable/with/aws/https?qos=1",
    method: "POST",
    port: 443,
    Socket: SecureSocket,
    body: JSON.stringify({ name: "Moddable", value: 123 }),
    response: String,
    secure: {
        protocolVersion: 0x0303,
        trace: false,
        applicationLayerProtocolNegotiation: "x-amzn-http-ca",
        certificate: new Resource("CA1-key.der"),
        clientCertificates: [new Resource("device.der")],
        clientKey: new Resource("private-key.der")
    },
});

request.callback = function (message, value)
{
    if (Request.responseComplete === message)
    {
        const json_data = JSON.parse(value);
        const message = json_data["message"];
        trace(`message: ${message}\n`);
    }
}
