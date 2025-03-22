#pragma endian(little)

// flattened RIFF

struct WaveHeader {
	char		riffTag[4];
	uint32_t	riffLength;
	char		waveTag[4];
	char		fmtTag[4];
	uint32_t	fmtLength;
	uint16_t	format;
	uint16_t	channels;
	uint32_t	sampleRate;
	uint32_t	byteRate;
	uint16_t	blockAlign;
	uint16_t	bitsPerSample;
	char		dataTag[4];
	uint32_t	dataLength;
};
