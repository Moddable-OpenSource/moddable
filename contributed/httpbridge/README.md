# Moddable http bridge example

This example shows how to use the http bridge components.

It starts a bi-directional websocket between the Moddable server and a browser. If you start another browser instance, changes on one browser reflect in the other.

### build the zip file
```
cd site
npm install
npm run build
```

### build moddable
From the site folder:
```
npm run mcconfig
```

This will build the `site.zip` and launch the simulator

open browser `http://localhost`

### front end development
```
`npm run dev`
or
`wmr`

open browser `http://localhost:8080`

Edit any ts or css file, and on save the browser will auto-update with changes, and the `websocket` is connected to the simulator Modable server

