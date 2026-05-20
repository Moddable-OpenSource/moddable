# Conversation
Copyright 2025 Moddable Tech, Inc.<BR>
Updated December 3, 2025

## Architecture

The conversation module uses a JavaScript [worker](https://moddable.com/documentation/base/worker). The worker is in charge of networks protocols, communicating with the AI cloud services, and encoding/decoding audio samples.

The conversation module and its worker communicate with [marshalled messages](https://www.moddable.com/documentation/xs/XS%20Marshalling). They share input and output audio buffers for efficiency. This document describes the [messages](#Messages).

Because audio samples are transmitted as Base64 encoded data embedded in JSON, workers use a special parser to optimize memory usage and throughput. This document also describes the [`JSONBase64Parser`](#JSONBase64Parser).

The conversation library implements support for various AI cloud services using this worker architecture:

- [OpenAI Realtime](https://platform.openai.com/docs/guides/realtime)
- [Google Gemini Live](https://ai.google.dev/api/multimodal-live)
- [Hume Empathic Voice Interface](https://dev.hume.ai/docs/empathic-voice-interface-evi/overview)
- [Eleven Labs Conversational AI](https://elevenlabs.io/docs/conversational-ai/overview)
- [Deepgram Voice Agent](https://deepgram.com/product/voice-agent-api)

## Programming Interface

The conversation module exports one class that handles worker messages and audio input/output. Applications use a cloud service thru an instance of that class.

#### `constructor(options)`

The `options` object selects and configures a service. Its properties are:

- `specifier`: *string*, the specifier of the service, `"deepgramAgent"`, `"elevenLabsAgent"`, `"googleGeminiLive"`, `"humeAIEVI"` or `"openAIRealtime"`.
- `instructions`: *string*, the instructions that provide guidance to the service
- `functions`: *array* of function descriptions, optional
- `voiceID`: *string*, the identifier of the voice, optional
- `providerID`: *string*, the identifier of the language model provider, optional
- `modelID`: *string*, the identifier of the language model, optional
- `apiKey`: *string*, the API key of the AI cloud services, optional

The `options` object can also provides callbacks. All callbacks are optional.

- `onFunctionCall(call, name, params)`: called when a function call is received, see [receiveFunctionCall](#msg-receiveFunctionCall).
- `onInputLevelChanged(level)`: called when the audio input level changed, `level` is an integer, the average of audio input samples.
- `onInputTranscript(text, more)`: called when the audio input transcription is received, `text ` is a string, `more` is a boolean to tell if more text is expected.
- `onOutputLevelChanged(level)`: called when the audio output level changed, `level` is an integer, the average of audio output samples.
- `onOutputTranscript(text, more)`: called when the audio output transcription is received, `text ` is a string, `more` is a boolean to tell if more text is expected.
- `onStateChanged(state)`: called when the state of the conversation changed, `state` is an integer, values are exported by the class:

```
	static FAILED = -1;
	static DISCONNECTED = 0;
	static DISCONNECTING = 1;
	static CONNECTING = 2;
	static CONNECTED = 3;
	static SPEAKING = 4;		// user is speaking (sending audio to cloud)
	static LISTENING = 5;		// user is listening (receiving audio from cloud)
	static WAITING = 6;
```

> When the state is `FAILED`, application can get details from the `error` property of the class instance.

#### `connect()`

Use `connect` to connect to the service. Applications have to wait for the `CONNECTED` state.

#### `disconnect()`

Use `disconnect` to connect to the service. Applications have to wait for the `DISCONNECTED` state.

#### `sendFunctionResult(call, result)`

Use `sendFunctionResult` to send the result of a function call, see [sendFunctionResult](#msg-sendFunctionResult).

#### `sendText(text)`

Use `sendText` to inform the service about user interactions that did not involve speech.


<a id="Messages"></a>

## Messages

### Application to Worker

- [configure](#msg-configure)
- [connect](#msg-connect)
- [disconnect](#msg-disconnect)
- [sendAudio](#msg-sendAudio)
- [sendFunctionResult](#msg-sendFunctionResult)
- [sendText](#msg-sendText)


<a id="msg-configure"></a>
#### configure

- `instructions`: *string*, the instructions that provide guidance to the service
- `functions`: *array* of function descriptions, optional
- `voiceID`: *string*, the identifier of the voice, optional
- `providerID`: *string*, the identifier of the language model provider, optional
- `modelID`: *string*, the identifier of the language model, optional
- `apiKey`: *string*, the API key of the AI cloud services, optional

The `voiceID`, `providerID` and `modelID` are specific to each service. Look at [ConversationalAI assets](https://github.com/Moddable-OpenSource/moddable/blob/public/contributed/conversationalAI/assets.js) to get voice, provider and model identifiers, names and descriptions by service.

The format of function descriptions is a JSON schema that is more or less common to all services.

```javascript
{
	id:"configure",
	instructions: "You are a helpful lighting system bot. You can turn lights on and off. Do not perform any other tasks.",
	functions: [
		{
			name: "turn_light_on",
			description: "Turn the light on. Call this whenever you need to turn on a light, for example when a customer tells 'turn bedroom light on.'",
			parameters: {
				type: "object",
				properties: {
					light_name: {
						type: "string",
						description: "The name of the light."
					}
				},
				required: ["light_name"],
			}
		},
		{
			name: "turn_light_off",
			description: "Turn the light off. Call this whenever you need to turn off a light, for example when a customer tells 'turn bedroom light off.'",
			parameters: {
				type: "object",
				properties: {
					light_name: {
						type: "string",
						description: "The name of the light."
					}
				},
				required: ["light_name"],
			}
		}
	]
}
```

The application must configure the worker once, before connecting the worker to the service.

<a id="msg-connect"></a>
#### connect

- `inputBuffer`: *object*, a `SharedArrayBuffer` instance
- `outputBuffer`: *object*, a `SharedArrayBuffer` instance

Tell the worker to connect to the service. Both buffers are used as circular buffers for audio samples.

```javascript
{
	id: 'connect',
	inputBuffer: this.inputBuffer,
	outputBuffer: this.outputBuffer,
}
```

The application waits for the [**connected** message](#msg-connected) to begin sending audio samples to the worker.

<a id="msg-disconnect"></a>
#### disconnect

Tell the worker to disconnect from the service.

```javascript
{
	id: 'disconnect'
}
```
The application waits for the **disconnected** message.

<a id="msg-sendAudio"></a>
#### sendAudio

- `offset`: *number*, the byte offset of audio samples in the input buffer
- `size`: *number*, the byte length of audio samples in the input buffer

Tell the worker to send audio samples to the service.

```javascript
{
	id: 'sendAudio',
	offset: 1024,
	size: 1024
}
```

<a id="msg-sendFunctionResult"></a>
#### sendFunctionResult

- `call`: *string*, a unique identifier for the call
- `result`: *string*, a description of the result

Tell the worker to send the function result to the service.

```javascript
{
	id: 'sendFunctionResult',
	call: "1234567",
	result: "The kitchen light is on."
}
```

The `call` identifier comes from the [**receiveFunctionCall** message](#msg-receiveFunctionCall).

<a id="msg-sendText"></a>
#### sendText

- `text`: string

Tell the worker to send text to the service.

```javascript
{
	id: 'sendText',
	text: "The kitchen light is on."
}
```

The application uses the **sendText** message to inform the service about user interactions that did not involve speech.

### Worker to Application

- [configureAudio](#msg-configureAudio)
- [disconnected](#msg-disconnected)
- [failed](#msg-failed)
- [listen](#msg-listen)
- [receiveAudio](#msg-receiveAudio)
- [receiveFunctionCall](#msg-receiveFunctionCall)
- [receiveInputText](msg-receiveInputText)
- [receiveOutputText](msg-receiveOutputText)
- [speak](msg-speak)


<a id="msg-configureAudio"></a>
#### configureAudio

- `inputSampleRate`: *number*
- `outputSampleRate `: *number*

Tell the application which sample rates to use for input and output. Samples are always 16-bit PCM.

```javascript
{
	id: 'configureAudio',
	inputSampleRate: 8000,
	outputSampleRate: 24000
}
```

<a id="msg-connected"></a>
#### connected

Tell the application that the worker is connected and has configured the service.

```javascript
{
	id: 'connected'
}
```

<a id="msg-disconnected"></a>
#### disconnected

Tell the application that the worker is disconnected from the service.

```javascript
{
	id: 'disconnected'
}
```
<a id="msg-failed"></a>
#### failed

- `error`: *string*

Tell the application that the worker is disconnected from the service because of an error.

```javascript
{
	id: 'failed',
	error: "network error"
}
```

<a id="msg-listen"></a>
#### listen

Tell the application that the worker is receiving audio samples from the service.

```javascript
{
	id: 'listen'
}
```

The application creates an audio output object which reads audio samples from the output buffer.

<a id="msg-receiveAudio"></a>
#### receiveAudio

- `offset`: *number*, the byte offset of audio samples in the output buffer
- `size`: *number*, the byte length of audio samples in the output buffer

Tell the application that the worker received audio samples from the service.

```javascript
{
	id: 'receiveAudio',
	offset: 1024,
	size: 1024
}
```
Audio samples can arrive faster than real time so the application has to queue offsets and sizes.

<a id="msg-receiveFunctionCall"></a>
#### receiveFunctionCall

- `call`: *string*, a unique identifier for the call
- `name`: *string*, the name of one of the functions described in the configuration
- `parameters`: *object* with properties corresponding to the parameters described in the configuration

Tell the application that the worker received a function call from the service.

```javascript
{
	id: 'receiveFunctionCall',
	call: "1234567",
	name: "turn_light_on"
	parameters: {
		light_name: "kitchen light"
	}
}
```

The application uses `name` and `parameters` to do something. When done, the application uses the [**sendFunctionResult** message](#msg-sendFunctionResult) to return a result with the `call` identifier.

<a id="msg-receiveInputText"></a>
#### receiveInputText

- `text`: *string*
- `more`: *boolean*, `true` if the text is incomplete

Tell the application that the worker received a transcription of the audio input.

```javascript
{
	id: 'receiveInputText',
	text: "Hello!",
	more: false
}
```

The application concatenates `text` until `more` is false.

<a id="msg-receiveOutputText"></a>
#### receiveOutputText

- `text`: *string*
- `more`: *boolean*, `true` if the text is incomplete

Tell the application that the worker received a transcription of the audio output.

```javascript
{
	id: 'receiveOutputText',
	text: "How are ",
	more: true
}
```
The application concatenates `text` until `more` is false.

<a id="msg-speak"></a>
#### speak

Tell the application that the worker is waiting for audio samples to send to the service.

```javascript
{
	id: 'speak'
}
```

The application creates an audio input object which writes audio samples in the input buffer. The application uses **sendAudio** messages to send audio samples to the worker.

> The application continuously sends audio samples to the worker until the service begins to respond. At that time, the application will receive a [**listen** message](#msg-listen).


<a id="JSONBase64Parser"></a>

## JSONBase64Parser

### Why

Many AI services have a programming interfaces built on secure WebSocket with JSON payloads. For voice input and output, the JSON payloads contain PCM audio samples encoded as Base64 strings.

That is quite a burden for micro-controllers! Let us focus on voice input with the Moddable SDK...

Here is what the initial approach used:

- [WHATWG WebSocket](https://websockets.spec.whatwg.org) to concat packets into payloads
- `String.fromArrayBuffer` to convert payloads into strings
- `JSON.parse` to convert strings into objects
- `Uint8Array.fromBase64` to convert properties into samples

Samples are in memory multiples times! Base64 encoded in the payloads, strings and properties, as binary data in the samples. Moreover, the app has to wait for the whole payload to be downloaded in order to do multiple conversions and, eventually, to play audio.

Streaming JSON parsers like Mark Wharton's contributed [JSONParser](https://github.com/Moddable-OpenSource/moddable/blob/public/contributed/jsonparser/README.md) help, but Base64 strings would still need to be completely downloaded before being converted.

### How

The `JSONBase64Parser` class implements a streaming JSON parser that also decodes Base64 strings into binary data.

Here is the constructor:

`JSONBase64Parser(target, buffer, step)`

- `target`: the object to callback
- `buffer`: a `SharedArrayBuffer` instance to store binary data
- `alignment`: a number of bytes to align binary data, for instance 2 for 16-bit samples

The parser uses the provided buffer circularly. So the buffer must be large enough to give the app enough time to play audio before the parser rewinds.

Feed the parser with its read method

`read(packet)`

- `packet`: the `ArrayBuffer` instance returned by the `read` method from the `WebSocketClient` instance.

When the parser is about to process a string, it asks its target if the string is Base64 encoded binary data with the `isBase64` callback

`isBase64(result, object, name)`

- `result`: the parser result, which is partial
- `object`: the object being parsed
- `name`: the name of the property being parsed

If the callback returns true, the parser directly converts the string into binary data in the provided buffer.

When the string is processed, or when the packet is processed, the parser tells its target about the decoded binary data with the `onBase64` callback.

`onBase64(offset, size)`

- `offset`: the offset of the binary data in the buffer
- `size`: the size of the binary data in the buffer

Both parameter are in bytes, the size is always a multiple of the provided alignment.

The app does not wait for the entire payload to be downloaded but can play audio for each packet that contains Base64 encoded samples.
