const minimumOptions = {
	static: 8192,
	heap: {
		initial: 64,
		incremental: 64
	},
	chunk: {
		initial: 1024,
		incremental: 512
	},
	stack: 64,
	keys: {
		initial: 1
	}
};

export default minimumOptions;
