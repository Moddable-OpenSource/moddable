# Connecting to AWS IoT with the Moddable SDK
​Copyright 2023 Moddable Tech, Inc.<BR>
Revised: March 28, 2023

In this guide, you will learn how to use the Moddable SDK to securely connect  AWS IoT using the HTTP and MQTT protocols.
​
There are two examples that show the techniques described in this guide, [http-aws](./http-aws) and [mqqt-aws](./mqtt-aws)
​
## Setup AWS Thing
​
First, let's setup AWS IoT with an AWS "Thing" to be able to download keys that give us the correct permissions to connect with AWS IoT. In our examples, we use TLS for the security of the connection, described in [Secure Socket in the Moddable SDK](../../../documentation/network/securesocket.md).
​
The keys we need are:
​
* Device certificate
* Private Key
* Amazon trust services endpoint key
  * Referred to as "CA key" going forward
  * Both RSA and ECC may be used
​
See the [Amazon documentation](https://docs.aws.amazon.com/iot/latest/developerguide/create-iot-resources.html) for how to create and download these keys.
​
## Convert Keys
​
Now that you have your keys, we need to convert them from their PEM (Base64 encoded ASCII) format to DER (binary) format. Use the following commands to convert the keys to the format used by the Moddable SDK. We will name the converted key files `device.der`, `CA1.key`, and `private-key.der` to match the names used by the examples.
​
1. Convert the device certificate:
​
```
openssl x509 -inform pem -in <path/to/device-cert.key> -out device.der -outform der
```
​
2. Convert the CA key:
​
```
openssl x509 -inform pem -in <path/to/ca-key.key> -out CA1-key.der -outform der
```
​
3. Convert the private key:
​
```
openssl pkcs8 -topk8 -in </path/to/private_key.key> -inform pem -out private-key.der -outform der -nocrypt
```
​
4. Place the newly converted keys into the directory of the example you are running, `$MODDABLE/examples/network/aws/mqtt-aws` for `mqtt-aws`.
​
> **Note**: To use different certificate names in the examples, be sure to update both the source code in `main.js` and `manifest.json`.
​
For more information about key conversion, see the [cryptography documentation](../../..//documentation/crypt/crypt.md#class-transform).
​
## Prepare the MQTT Test Client on AWS
​
The [AWS MQTT Test Client](https://us-east-1.console.aws.amazon.com/iot/home?region=us-east-1#/test) is a convenient tool to verify that messages from our device are received by AWS. It supports MQTT messages sent using MQTT, MQTT over WebSocket, and also HTTPS. We will use it with both the `mqtt-aws` and `http-aws` examples.
​
1. Go to the [AWS MQTT Test Client](https://us-east-1.console.aws.amazon.com/iot/home?region=us-east-1#/test) page. From here, create a topic to subscribe to. In this example, the topic will be `moddable/with/aws/#`. If you decide to change this, make sure you change it accordingly in your example code.
2. We also need to get the `endpoint` of the client, which can be found in the "Connection details" dropdown near the top of the page. Make note of the endpoint for the next step, and keep this tab open.
​
The AWS MQTT Test Client not only provides an easy way to verify that our setup is working properly; it also enables you to connect the network traffic from the client to other services on AWS as your project requires.
​
## Set The Endpoint
​
With our keys formatted correctly and placed in the correct directory, and AWS prepared to receive our messages, we are now nearly ready run the examples in this directory to communicate with AWS IoT.
​
You first need to update your example to use the AWS endpoint you noted earlier. In `main.js` of your selected example, change the `awsEndpoint` variable to match the following format: `<endpoint>`.iot.`<region>`.amazonaws.com (ex: `a2vxxxxxxxxxxx-ats.iot.us-east-1.amazonaws.com`
​
> **Note**: do not include protocol identifiers like `https://`
​
## Test The Connection
​
Once you have all of this setup, you can run your selected project!
​
This command runs the example on your computer using the simulator:
​
```
mcconfig -m -d
```
​
For `mqtt-aws`, you should see  the date and a random number logged to the xsbug console once a second. You should also see these messages appearing in the AWS MQTT Test Client.
​
For `http-aws`, you should see that you get a message response of "OK" in xsbug and also see a "Moddable: 123" message appear in the AWS MQTT Test Client.
​
<!-- See the [MQTT module](../../..//modules/network/mqtt/mqtt.js) and [HTTP module](../../../modules/network/http/http.js) for more info about how these modules work. -->
​
## Troubleshooting
​
If you do not see any messages in the Test Client, ensure that the paths to your certificates are correct and names are formatted correctly in the code, and that you formatted the endpoint URL correctly (without `https://`, ex: `a2vxxxxxxxxxxx-ats.iot.us-east-1.amazonaws.com`).
​
### Simple Test Connection
​
You can run the following from your computer's terminal, using the keys downloaded from AWS (before converting them to .der format) to validate that your keys, endpoint, and AWS IoT are setup correctly. This sends a "Hello, World" message to your AWS MQTT Test Client using HTTPS (make sure you are subscribed to `moddable/with/aws/#` in the client):
​
```
curl --tlsv1.2 --cacert <path/to/CA-key.pem> --cert <path/to/device.pem.crt> --key <path/to/private.pem.key> --request POST --data "{ \"message\": \"Hello, world\" }" "https://<endpoint_url>:8443/topics/moddable/with/aws/test?qos=1"
```
​
Something similar to the following should be printed to your terminal if the connection was successful:
​
```json
{"message":"OK","traceId":"xxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxx"}
```
​
Notes:
​
* Make sure you replace `<path/to/CA-key.pem>`, `<path/to/device.pem.crt>`, and `<path/to/private.pem.key>` with the CA key, device key, and private key you originally downloaded from AWS, respectively.
* Change the `<endpoint_url>` to the endpoint of your AWS MQTT Test Client, which should also be the same value as the `awsEndpoint` variable in your `main.js` (ex:` a2vxxxxxxxxxxx-ats.iot.us-east-1.amazonaws.com`
* If you don't have `curl` installed on your machine, you can get it from [here](https://everything.curl.dev/get).
​
### Now you should be able to communicate with AWS using the Moddable SDK!