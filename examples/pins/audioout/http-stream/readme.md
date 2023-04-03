# Audio Streamers
Copyright 2022-2023 Moddable Tech, Inc.<BR>
Revised: March 13, 2023

The `WavStream` and `SBCStream` classes plays audio streams delivered over HTTP. The `WavStream` class plays uncompressed WAV audio files and `Audio/L16`; the `SBCStream` class plays [SBC compressed](https://en.wikipedia.org/wiki/SBC_%28codec%29) audio. SBC is a low-complexity format used primarily by Bluetooth. Its relatively high quality and simple decoder make it well suited for microcontrollers. 

The `WavStream` and `SBCStream` classes share a common API but have separate implementations. They uses the following modules:

- `HTTPClient` from ECMA-419 2nd Edition (draft). The API design of the HTTP request makes buffering of streaming data very straightforward.
- `AudioOut` class for audio playback
- `WavReader` class to parse the header of the received WAV file (`WavStream` only)

The API itself follows the ECMA-419 style as much as possible, including passing in the full HTTP configuration (with any necessary constructors) and all callbacks.

The `AudioOut` instance is provided by the calling script, which also specifies the stream index to use for playback. This leaves the remaining streams available for other audio (including additional instances of `WavStream` and `SBCStream`).

The following is a simple example of using `WavStream` to play a stream.

```js
const audio = new AudioOut({});

new WavStream({
	http: device.network.http,
	host: "www.example.com",
	path: "/myaudio_11025.wav",
	audio: {
		out: audio
	}
});

audio.start();
```

To stream SBC instead, change the constructor from `WavStream` to `SBCStream` and update the `path` property for the SBC audio stream.

The streamer API provides several callbacks to manage the streaming session, including notification of streaming stalls and playback completion. The callbacks are documented below as part of the options object of the constructor.

The streamers try to keep one second of audio buffers queued with the audio output. Once one second of audio is buffered, playback begins. Should the buffered bytes drop to 0, playback stops until one second of audio is again buffered.

The `HTTPClient` does not yet implement `TLS`. When it does, `WavStream` and `SBCStream` will support streaming over HTTPS.

### `Audio/L16` Streams
Uncompressed audio streams are the data portion of a WAVE file with the sample rate and channel count specified in the MIME type. These are useful for live-streaming of uncompressed audio, and is also used for live transcoding of compressed data to lightweight clients. It is specified by [RFC 2586](https://datatracker.ietf.org/doc/html/rfc2586). The data is delivered in network byte order (big-endian) and the WavStreamer converts it to little-endian for playback.

The following command line allows ffmpeg to be used as simple server for testing Audio/L16 streaming:

```
ffmpeg -i bflatmajor.wav -listen 1 -content_type "audio/L16;rate=11025&channels=1" -f s16be -ar 11025 -acodec -ac 1 pcm_s16be http://127.0.0.1:8080
```

### SBC streams
SBC audio files are a sequence of SBC audio frames without a header or additional framing. The following command line uses ffmpeg to generate an SBC audio file that may be delivered from any HTTP server.

```
ffmpeg -i bflatmajor.wav -ac 1 -ar 16000 -b:a 32k ~/bflatmajor.sbc
```

### Stereo Streams
`WavStream` accepts stereo data and converts mixes it to mono for playback. This is provided for compatibility with existing audio sources as it is clearly not an optimal use of network bandwidth.

`SBCStream` rejects stereo data.

### Underflow (stalls)
Streaming stalls are inevitable. When the streamers run out of audio to play, they enter a "not ready" state. If an `onReady` callbacks is provided, it is called with `false` to indicate that audio playback is paused. The streamers do not stop the audio channel when stalled as this would stop all audio streams. Instead, when stalled, the streamer does not queue any buffers to the audio instance until it has accumulated at least one full second of audio for playback. The script may stop playback when stalled, if it wants to control when audio playback resumes.

## API reference

### `constructor(options)`

The options object may contain the following properties. The `http`, `host`, `path`, and `audio.out` properties are required.

- `http` - the HTTP client configuration. This usually comes from the host provider at `device.network.http`
- `host` - the HTTP host to connect to stream from
- `port` - the remote port to connect to
- `path` - the path of the HTTP resource to stream
- `request` - an optional object that can contain additional [HTTP request](../../../../documentation/network/network.md#http-request) options such as `method`, `headers`, `body` etc. These options will be passed directly to the HTTP client. If not specified, default options will be used.
- `audio.out` - the audio output instance to play the audio on
- `audio.stream` - the stream number of the audio output to use to play the audio. Defaults to `0`.
- `waveHeaderBytes` - the number of bytes to buffer before parsing the WAV header. Defaults to `512` (only supported by `WavStream`)
- `bufferDuration` - the duration in milliseconds of audio to buffer during playback. Playback starts when this target is reached. Defaults to `1000` milliseconds.
- `onPlayed(buffer)` - callback function invoked after the audio output is done with an audio buffer. The audio is uncompressed for `WavStream` and SBC compressed for `SBCStream`. This callback is useful for calculating the RMS of uncompressed audio that is playing.
- `onReady(ready)` - callback function invoked with `true` when there is enough audio buffered to be able to begin playback and `false` when there is an audio buffer underflow that causes playback to pause
- `onError(e)` - callback function invoked on a fatal error, such as the remote endpoint disconnecting
- `onDone()` - callback function invoked the complete stream has been successfully played

### `close()`

Stops playback on the specified stream and frees all resources.
