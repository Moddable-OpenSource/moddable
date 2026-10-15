// only used by commodetto apps
export default class {
	#points = [];
	constructor(options = {}) {
		screen.context = this;
	}
	configure(options = {}) {
	}
	onTouchBegan(id, x, y, when) {
		this.#points.push({ id, x, y });
	}
	onTouchEnded(id, x, y, when) {
		const index = this.#points.findIndex(point => point.id == id);
		if (index >= 0) {
			this.#points.splice(index, 1);
		}	
	}
	onTouchMoved(id, x, y, when) {
		const point = this.#points.find(point => point.id == id);
		if (point) {
			point.x = x;
			point.y = y;
		}
	}
	read(points) {
		debugger
	}
	sample() {
		return this.#points;
	}
}
