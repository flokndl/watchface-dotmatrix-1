# Dotmatrix Watchface

A retro-inspired dotmatrix-style watchface for Pebble smartwatches with smooth animated digit transitions.

## Features

- **Dotmatrix Display**: Classic dotmatrix-style numeric display showing hours and minutes
- **Smooth Animations**: Each digit transitions smoothly when the time changes with a sliding animation effect
- **Smart Hour Display**: Automatically hides the leading zero for single-digit hours (e.g., shows "9:30" instead of "09:30")
- **12/24 Hour Format**: Automatically adapts to your watch's time format setting
- **Multi-Platform Support**: Works on all Pebble platforms (aplite, basalt, chalk, diorite)


## Installation

### Option 1: Install via Pebble Mobile App (Recommended)

1. Download the latest `.pbw` file from the [Releases](../../releases) page
2. Open the Pebble mobile app on your phone
3. Go to **Menu** → **Settings** → **Watchfaces**
4. Tap **Add Watchface** → **From File**
5. Select the downloaded `.pbw` file
6. The watchface will be installed on your Pebble watch

### Option 2: Build from Source

#### Prerequisites

1. Install the Pebble SDK from [Rebble](https://developer.rebble.io/)
2. Ensure you have Python 2.7 installed
3. Set up your development environment according to the [Pebble SDK documentation](https://developer.rebble.io/developer.pebble.com/sdk/index.html)

#### Build Steps

1. Clone this repository:
   ```bash
   git clone https://github.com/flokndl/watchface-dotmatrix-1.git
   cd watchface-dotmatrix-1
   ```

2. Build the watchface:
   ```bash
   pebble build
   ```

3. Install on your Pebble watch:
   ```bash
   pebble install --phone <PHONE_IP>
   ```
   
   Or manually install the `.pbw` file from the `build/` directory using the Pebble mobile app.

## Project Structure

```
.
├── src/
│   └── main.c          # Main watchface code with animation logic
├── resources/          # Bitmap images for digits (0-9)
│   └── images/
│       └── num/        # Individual digit images
├── package.json        # npm package configuration
├── appinfo.json        # App metadata and resource definitions
├── wscript            # Build configuration
└── pebble-js-app.js   # JavaScript side (optional, for configuration)
```

## How It Works

The watchface uses a sophisticated animation system:

- Each digit position has its own mask layer (for clipping) and wrapper layer (for sliding animation)
- When a digit changes, the old and new digit bitmaps are placed side-by-side
- A smooth sliding animation transitions from the old digit to the new one
- The hour tens digit (leftmost) is automatically hidden when displaying single-digit hours
- Leading digits (hour tens and minute tens) have a slight delay in their animations for a cascading effect

## Development

To customize the watchface:

- Edit `src/main.c` to modify the display logic or animations
- Replace images in `resources/images/num/` to change the digit appearance
- Adjust animation timing by modifying `TRANSITION_DURATION_MS` and `DELAY_MS` constants
- Configure app settings in `appinfo.json`

## Compatibility

- **Pebble SDK**: Version 3
- **Platforms**: aplite, basalt, chalk, diorite
- **Watch Models**: All Pebble watches (Classic, Time, Time Steel, Time Round, Pebble 2)

## License

This work is licensed under a [Creative Commons Attribution-ShareAlike 4.0 International License](https://creativecommons.org/licenses/by-sa/4.0/).

You are free to:
- **Share** — copy and redistribute the material in any medium or format
- **Adapt** — remix, transform, and build upon the material for any purpose

Under the following terms:
- **Attribution** — You must give appropriate credit, provide a link to the license, and indicate if changes were made
- **ShareAlike** — If you remix, transform, or build upon the material, you must distribute your contributions under the same license as the original

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Acknowledgments

Built for the Pebble community and the [Rebble](https://rebble.io/) project.
