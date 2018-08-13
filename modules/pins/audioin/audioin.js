class AudioIn @ "xs_audioin_destructor" {
	constructor() @ "xs_audioin";
	close() @ "xs_audioin_close";
	read(samples) @ "xs_audioin_read";

	get sampleRate() @ "xs_audioin_get_sampleRate";
	get bitsPerSample() @ "xs_audioin_get_bitsPerSample";
	get numChannels() @ "xs_audioin_get_numChannels";
}
Object.freeze(AudioIn.prototype);

export default AudioIn;

