
import Timer from "timer";
import Time from "time";

import Monitor from "pins/digital/monitor";
import Digital from "pins/digital";

const BUTTON_DEBOUNCE_MS = 50;

class ClockButton {
	constructor(dict) {
		this.monitor = new Monitor(dict);
		this.monitor.parent = this;
		this.lastPressed = Time.ticks;
		this.timePressed = 0;
		this.timeReleased = 0;
		this.elapsed = 0;
		this.stillDown = 0;
		this.stillDownCycleMS = (dict.cycle !== undefined) ? dict.cycle : 200;
		this.monitor.onChanged = this.buttonStateChanged;
	}

	buttonStateChanged() {
		let now = Time.ticks;
		if (this.read()) {		// if high then button is released
			this.parent.timeReleased = now;
			if (this.parent.timePressed)
				this.parent.elapsed = now - this.parent.timePressed;
			if (this.parent.stillDown)
				this.parent.stillDown = 0;
//			if (undefined !== this.parent.stillDownTimer) {
//				Timer.clear(this.parent.stillDownTimer);
//				this.parent.stillDownTimer = undefined;
//			}
			if (undefined != this.parent.onReleased)
				this.parent.onReleased(this.parent);
		}
		else {					// if low then button is pressed
			this.parent.timePressed = now;
			if ((now - this.lastReleased) < BUTTON_DEBOUNCE_MS)
				return;

			this.parent.elapsed = 0;
			if (undefined !== this.parent.onStillDown) {
				this.parent.stillDown = 1;
/*				this.parent.stillDownTimer = */
Timer.set(id => {
					this.parent.elapsed = Time.ticks - this.parent.timePressed;
					if (this.parent.stillDown) {
						this.parent.onStillDown(this.parent);
					}
					else {
						Timer.clear(id);
					}
				}, this.parent.stillDownCycleMS, this.parent.stillDownCycleMS).parent = this.parent;
//				this.parent.stillDownTimer.parent = this.parent;
			}
			if (undefined != this.parent.onPressed)
				this.parent.onPressed(this.parent);
		}
	}
	
}

export default ClockButton;
