{
  "include": [
    "$(MODDABLE)/examples/manifest_base.json",
    "$(MODDABLE)/modules/io/manifest.json",
    "$(MODULES)/data/text/decoder/manifest.json"
  ],
  "modules": {
    "*": "./cluster"
  }
}
