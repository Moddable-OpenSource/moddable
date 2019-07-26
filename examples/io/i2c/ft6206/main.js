import Touch from "sensor/touch";

const touch = new Touch({
	scl: 4,
	sda: 5,
});

while (true) {
	const points = touch.sample();
	if (!points.length)
		continue;

	if (points[0])
		trace(`Point 1 {${points[0].x}, ${points[0].y}}\n`);

	if (points[1])
		trace(`Point 2 {${points[1].x}, ${points[1].y}}\n`);
}
