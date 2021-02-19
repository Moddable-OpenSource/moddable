import M5Button from "m5button";
import AudioOut from "pins/audioout";
import Resource from "Resource";
import config from "mc/config";
import Timer from "timer";

export default function (done) {
	global.button = {
		a: new M5Button(39),
		b: new M5Button(38),
		c: new M5Button(37)
	};

	if (config.startupSound) {
	const speaker = new AudioOut({streams: 1});
	speaker.callback = function () {
		this.stop();
		this.close();
		this.done();
	};
		speaker.done = done;
		done = undefined;

		speaker.enqueue(0, AudioOut.Samples, new Resource(config.startupSound));
		speaker.enqueue(0, AudioOut.Callback, 0);
		speaker.start();
	}

	//@@ microphone

	done?.();
}
