import DigitalPotentiometer from "embedded:peripheral/Potentiometer/X9C";
import Timer from "timer"

const pot = new DigitalPotentiometer({
	increment: {
		io: device.io.Digital,
		pin: 14
	},
	upDown: {
		io: device.io.Digital,
		pin: 12
	},
	csNVRAMWrite: {
		io: device.io.Digital,
		pin: 13
	}
});


for (let i = 25; i <= 33; i++)
{
	pot.configure({
		wiper: i
	});
	trace(`Wiper Value: ${pot.wiper}\n`);
	Timer.delay(1000);
}


pot.close();
