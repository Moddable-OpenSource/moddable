import M5Button from "m5button";
import Button from "button";

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

        done();
}
