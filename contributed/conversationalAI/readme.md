# Conversational AI
Copyright 2025 Moddable Tech, Inc.<BR>
Updated March 5, 2025

The Conversational AI application provides real-time voice chats with over a dozen AI assistants from multiple AI services.

The application is an example of what can be built using Moddable Realtime AI architecture. To learn more about the architecture, check out the [documentation](./../../modules/network/services/chatAudioIO/readme.md).

The AI assistants are defined in [`model.json`](./model.json). You can edit this file to enhance the predefined agents and add your own.

Conversational AI is built using the Piu user interface framework. The [`views`](./views) directory contains the various screens. To learn more about Piu, see its [documentation](./../../documentation/piu/piu.md).

The sophisticated mobile-like scrolling on the [assistant chooser view](./views/Personas.js) is implemented entirely in JavaScript in [`ScrollBehaviors.js`](./ScrollBehaviors.js).

The bitmap fonts are created as part of the build process from TrueType fonts in the [`fonts`](./fonts) directory. See the [documentation](./../../documentation/commodetto/Creating%20fonts%20for%20Moddable%20applications.md) on using `fontbm` for details on the process and using your own fonts.
