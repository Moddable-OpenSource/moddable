import Sensor from "embedded:sensor/OxygenGasSensor-OSensor/SEN0322";
import Timer from "timer";

const knownOxygenVal = 20.9; // Known concentration of oxygen in the air, for calibration
const oxygenMV = 0; // The value marked on the sensor, set to 0 unless otherwise required

const sensor = new Sensor({
    sensor: {
        ...device.I2C.default,
        io: device.io.I2C
    }
});

sensor.configure({
    vol: knownOxygenVal,
    mv: oxygenMV
});

Timer.repeat(() =>
{
    const sample = sensor.sample();
    trace(`O2: ${sample.O} ppm\n`);
}, 500);