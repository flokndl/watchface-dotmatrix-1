# How to View Pebble Watchface Logs

## Method 1: Using Pebble CLI (Recommended)

1. **Make sure your watch is connected** and the watchface is installed
2. **Open a terminal** in your project directory
3. **Run the logs command:**
   ```bash
   pebble logs
   ```
   
   This will show real-time logs from your watch and the JavaScript side.

4. **To filter for JavaScript logs only**, you can use:
   ```bash
   pebble logs | grep "pebble-js-app"
   ```

## Method 2: Using Pebble Emulator

If you're testing on the emulator:

1. **Start the emulator:**
   ```bash
   pebble install --emulator basalt
   ```
   (Replace `basalt` with your target platform: `aplite`, `basalt`, `chalk`, or `diorite`)

2. **View logs:**
   ```bash
   pebble logs
   ```

## Method 3: Mobile App Logs

### iOS:
1. Connect your iPhone to a Mac
2. Open **Console.app** (Applications > Utilities > Console)
3. Select your iPhone from the device list
4. Filter for "Pebble" or "pebble-js-app"

### Android:
1. Enable **Developer Options** on your Android device
2. Enable **USB Debugging**
3. Connect to your computer
4. Use `adb logcat`:
   ```bash
   adb logcat | grep -i pebble
   ```

## What to Look For

When you click the settings button, you should see logs like:
- `pebble-js-app.js: Starting to load...`
- `pebble-js-app.js: Attempting to require pebble-clay...`
- `pebble-js-app.js: Clay module loaded successfully`
- `pebble-js-app.js: Clay initialized successfully`
- `pebble-js-app.js: showConfiguration event fired`

If you see any `ERROR` messages, those will tell you what's wrong.

## Quick Test

1. Run `pebble logs` in one terminal
2. Try to open the configuration page from the Pebble app
3. Watch the terminal for log messages

