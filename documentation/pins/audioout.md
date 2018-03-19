# AudioOut
Copyright 2018 Moddable Tech, Inc.

Revised: March 19, 2018

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## class AudioOut
The `AudioOut` class provides audio playback with a four stream mixer.

	import AudioOut from "pins/i2s";

### Play a sound
The following example plays a single bell sound. The audio is stored in a resource. The format of the resource is Moddable Audio, which is uncompressed audio with a small header containing the audio encoding.

	let bell = new Resource("bell.maud");
	let audio = new AudioOut({sampleRate: 11025, bitsPerSample: 16, numChannels: 1, streams: 1});
	audio.enqueue(0, AudioOut.Samples, bell);
	audio.start();

The constructor configures the `sampleRate`, `bitsPerSample`, and `numChannels` for the output. The supported configurations depend on the host hardware. The constructor also configures the maximum number of parallel streams supported by the instance, one in this example.

### Play a sound repeatedly
The following example plays the same bell sound four times in row by passing 4 for the optional `repeat` parameter of the `enqueue` function.

	let audio = new AudioOut({sampleRate: 11025, bitsPerSample: 16, numChannels: 1, streams: 1});
	audio.enqueue(0, AudioOut.Samples, bell, 4);
	audio.start();

Set the repeat parameter to `Infinity` to repeat the sound indefinitely:

	audio.enqueue(0, AudioOut.Samples, bell, Infinity);

### Receive a callback
To receive a callback after a buffer completes playback, enqueue a `Callback` command:

	audio.enqueue(0, AudioOut.Samples, bell, 2);
	audio.enqueue(0, AudioOut.Callback, 5);
	audio.start();
	audio.callback = id => trace(`audio callback id ${id}\n`);

This example traces the value `5` to the console after the bell sound plays twice.

### Play part of a sound
To play part of a sound, pass the optional `start` and `count` parameters to the `enqueue` function. The start and count parameters use sample number as units, not bytes. The following example plays the second second of the bell sound once followed by the first second twice.

	let audio = new AudioOut({sampleRate: 11025, bitsPerSample: 16, numChannels: 1, streams: 1});
	audio.enqueue(0, AudioOut.Samples, bell, 1, 11025, 11025);
	audio.enqueue(0, AudioOut.Samples, bell, 2, 0, 11025);
	audio.start();

### Play sounds in parallel
To play sounds in parallel, enqueue them to separate stream. The following sample plays the bell sound once on stream zero in parallel with the beep sound four times on stream one.

	let audio = new AudioOut({sampleRate: 11025, bitsPerSample: 16, numChannels: 1, streams: 2});
	audio.enqueue(0, AudioOut.Samples, bell);
	audio.enqueue(1, AudioOut.Samples, beep, 4);
	audio.start();

### Flush enqueued audio
The audio queued on a given stream may be flushed by calling `enqueue` with the `Flush` command. The following example flushes stream 1.

	audio.enqueue(1, AudioOut.Flush);

### Stop playback
To stop playback of all streams, call `stop`.

	audio.stop();

The `stop` function does not flush any enqueued samples. Calling `start` resumes playback where the audio was stopped.

### Simple Attack-Sustain-Decay
Audio samples often consist of three sections, an initial attack followed by a sustain section that can be seamlessly looped indefinitely, followed by a final decay section for the end of the sample. The `AudioOut` instance can be used to play these kinds of audio samples for durations that are unknown when playback begins, for example playing a sample for the length of time the user holds down a button.

This is done by using the ability to repeat a sample an infinite number of times, until the next sample is queued. To begin the sample playback, enqueue both the attack and sustain sections.

	audio.enqueue(0, AudioOut.Samples, aSample, 1, aSample.attackStart, aSample.attackCount);
	audio.enqueue(0, AudioOut.Samples, aSample, Infinity, aSample.sustainStart, aSample.sustainCount);
	
When it is time to end playback of the sample, enqueue the decay section. This will terminate the enqueue sustain section when it completes the current repetition:

	audio.enqueue(0, AudioOut.Samples, aSample, 1, aSample.decayStart, aSample.decayCount);

### constructor(dictionary)
The constructor accepts a dictionary to configure the audio output. 

	let audio = new AudioOut({sampleRate: 11025, bitsPerSample: 16, numChannels: 2, streams: 3});

The following properties are defined.

- **sampleRate** -- The number of samples per second of audio to playback. Sample rates from 8000 to 48000 are supported, though this may be limited on a particular device.
- **bitsPersample** -- The number of bits per sample, either 8 or 16.
- **numChannels** -- The number of channels in each audio sample, 1 for mono and 2 for stereo.
- **streams** -- The maximum number of simultaneous streams supported by this audio out instance. Valid values are from 1 to 4, with a default value of 1.

All audio buffers played on a given an instance of `AudioOut` must have the same sample rate, bits per sample, and number of channels as the AudioOut instance is initialized to in its constructor.

When an audio out is instantiated, it is in the stopped state. It is necessary to call `start` to begin audio playback.

### close()
Call the `close` function to free all resources associated with the audio output. If audio is playing at the time `close` is called, it is immediately stopped.

	audio.close();

### start()
Call the `start` function to begin playing audio. If no audio is enqueued for playback, silence is played.

	audio.start();

### stop()
Call the `stop` function to immediately suspend audio playback. 

	audio.stop();

Calling `stop` has no effect on the state of any audio queued for playback on any streams. Calling `start` after `stop` will resume playback at the point it was suspended.

### enqueue(stream, command, buffer, repeat, offset, count)
The enqueue function has several different functions, all related to the audio queued for playback:

- Enqueuing audio samples for playback on a stream
- Enqueuing a callback at a particular point in a stream's playback
- Flushing the audio queued for playback on a stream

All invocations of the `enqueue` function include the first parameter, the target stream number, a value from 0 to one less than the number of streams configured using the constructor.

#### Enqueuing audio samples
Audio samples to play may be provided either as a MAUD audio resource or as raw audio samples. In both cases, the samples must be in the same format as the audio output (e.g. have the same sample rate, bits per sample, and number of channels).

To `enqueue` audio samples with a Moddable Audio header (MAUD), call enqueue with a buffer of samples:

	audio.enqueue(0, AuioOut.Samples, buffer);

To `enqueue` a buffer of audio samples with no header call enqueue with a buffer of samples:

	audio.enqueue(0, AudioOut.RawSamples, buffer);

To play the buffer several times, specify the optional `repeat` count. The repeat count is either a positive integer or `Infinity`.

	audio.enqueue(0, AudioOut.Samples, bufferOne, 4);
	audio.enqueue(0, AudioOut.Samples, bufferTwo, Infinity);

If the repeat count is `Infinity`, the buffer is repeated until the audio out instance is closed, the streaming is flushed, or another buffer of audio is enqueued. In the final case, the currently playing buffer plays to completion, and then the following buffer is played.
	
A subset of the samples in the buffer may be selected for playback by using the optional `offset` and `count` parameters. Both parameters are in units of samples, not bytes. The `offset` parameter indicates the number of samples into the buffer to begin playback. If the `count` parameter is not provided, playback proceeds to the end of the buffer. It the `count` parameter is provided, only the number of samples it specifies are played.

#### Enqueuing callbacks
To schedule a callback at a particular point in a stream, call enqueue with an integer value for the buffer argument. When the buffer preceding the callback completes playback, the instance's `callback` function will be invoked.

	audio.enqueue(0, AudioOut.Samples, bufferOne);
	audio.enqueue(0, AudioOut.Callback, 1);
	audio.enqueue(0, AudioOut.Samples, bufferTwo);
	audio.enqueue(0, AudioOut.Callback, 2);
	audio.callback = id => trace(`finished playing buffer ${id}\n`);

#### Dequeuing audio samples
All of the samples and callbacks enqueued on a specified stream may be dequeued by calling `enqueue` with only the `stream` parameter:

	audio.enqueue(3, AudioOut.Flush);

#### Changing volume
To change the volume, enqueue a `Volume` command on a stream. The volume command does not change the volume of samples already enqueued, only those enqueued after the `Volume` command.

	audio.enqueue(0, AudioOut.Volume, 128);

Values for the volume command range from 0 for silent, to 256 for full volume.

## MAUD format
The `maud` format, "Moddable Audio", is a simple uncompressed audio format intended to be compact and trivially parsed. The `enqueue` function of `AudioOut` class accepts samples in the `maud` format. The `wav2maud` tool in the Moddable SDK converts WAV files to `maud` resources.

The format has a twelve byte header followed immediately by audio samples.

- offset 0 -- ASCII 'm'
- offset 1 -- ASCII 'a'
- offset 2 -- version number (1)
- offset 3 -- bits per sample (8 or 16)
- offset 4 -- sample rate (between 8000 and 48000, inclusive). 2 bytes, unsigned, little-endian
- offset 6 -- number of channels (1 or 2)
- offset 7 -- unused (0)
- offset 8 -- sample count. 4 bytes, unsigned little-endian

Audio samples immediately follow the header. If there are two channels, the channels are interleaved. 16-bit samples are signed little-endian values. 8-bit samples are signed values.

## Manifest defines
The `audioOut` module is be configured at build time.

### Defines for all targets
- `MODDEF_AUDIOOUT_STREAMS` -- maximum number of simultaneous audio streams. The maximum is 4 and the default is 4.
- `MODDEF_AUDIOOUT_BITSPERSAMPLE` -- number of bits per sample. macOS supports 8 and 16. ESP32 and ESP8266 support 16 only. The default is 16.
- `MODDEF_AUDIOOUT_QUEUELENGTH` -- maximum number of items that may be queued on a single stream. Default is 8.

### Defines for ESP32
- `MODDEF_AUDIOOUT_I2S_NUM` -- The ESP32 I2S unit number. Default is 0.
- `MODDEF_AUDIOOUT_I2S_BCK_PIN` -- The I2S clock pin. The default is 26.
- `MODDEF_AUDIOOUT_I2S_LR_PIN` -- The I2S LR pin. The default is 25.
- `MODDEF_AUDIOOUT_I2S_DATAOUT_PIN` -- The I2S data pin. The default is 22.
- `MODDEF_AUDIOOUT_I2S_BITSPERSAMPLE` - Number of bits per sample to transmit over I2S, either 16 or 32. Default is 16.

### Defines for ESP8266
- `MODDEF_AUDIOOUT_I2S_PDM` -- If zero, PCM samples are transmitted over I2S. If non-zero, samples are transmitted using PDM. Set to 32 for no oversampling, 64 for 2x oversampling, and 128 for 4x oversampling. Default is 0.
