import M5Button from "m5button";
import AudioOut from "pins/audioout";
import Resource from "Resource";
import config from "mc/config";

export default function (done) {
	global.button = {
		a: new M5Button(39),
		b: new M5Button(38),
		c: new M5Button(37)
	};

	global.speaker = new AudioOut({streams: 4, });
	if (config.startupSound) {
		speaker.callback = function() {this.stop()};
		speaker.enqueue(0, AudioOut.Samples, new Resource(config.startupSound));
		speaker.enqueue(0, AudioOut.Callback, 0);
		speaker.start();
	}

	//@@ microphone

	done();
}
