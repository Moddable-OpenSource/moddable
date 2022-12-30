# AudioOut
Copyright 2021-2022 Moddable Tech, Inc.<BR>
Revised: December 4, 2022

## class AudioOut
The `AudioOut` class provides audio playback with a four stream mixer.

```js
import AudioOut from "pins/i2s";
```

### Play a sound
The following example plays a single bell sound. The audio is stored in a resource. The format of the resource is Moddable Audio, which is uncompressed audio with a small header containing the audio encoding.

```js
let bell = new Resource("bell.maud");
let audio = new AudioOut({sampleRate: 11025, bitsPerSample: 16, numChannels: 1, streams: 1});
audio.enqueue(0, AudioOut.Samples, bell);
audio.start();
```

The constructor configures the `sampleRate`, `bitsPerSample`, and `numChannels` for the output. The supported configurations depend on the host hardware. The constructor also configures the maximum number of parallel streams supported by the instance, one in this example.

### Play a sound repeatedly
The following example plays the same bell sound four times in row by passing 4 for the optional `repeat` parameter of the `enqueue` function.

```js
let audio = new AudioOut({sampleRate: 11025, bitsPerSample: 16, numChannels: 1, streams: 1});
audio.enqueue(0, AudioOut.Samples, bell, 4);
audio.start();
```

Set the repeat parameter to `Infinity` to repeat the sound indefinitely:

```js
audio.enqueue(0, AudioOut.Samples, bell, Infinity);
```

### Receive a callback
To receive a callback after a buffer completes playback, enqueue a `Callback` command:

```js
audio.enqueue(0, AudioOut.Samples, bell, 2);
audio.enqueue(0, AudioOut.Callback, 5);
audio.start();
audio.callback = id => trace(`audio callback id ${id}\n`);
```

This example traces the value `5` to the console after the bell sound plays twice.

### Play part of a sound
To play part of a sound, pass the optional `start` and `count` parameters to the `enqueue` function. The start and count parameters use sample number as units, not bytes. The following example plays the second second of the bell sound once followed by the first second twice.

```js
let audio = new AudioOut({sampleRate: 11025, bitsPerSample: 16, numChannels: 1, streams: 1});
audio.enqueue(0, AudioOut.Samples, bell, 1, 11025, 11025);
audio.enqueue(0, AudioOut.Samples, bell, 2, 0, 11025);
audio.start();
```

### Play sounds in parallel
To play sounds in parallel, enqueue them to separate stream. The following sample plays the bell sound once on stream zero in parallel with the beep sound four times on stream one.

```js
let audio = new AudioOut({sampleRate: 11025, bitsPerSample: 16, numChannels: 1, streams: 2});
audio.enqueue(0, AudioOut.Samples, bell);
audio.enqueue(1, AudioOut.Samples, beep, 4);
audio.start();
```

### Flush enqueued audio
The audio queued on a given stream may be flushed by calling `enqueue` with the `Flush` command. The following example flushes stream 1.

```js
audio.enqueue(1, AudioOut.Flush);
```

### Stop playback
To stop playback of all streams, call `stop`.

```js
audio.stop();
```

The `stop` function does not flush any enqueued samples. Calling `start` resumes playback where the audio was stopped.

### Simple Attack-Sustain-Decay
Audio samples often consist of three sections, an initial attack followed by a sustain section that can be seamlessly looped indefinitely, followed by a final decay section for the end of the sample. The `AudioOut` instance can be used to play these kinds of audio samples for durations that are unknown when playback begins, for example playing a sample for the length of time the user holds down a button.

This is done by using the ability to repeat a sample an infinite number of times, until the next sample is queued. To begin the sample playback, enqueue both the attack and sustain sections.

```js
audio.enqueue(0, AudioOut.Samples, aSample, 1, aSample.attackStart, aSample.attackCount);
audio.enqueue(0, AudioOut.Samples, aSample, Infinity, aSample.sustainStart, aSample.sustainCount);
```
	
When it is time to end playback of the sample, enqueue the decay section. This will terminate the enqueue sustain section when it completes the current repetition:

```js
audio.enqueue(0, AudioOut.Samples, aSample, 1, aSample.decayStart, aSample.decayCount);
```

### constructor(dictionary)
The constructor accepts a dictionary to configure the audio output. 

```js
let audio = new AudioOut({sampleRate: 11025, bitsPerSample: 16, numChannels: 2, streams: 3});
```
The following properties are defined.

- **sampleRate** -- The number of samples per second of audio to playback. Sample rates from 8000 to 48000 are supported, though this may be limited on a particular device.
- **bitsPersample** -- The number of bits per sample, either 8 or 16.
- **numChannels** -- The number of channels in each audio sample, 1 for mono and 2 for stereo.
- **streams** -- The maximum number of simultaneous streams supported by this audio out instance. Valid values are from 1 to 4, with a default value of 1.

All audio buffers played on a given an instance of `AudioOut` must have the same sample rate, bits per sample, and number of channels as the AudioOut instance is initialized to in its constructor.

When an audio out is instantiated, it is in the stopped state. It is necessary to call `start` to begin audio playback.

### close()
Call the `close` function to free all resources associated with the audio output. If audio is playing at the time `close` is called, it is immediately stopped.

```js
audio.close();
```

### start()
Call the `start` function to begin playing audio. If no audio is enqueued for playback, silence is played.

```js
audio.start();
```

### stop()
Call the `stop` function to immediately suspend audio playback. 

```js
audio.stop();
```

Calling `stop` has no effect on the state of any audio queued for playback on any streams. Calling `start` after `stop` will resume playback at the point it was suspended.

### enqueue(stream, command, buffer, repeat, offset, count)
The enqueue function has several different functions, all related to the audio queued for playback:

- Enqueuing audio samples for playback on a stream
- Enqueuing a callback at a particular point in a stream's playback
- Flushing the audio queued for playback on a stream

All invocations of the `enqueue` function include the first parameter, the target stream number, a value from 0 to one less than the number of streams configured using the constructor.

The `length` function returns the number of unused queue entries of a stream.

#### Enqueuing audio samples
Audio samples to play may be provided either as a MAUD audio resource or as raw audio samples. In both cases, the samples must be in the same format as the audio output (e.g. have the same sample rate, bits per sample, and number of channels).

To `enqueue` audio samples with a Moddable Audio header (MAUD), call enqueue with a buffer of samples:

```js
audio.enqueue(0, AudioOut.Samples, buffer);
```

To `enqueue` a buffer of uncompressed audio samples with no header call enqueue with a buffer of samples:

```js
audio.enqueue(0, AudioOut.RawSamples, buffer);
```

To play the buffer several times, specify the optional `repeat` count. The repeat count is either a positive integer or `Infinity`.

```js
audio.enqueue(0, AudioOut.Samples, bufferOne, 4);
audio.enqueue(0, AudioOut.Samples, bufferTwo, Infinity);
```

If the repeat count is `Infinity`, the buffer is repeated until the audio out instance is closed, the streaming is flushed, or another buffer of audio is enqueued. In the final case, the currently playing buffer plays to completion, and then the following buffer is played.
	
A subset of the samples in the buffer may be selected for playback by using the optional `offset` and `count` parameters. Both parameters are in units of samples, not bytes. The `offset` parameter indicates the number of samples into the buffer to begin playback. If the `count` parameter is not provided, playback proceeds to the end of the buffer. It the `count` parameter is provided, only the number of samples it specifies are played.

#### Enqueuing tones and silence

To `enqueue` a tone, provide the frequency and number of samples. The square wave will be generated.  The following queues 8000 samples of a 440 Hz A natural. Pass `Infinty` for the sample count to play the tone until `flush`,  `stop`, or `close`. 

```js
audio.enqueue(0, AudioOut.Tone, 440, 8000);
```
To `enqueue` silence, provide the number of samples. Queuing silence is useful for adding precise gaps between audio buffers.  The following queues 11025 samples of silence.

```js
audio.enqueue(0, AudioOut.Silence, 8000);
```

#### Enqueuing callbacks
To schedule a callback at a particular point in a stream, call enqueue with an integer value for the buffer argument. When the buffer preceding the callback completes playback, the instance's `callback` function will be invoked.

```js
audio.enqueue(0, AudioOut.Samples, bufferOne);
audio.enqueue(0, AudioOut.Callback, 1);
audio.enqueue(0, AudioOut.Samples, bufferTwo);
audio.enqueue(0, AudioOut.Callback, 2);
audio.callback = id => trace(`finished playing buffer ${id}\n`);
```

Instead of a single callback function that is called for all streams, a separate callback for each stream maybe provided using the `callbacks` property:

```js
audio.callbacks = [
	id => trace(`finished playing buffer ${id} from stream 0\n`),
	id => trace(`finished playing buffer ${id} from stream 1\n`)
];
```

If both the `callback` and `callbacks` property are set, only the the `callbacks` property is used.

#### Dequeuing audio samples
All of the samples and callbacks enqueued on a specified stream may be dequeued by calling `enqueue` with only the `stream` parameter:

```js
audio.enqueue(3, AudioOut.Flush);
```

#### Changing volume
To change the volume, enqueue a `Volume` command on a stream. The volume command does not change the volume of samples already enqueued, only those enqueued after the `Volume` command.

```js
audio.enqueue(0, AudioOut.Volume, 128);
```

Values for the volume command range from 0 for silent, to 256 for full volume.

### length(stream)

The `length` function returns the number of unused queue entries of the specified stream. This information can be used to determine if the stream is currently able to accept additional calls to `enqueue`.

```js
if (audio.length(0) >= 2) {
	audio.enqueue(0, AudioOut.Tone, 440, 1000);
	audio.enqueue(0, AudioOut.Tone, 330, 1000);
}
```

### Properties

All properties of the audioOut instance are read-only.

#### sampleRate

The sample rate of the instance as a number.

#### bitsPerSample

The number of bits per sample of the instance as a number.

#### channelCount

The number of channels output by the instance as a number.

#### streams

The maximum number of simultaneous streams supported by the instance as a number.

## class Mixer
The `Mixer` class provides access to the four-channel mixer and audio decompressors used by the `AudioOut`. This is useful for processing audio for other purposes, such as network streaming.

```js
import {Mixer} from "pins/i2s";
```

The mixer has the same API foundation as `AudioOut`, including `enqueue`. The mixer does not implement `start` or `stop` methods but instead provides a `mix` function which is used to pull samples that have been queued.

### mix(sampleCount)
### mix(buffer)
The `mix` function can be called in two ways. First, when passed an integer count of the number of samples to mix, it returns a host buffer that contains the samples. Second, when passed a buffer (`ArrayBuffer`, `SharedArrayBuffer`, `Uint8Array`, `Int8Array`, `DataView`), it mixes the samples directly to the provided buffer.

#### Output tone to new buffer
The following code mixes 600 samples of a 440 Hz tone to a new buffer.

```js
const mixer = new Mixer({streams: 1, sampleRate: 12000, numChannels: 1});
mixer.enqueue(0, Mixer.Tone, 440);
const samples = mixer.mix(600);
```

#### Output tone to existing buffer
The following code mixes 600 samples of a 440 Hz tone to an existing buffer.

```js
const samples = new ArrayBuffer(600 * 2);
const mixer = new Mixer({streams: 1, sampleRate: 12000, numChannels: 1});
mixer.enqueue(0, Mixer.Tone, 440);
mixer.mix(samples);
```

## MAUD format
The `maud` format, "Moddable Audio", is a simple audio format intended to be compact and trivially parsed. The `enqueue` function of `AudioOut` class accepts samples in the `maud` format. The `wav2maud` tool in the Moddable SDK converts WAV files to `maud` resources.

The format has a twelve byte header followed immediately by audio samples.

- offset 0 -- ASCII 'm'
- offset 1 -- ASCII 'a'
- offset 2 -- version number (1)
- offset 3 -- bits per sample (8 or 16)
- offset 4 -- sample rate (between 8000 and 48000, inclusive). 2 bytes, unsigned, little-endian
- offset 6 -- number of channels (1 or 2)
- offset 7 -- sample format (0 for uncompressed, 1 for IMA ADPCM, 2 for SBC)
- offset 8 -- sample count. 4 bytes, unsigned little-endian

Audio samples immediately follow the header. If there are two channels, the channels are interleaved. 16-bit samples are signed little-endian values. 8-bit samples are signed values.

IMA ADPCM are based on the algorithm described in "Recommended Practices for Enhancing Digital Audio Compatibility in Multimedia Systems" Revision 3.00 from October 21, 1992. Audio compression is approximately 4:1 for 16 bit samples. Only single channel audio is supported. Each compressed chunk contains 129 samples and uses 68 bytes. Chunks are decompressed one at a time, on demand during playback to minimize in-memory buffers.

## Manifest defines
The `audioOut` module is configured at build time.

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
- `MODDEF_AUDIOOUT_I2S_DAC` - Enable built-in Digital-to-Analog (DAC) output. Set to 1 to enable DAC. Default is 0.
- `MODDEF_AUDIOOUT_I2S_DAC_CHANNEL` - Controls DAC output - left (1), right (2), or both (3). Defaults to both.
- `MODDEF_AUDIOOUT_I2S_PDM` - Enable built-in PDM encoder. Set to 1 to enable PDM. Default is 0.

### Defines for ESP8266
- `MODDEF_AUDIOOUT_I2S_PDM` -- If zero, PCM samples are transmitted over I2S. If non-zero, samples are transmitted using PDM. Set to 32 for no oversampling, 64 for 2x oversampling, and 128 for 4x oversampling. Default is 0.
