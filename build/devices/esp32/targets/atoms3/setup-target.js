import Digital from "pins/digital";
//import Monitor from "monitor";
import M5Button from "m5button";
import config from "mc/config";
import Timer from "timer";
import Button from "button";
import I2C from "pins/i2c";
import MPU6886 from "mpu6886";

class Flash {
        constructor(options) {
                return new Button({
                        ...options,
                        pin: 0,
                        invert: true
                });
        }
}

globalThis.Host = Object.freeze({
        Button: {
                Default: Flash,
                Flash
        }
}, true);

export default function (done) {
        globalThis.button = {
                a: new M5Button(41)
        };

        const sensor = new MPU6886;
        globalThis.accelerometer = new Accelerometer(sensor);
        globalThis.gyro = new Gyro(sensor);

        done();
}

class Accelerometer {
        #sensor;
        #timer;

        constructor(sensor) {
                this.#sensor = sensor;
        }
        start(frequency) {
                this.stop();
                this.#timer = Timer.repeat(id => {
                        if (!this.onreading)
                                return;

                        this.#sensor.configure({ operation: "accelerometer" });
                        const sample = this.#sensor.sample();
                        if (sample)
                                this.onreading({x: sample.x, y: -sample.y, z: -sample.z});
                }, frequency);
        }
        stop() {
                if (undefined !== this.#timer)
                        Timer.clear(this.#timer);
                this.#timer = undefined;
        }
}

class Gyro {
        #sensor;
        #timer;

        constructor(sensor) {
                this.#sensor = sensor;
        }
        start(frequency) {
                this.stop();
                this.#timer = Timer.repeat(id => {
                        if (!this.onreading)
                                return;

                        this.#sensor.configure({ operation: "gyroscope" });
                        const sample = this.#sensor.sample();
                        if (sample)
                                this.onreading({x: -sample.y, y: -sample.x, z: -sample.z});
                }, frequency);
        }
        stop() {
                if (undefined !== this.#timer)
                        Timer.clear(this.#timer);
                this.#timer = undefined;
        }
}
