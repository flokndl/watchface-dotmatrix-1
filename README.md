# Pebble 2 Duo Watchface

A boilerplate watchface project for Pebble 2 Duo.

## Prerequisites

1. Install the Pebble SDK from [Rebble](https://developer.rebble.io/)
2. Ensure you have Python 2.7 installed
3. Set up your development environment according to the [Pebble SDK documentation](https://developer.rebble.io/developer.pebble.com/sdk/index.html)

## Project Structure

```
.
├── src/
│   └── main.c          # Main watchface code
├── resources/          # Images, fonts, etc. (optional)
├── package.json        # npm package configuration
├── appinfo.json        # App metadata
├── wscript            # Build configuration
└── pebble-js-app.js   # JavaScript side (optional)
```

## Building

1. Build the watchface:
   ```bash
   pebble build
   ```

2. Install on your Pebble watch:
   ```bash
   pebble install --phone <PHONE_IP>
   ```
   Or use the Pebble mobile app to install the `.pbw` file from the `build/` directory.

## Development

- Edit `src/main.c` to customize the watchface
- Add resources (images, fonts) to the `resources/` directory
- Configure app settings in `appinfo.json`
- Update `package.json` with your project details

## Features

This boilerplate includes:
- Basic time display (12/24 hour format)
- Date display
- Support for all Pebble platforms (aplite, basalt, chalk, diorite)
- Configurable watchface structure

## License

MIT

