import Digital from "builtin/digital";

const led = new Digital({
   pin: 2,
   mode: Digital.Output,
});
led.write(1);		// off

let state = 0;
System.setInterval(() => {
	led.write(state);
	state ^= 1;
}, 200);
