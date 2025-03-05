# Conversational AI
Copyright 2025 Moddable Tech, Inc.<BR>
Updated March 5, 2025

## Introduction
The Conversational AI application provides real-time voice chats with over a dozen AI assistants from multiple AI services.

The application is an example of what can be built using Moddable Realtime AI architecture. To learn more about the architecture, check out the [documentation](./../../modules/network/services/chatAudioIO/readme.md). This application and architecture were first introduced with Moddable SDK 5.5.0. Check out the [release notes](https://github.com/Moddable-OpenSource/moddable/releases/tag/untagged-aabea57c6b0eaf9cf226) for additional information and the motivation.

## Configuring
You'll need to set-up a little bit of configuration the first time you use Conversational AI.

### AI Service Keys
You will need your own keys for the AI services used by the Conversational AL. If you don't already have them, you can sign up with [OpenAI](https://platform.openai.com/api-keys) and [Google Gemini](https://ai.google.dev/gemini-api/docs/api-key).

It is recommended that you specify the API keys on the command line, though there is also a place to put them in the `config` section of [manifest.json](./manifest.json). On the command line, you can specify the keys directly:

```
mcconfig -d -m -p esp32/moddable_six_cdc openAIKey="xyzzy" geminiAPIKey="abcde"
```

You can also add the keys to your environment, so that they can be accessed by name.

```
export openAIKey="xyzzy"
export geminiAPIKey ="abcde"
mcconfig -d -m -p esp32/moddable_six_cdc openAIKey=$openAIKey geminiAPIKey=$geminiAPIKey
```

### Wi-Fi
The application does not currently have a user interface to connect to Wi-Fi. You will need to set that from the command line when building, using the usual `ssid` and `password` variables:

```
mcconfig -d -m -p esp32/moddable_six_cdc ssid="my wifi" password="secret"
```

## Microphone
The application is generally very straightforward to use. There is one nice feature that you might not notice immediately. The microphone icon is shown only when the AI service is listening. If you don't see the microphone on-screen, the service cannot hear what you say. This allows the application to support embedded hardware, like the ESP32-S3, that does not support PDM audio input and output simultaneously. If you tap the microphone icon, it mutes the microphone while maintaining the connection to the service. This is great for privacy and demos. Tap the icon again to unmute it.

## Navigating the Code
The AI assistants are defined in [`model.json`](./model.json). You can edit this file to enhance the predefined agents and add your own.

Conversational AI is built using the Piu user interface framework. The [`views`](./views) directory contains the various screens. To learn more about Piu, see its [documentation](./../../documentation/piu/piu.md).

The sophisticated mobile-like scrolling on the [assistant chooser view](./views/Personas.js) is implemented entirely in JavaScript in [`ScrollBehaviors.js`](./ScrollBehaviors.js).

The bitmap fonts are created as part of the build process from TrueType fonts in the [`fonts`](./fonts) directory. See the [documentation](./../../documentation/commodetto/Creating%20fonts%20for%20Moddable%20applications.md#fontbm) on using `fontbm` for details on the process and using your own fonts.
