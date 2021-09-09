const url = `ws://${window.location.hostname}:8080`;
const ws = new WebSocket(url);

ws.onopen = function() {
	console.log("ws open");
	ws.send(JSON.stringify({"hello": "world"}));
}

ws.onclose = function() {
	console.log("ws close");
}

ws.onerror = function() {
	console.log("ws error");
}

ws.onmessage = function(event) {
	const data = event.data;
	console.log(`ws message: ${data}\n`);
}
