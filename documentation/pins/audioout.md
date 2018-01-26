# AudioOut
Copyright 2018 Moddable Tech, Inc.

Revised: January 25, 2018

**Warning**: These notes are preliminary. Omissions and errors are likely. If you encounter problems, please ask for assistance.

## class AudioOut
The `AudioOut` class provides audio playback with a four stream mixer.

	import AudioOut from "pins/i2s";

### Play a sound
The following example plays a single bell sound. The audio is stored in a resource. The format of the resource is Moddable Audio, which is uncompressed audio with a small header containing the audio encoding.

	let bell = new Resource("bell.maud");
	let audio = new AudioOut({sampleRate: 11025, bitsPerSample: 16, numChannels: 1, streams: 1});
	audio.enqueue(0, bell);
	audio.start();

The constructor configures the `sampleRate`, `bitsPerSample`, and `numChannels` for the output. The supported configurations depend on the host hardware. The constructor also configures the maximum number of parallel streams supported by the instance, one in this example.

### Play a sound repeatedly
The following example plays the same bell sound four times in row by passing 4 for the optional `repeat` parameter of the `enqueue` function.

	let audio = new AudioOut({sampleRate: 11025, bitsPerSample: 16, numChannels: 1, streams: 1});
	audio.enqueue(0, bell, 4);
	audio.start();

Set the repeat parameter to `Infinity` to repeat the sound indefinitely:

	audio.enqueue(0, bell, Infinity);

### Receive a callback
To receive a callback after a buffer completes playback, enqueue an integer in place of the samples:

	audio.enqueue(0, bell, 2);
	audio.enqueue(0, 5);
	audio.start();
	audio.callback = id => trace(`audio callback id ${id}\n`);

This example traces the value `5` to the console after the bell sound plays twice.

### Play part of a sound
To play part of a sound, pass the optional `start` and `count` parameters to the `enqueue` function. The start and count parameters use sample number as units, not bytes. The following example plays the second second of the bell sound once followed by the first second twice.

	let audio = new AudioOut({sampleRate: 11025, bitsPerSample: 16, numChannels: 1, streams: 1});
	audio.enqueue(0, bell, 1, 11025, 11025);
	audio.enqueue(0, bell, 2, 0, 11025);
	audio.start();

### Play sounds in parallel
To play sounds in parallel, enqueue them to separate stream. The following sample plays the bell sound once on stream zero in parallel with the beep sound four times on stream one.

	let audio = new AudioOut({sampleRate: 11025, bitsPerSample: 16, numChannels: 1, streams: 2});
	audio.enqueue(0, bell);
	audio.enqueue(1, beep, 4);
	audio.start();

### Flush enqueued audio
The audio queued on a given stream may be flushed by calling `enqueue` with only the stream parameter. The following example flushes stream 1.

	audio.enqueue(1);

### Stop playback
To stop playback of all streams, call `stop`.

	audio.stop();

The `stop` function does not flush any enqueued samples. Calling `start` resumes playback where the audio was stopped.

### Simple Attack-Sustain-Decay
Audio samples often consist of three sections, an initial attack followed by a sustain section that can be seamlessly looped indefinitely, followed by a final decay section for the end of the sample. The `AudioOut` instance can be used to play these kinds of audio samples for durations that are unknown when playback begins, for example playing a sample for the length of time the user holds down a button.

This is done by using the ability to repeat a sample an infinite number of times, until the next sample is queued. To begin the sample playback, enqueue both the attack and sustain sections.

	audio.enqueue(0, aSample, 1, aSample.attackStart, aSample.attackCount);
	audio.enqueue(0, aSample, Infinity, aSample.sustainStart, aSample.sustainCount);
	
When it is time to end playback of the sample, enqueue the decay section. This will terminate the enqueue sustain section when it completes the current repetition:

	audio.enqueue(0, aSample, 1, aSample.decayStart, aSample.decayCount);

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

### enqueue(stream, buffer, repeat, offset, count)
The enqueue function has several different functions, all related to the audio queued for playback:

- Enqueuing audio samples for playback on a stream
- Enqueuing a callback at a particular point in a stream's playback
- Flushing the audio queued for playback on a stream

All invocations of the `enqueue` function include the first parameter, the target stream number, a value from 0 to one less than the number of streams configured using the constructor.

#### Enqueuing audio samples
To `enqueue` audio samples call enqueue with a buffer of samples:

	audio.enqueue(0, buffer);

To play the buffer several times, specify the optional `repeat` count. The repeat count is either a positive integer or `Infinity`.

	audio.enqueue(0, bufferOne, 4);
	audio.enqueue(0, bufferTwo, Infinity);

If the repeat count is `Infinity`, the buffer is repeated until the audio out instance is closed, the streaming is flushed, or another buffer of audio is enqueued. In the final case, the currently playing buffer plays to completion, and then the following buffer is played.
	
A subset of the samples in the buffer may be selected for playback by using the optional `offset` and `count` parameters. Both parameters are in units of samples, not bytes. The `offset` parameter indicates the number of samples into the buffer to begin playback. If the `count` parameter is not provided, playback proceeds to the end of the buffer. It the `count` parameter is provided, only the number of samples it specifies are played.

#### Enqueuing callbacks
To schedule a callback at a particular point in a stream, call enqueue with an integer value for the buffer argument. When the buffer preceding the callback completes playback, the instance's `callback` function will be invoked.

	audio.enqueue(0, bufferOne);
	audio.enqueue(0, 1);
	audio.enqueue(0, bufferTwo);
	audio.enqueue(0, 2);
	audio.callback = id => trace(`finished playing buffer ${id}\n`);

#### Dequeuing audio samples
All of the samples and callbacks enqueued on a specified channel may be dequeued by calling `enqueue` with only the `stream` parameter:

	audio.enqueue(3);
