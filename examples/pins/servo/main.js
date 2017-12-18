import Servo from "pins/servo";
import Timer from "timer";

let servo = new Servo({pin: 5});

let angle = 0;
Timer.repeat(() => {
	angle += 2.5;
	if (angle > 180) angle -= 180;
	servo.write(angle);
}, 250);
