let connectedCount = 0;
let messageCount = 0;

globalThis.onconnect = function(e) {
	connectedCount += 1;;
	e.ports[0].onmessage = onmessage;
	e.ports[0].postMessage({connectedCount});
}

function onmessage(value) {
	messageCount += 1;
	this.postMessage({messageCount, value});
}
