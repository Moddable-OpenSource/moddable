/*---
description: 
flags: [async, module]
---*/

import Listener from 'embedded:io/socket/listener';
import TCP from 'embedded:io/socket/tcp';

await $NETWORK.connected;

$TESTMC.timeout(5_000);

const port = 1234;
let httpServerSocket;
let wsServerSocket;
let messageSent = false;

// http server listening
new Listener({
    port,
    onReadable() {
        // http sever gets a connection
        trace('Listener received a client connection\n');
        const serverTCPSocket = this.read();

        // transfer to HTTP connection
        httpServerSocket = new TCP({
            from: serverTCPSocket,
            onReadable(count) {
                // got an HTTP request
                const data = this.read(count);
                trace(
                    `Received HTTP request: ${String.fromArrayBuffer(data)}\n`
                );
                this.write(ArrayBuffer.fromString('HTTP Response'));

                // transfer HTTP connection to WS connection
                trace('Transfer HTTP socket to WS server connection\n');
                wsServerSocket = new TCP({
                    from: httpServerSocket
                });
                trace('Closing server socket now\n');
                wsServerSocket.close();		// this is the close that is being tested!
            },
        });
    },
});

new TCP({
    address: '127.0.0.1',
    port,
    onReadable(count) {
        const data = this.read(count);
        trace(
            `Client HTTP socket received response: ${String.fromArrayBuffer(
                data
            )}\n`
        );
    },
    onWritable() {
        if (!messageSent) {
            trace('Client sending HTTP request\n');
            this.write(ArrayBuffer.fromString('HTTP Request'));
            messageSent = true;
        }
    },
    onError() {
        trace('Success!  Client-side socket closed\n');
        $DONE();
    },
});

