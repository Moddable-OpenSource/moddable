 struct MAUD {
    char		tag[2];
    int8_t		version;
    int8_t		bitsPerSample;
    uint16_t	sampleRate;
    int8_t		channels;
    int8_t		sampleFormat;
    uint32_t	bufferSamples;
 };
 
 enum SampleFormat {
 	Uncompressed = 0,
 	IMA = 1,
 	SBC = 2,
 	Tone = 3,
 	Silence = 4
 };
