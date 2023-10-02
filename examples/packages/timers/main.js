setImmediate(() => console.log("immediate"));

let times = 10;
let timer = setInterval(() => { 
	if (times > 0) {
		times--;
		console.log("interval");
	}
	else
		clearInterval(timer);
}, 100);

setTimeout(() => console.log("timeout"), 1000);
