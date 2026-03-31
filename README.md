# ESP32 CYD MJPEG Video Player

A button-controlled MJPEG video player for the ESP32-2432S028R "Cheap Yellow Display" (CYD) board. Play synchronized video and audio files from an SD card with simple button controls.

![CYD Video Player](https://img.shields.io/badge/ESP32-CYD-yellow) ![License](https://img.shields.io/badge/license-CC--BY--NC-blue)

## Features

- Physical Button Controls - Previous/Next buttons for easy navigation
- MJPEG Video Playback - 320x240 resolution at up to 24 FPS
- Synchronized Audio - MP3 audio playback via internal DAC
- SD Card Storage - Play videos directly from microSD card
- Custom Welcome Screen - Personalized startup animation
- Auto-Loop - Automatically plays all videos in sequence
- Simple Wiring - Just two external buttons required

## Hardware Requirements

### ESP32-2432S028R "Cheap Yellow Display" Board

**Specifications:**
- ESP32-WROOM-32 microcontroller
- 2.8" ILI9341 TFT display (320x240)
- XPT2046 resistive touchscreen
- SC8002B audio amplifier
- MicroSD card slot
- Built-in speaker connector

**Where to Buy:**
- [AliExpress](https://www.aliexpress.com) - Search "ESP32-2432S028R"
- [Amazon](https://www.amazon.com) - Search "ESP32 Cheap Yellow Display"
- Typical price: $12-15 USD

### Additional Requirements

- **MicroSD Card** - FAT32 formatted, 32GB or smaller (Class 10 recommended)
- **Speaker** - 8Ω, 0.5-1W (connects to onboard speaker terminals)
- **USB Cable** - Micro-USB for programming and power
- **2x Push Buttons** - Momentary push buttons (normally open)
- **2x 10kΩ Resistors** - For pulldown on GPIO 35 and 22
- **Jumper Wires** - For connecting buttons to CYD

## Software Requirements

### Arduino IDE Setup

1. **Install Arduino IDE** (version 1.8.19 or 2.x)
   - Download from [arduino.cc](https://www.arduino.cc/en/software)

2. **Install ESP32 Board Support**
   - Open Arduino IDE
   - Go to **File → Preferences**
   - Add to "Additional Boards Manager URLs":
     ```
     https://espressif.github.io/arduino-esp32/package_esp32_index.json
     ```
   - Go to **Tools → Board → Boards Manager**
   - Search for "esp32" and install **esp32 by Espressif Systems**
   - Tested with version 2.0.x (recommended)

3. **Select Board**
   - Go to **Tools → Board → ESP32 Arduino**
   - Select **"ESP32-2432S028R CYD"**

### Required Libraries

Install these libraries via **Sketch → Include Library → Manage Libraries**:

| Library | Author | Version | Notes |
|---------|--------|---------|-------|
| **Arduino_GFX** | moononournation | 1.6.4+ | Display driver |
| **ESP8266Audio** | Earle F. Philhower | **2.0.0** | ⚠️ NOT 2.4.1! Audio is broken in newer versions |
| **JPEGDEC** | bitbank2 | Latest | JPEG decoder |

**Critical:** ESP8266Audio version 2.4.1 has broken audio output. Use version 2.0.0 specifically.

### Library Links

- [Arduino_GFX](https://github.com/moononournation/Arduino_GFX)
- [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio) (v2.0.0)
- [JPEGDEC](https://github.com/bitbank2/JPEGDEC)

## Installation

### 1. Clone or Download This Repository

```bash
git clone https://github.com/Casco-Bay-Studios/CYD-Video-Player.git
cd cyd-video-player
```

Or download ZIP and extract.

### 2. Open in Arduino IDE

- Open `MiniVideoPlayer_CYDv10.ino` in Arduino IDE
- Make sure `MjpegClass.h` is in the same folder

### 3. Configure Upload Settings

**Tools Menu Settings:**
- **Board:** ESP32 Dev Module
- **Upload Speed:** 115200 (or 921600 if stable)
- **Flash Frequency:** 80MHz
- **Flash Mode:** QIO
- **Flash Size:** 4MB (32Mb)
- **Partition Scheme:** Default 4MB with spiffs
- **Port:** Select your USB port

### 4. Upload Code

- Connect CYD via USB
- Click **Upload** button
- Wait for "Done uploading" message

## Preparing Video Files

### Video Encoding with FFmpeg

Videos must be converted to MJPEG format with matching MP3 audio files.

#### Install FFmpeg

**Windows:**
```bash
# Download from https://ffmpeg.org/download.html
# Or use: winget install ffmpeg
```

**Mac:**
```bash
brew install ffmpeg
```

**Linux:**
```bash
sudo apt install ffmpeg
```

#### Convert Video Files

**Recommended Settings (Best Quality/Performance Balance):**

```bash
# Video - 320x240, 24fps, good quality
ffmpeg -i input.mp4 -vf "fps=24,scale=320:240:flags=lanczos" -q:v 10 output.mjpeg

# Audio - 22kHz mono, 64kbps
ffmpeg -i input.mp4 -ar 22050 -ac 1 -ab 64k output.mp3
```


**Settings I used to get Widescreen (for movies) and also to counteract delay in audio due to large video buffer:**

```bash
# Video - 320x240, 24fps, WIDESCREEN
ffmpeg -i input.mp4 -vf "fps=24,scale=320:-1,pad=320:240:(ow-iw)/2:(oh-ih)/2" -q:v 8 output.mjpeg

# Audio - 22kHz mono, 64kbps
ffmpeg -i input.mp4 -ss 0.2 -ar 44100 -ac 1 -q:a 9 output.mp3
```



**For Smoother Playback (Lower Quality):**

```bash
# Video - Lower quality for faster decoding
ffmpeg -i input.mp4 -vf "fps=24,scale=320:240:flags=lanczos" -q:v 12 output.mjpeg

# Audio - Lower sample rate
ffmpeg -i input.mp4 -ar 16000 -ac 1 -ab 48k output.mp3
```

**For Lower Frame Rate (Even Smoother):**

```bash
# Video - 15fps instead of 24fps
ffmpeg -i input.mp4 -vf "fps=15,scale=320:240:flags=lanczos" -q:v 12 output.mjpeg

# Audio - Same as above
ffmpeg -i input.mp4 -ar 16000 -ac 1 -ab 48k output.mp3
```

And change in code:
```cpp
#define FPS 15  // Line 21
```

#### Quality Settings Explained

**Video Quality (`-q:v`):**
- Scale: 2-31 (lower = better quality, larger files)
- `8` = High quality (may stutter)
- `10` = Good quality (recommended)
- `12` = Medium quality (smoother playback)
- `15` = Lower quality (very smooth)

**Audio Sample Rate (`-ar`):**
- `44100` = CD quality (may stutter)
- `22050` = Good quality (recommended)
- `16000` = Acceptable quality (smoothest)

### File Naming Convention

Each video must have a matching audio file with the **exact same base name**:

```
video1.mjpeg + video1.mp3
episode2.mjpeg + episode2.mp3
cartoon.mjpeg + cartoon.mp3
```

### Prepare SD Card

1. **Format SD Card**
   - Use [SD Card Formatter](https://www.sdcard.org/downloads/formatter/)
   - Format as **FAT32**
   - 32GB or smaller cards work best

2. **Copy Files**
   - Copy all `.mjpeg` and `.mp3` file pairs to the **root directory** of SD card
   - Do NOT put them in folders
   - The player will find all matching pairs automatically

3. **Insert SD Card**
   - Insert into CYD's SD card slot
   - Make sure it clicks in place

## Button Wiring

### Wiring Diagram

Connect two push buttons to the CYD's extended GPIO connector:

**Button Circuit (for EACH button):**

```
 GPIO Pin
(22 or 35)            GND            
     |                |
     |                |
[Push Button (with 4 pins on it]
     |                |
     |                |
10kΩ Resistor        [no connection]    
     |
    3.3V
```

**Connections:**

| Button | GPIO | Function | Wiring |
|--------|------|----------|--------|
| NEXT | GPIO 22 | Skip to next video | Button to 3.3V + 10kΩ resistor + GPIO 22, other side of button to GND |
| PREV | GPIO 35 | Go to previous video | Button to 3.3V to 10kΩ resistor + GPIO 35, other side of button to GND |

**Important Notes:**
- GPIO 35 is INPUT-ONLY (no internal pullup available)
- 10kΩ pullup resistors are **required** for both buttons
- Buttons should be normally open (NO) momentary switches, 4 pin type
- When button is pressed, it connects GPIO to GND

### CYD Connector Pinout

The extended GPIO connector on the CYD (4-pin connector) has:
```
Pin 1: GPIO 21
Pin 2: GPIO 22  ← NEXT button
Pin 3: GPIO 35  ← PREV button
Pin 4: GND
```

You'll need to tap 3.3V from another location on the board.

## Usage

### Button Controls

- **NEXT Button (GPIO 22)** - Skip to next video
- **PREV Button (GPIO 35)** - Go to previous video

**During Playback:**
- Press NEXT to immediately skip to the next video
- Press PREV to go back to the previous video
- Videos auto-advance when they finish playing

### Startup Sequence

1. Power on the CYD
2. Welcome screen displays 
3. SD card is detected and file count shown
4. First video name displays for 4 seconds
5. Video begins playing automatically

### Auto-Play Mode

- Videos play continuously in sequence
- After last video, loops back to first video
- Use double-tap to skip videos manually

### Serial Monitor (Debugging)

Open Serial Monitor at **115200 baud** to see:
- SD card initialization status
- Number of video files found
- Currently playing file
- Touch coordinates (for calibration)
- Frame skip warnings
- Error messages

## Note on Audio
 - You can plug a speaker directly into the JST connection labeled SPEAK, but it is loud and kind of distorted.
 - I made a simple circuit to control volume, and also added an external audio jack, which cuts off the speaker audio when headphones are plugged in.
 - 





## Troubleshooting

### SD Card Not Detected

**Error:** "ERROR: SD card mount failed!"

**Solutions:**
- Format card as FAT32 (not exFAT)
- Use 32GB or smaller card
- Try a different/slower speed card
- Make sure card is fully inserted (clicks)
- Test card in computer first

### No Audio / Audio Stuttering

**Solutions:**
- Check speaker is connected to SPEAK terminals
- Adjust gain in code: `out->SetGain(0.7);` (try 0.5-1.0)
- Use lower sample rate: `-ar 16000`
- Use lower audio bitrate: `-ab 48k`
- Make sure using ESP8266Audio v2.0.0

### Video Choppy / Skipping Frames

**Solutions:**
- Lower video quality: `-q:v 12` or `-q:v 15`
- Reduce resolution: `scale=280:210` or `scale=240:180`
- Lower frame rate: `fps=15` (and update code)
-  Use Class 10 or UHS-1 SD card
- Reduce audio sample rate
- Enable multitasking (already enabled in code)

### Files Not Playing

**Error:** "Failed to open audio/video file"

**Solutions:**
- Files must be in root directory (not in folders)
- File pairs must have exact same base name
- Check file extensions: `.mjpeg` and `.mp3` (lowercase)
- No spaces or special characters in filenames
- Re-encode files with correct FFmpeg commands

### Display Issues

**White/Blank Screen:**
-  Check `true/false` parameter in display initialization (line 65)
-  Try different rotation values (0-3)
-  Verify backlight is on (GPIO 21 HIGH)
**Wrong Colors:**
-  Change IPS parameter: `false` ↔ `true` (line 65)

### Buttons Not Working

**Solutions:**
-  Check wiring: Button should connect GPIO pin to GND, but needs a resistor connected to 3.3V on the same side of the buttons as the GPIO pin connection
-  Verify 10kΩ resistor from button to 3.3V
-  Test buttons with multimeter
-  Check Serial Monitor for "Button Pressed!" messages
-  GPIO 35 requires external pulldown (no internal pullup)
-  Make sure using momentary (not latching) switches

### Compilation Errors

**"Library not found":**
- Install missing library via Library Manager
- Restart Arduino IDE

**"Multiple libraries found":**
- This is just a warning, usually safe to ignore
- Or uninstall duplicate libraries

**Version compatibility issues:**
- Use ESP32 board package v2.0.x
- Use ESP8266Audio v2.0.0 specifically

## Customization

### Change Welcome Screen Text

Edit the `typewriterFX()` function (around line 466):

```cpp
void typewriterFX() {
  gfx->setTextColor(CYAN, BLACK);
  gfx->setTextSize(2);
  gfx->setCursor(60, 60);
  String message = "Your Text Here";  // ← Change this
  
  for (int i = 0; i < message.length(); i++) {
    gfx->print(message[i]);
    delay(75);
  }
  
  // ... add more text sections
}
```

### Adjust Button Debounce

If buttons are too sensitive or triggering multiple times:

```cpp
unsigned long debounceDelay = 200;  // Increase for slower response (line ~90)
```

### Change Button Pins

To use different GPIO pins, update these lines (around line 54-55):

```cpp
int BTN_NEXT = 22;  // Change to your preferred GPIO
int BTN_PREV = 35;  // Change to your preferred GPIO
```

**Note:** Avoid GPIOs used by display, SD card, or audio (see pinout table below).

### Change Video Colors

Edit color definitions (around line 29):

```cpp
#define BLACK 0x0000 
#define CYAN 0x07FF
#define PINK 0xF81F
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define WHITE 0xFFFF
#define YELLOW 0xFFE0
```

### Adjust Volume

In `setup()` function (around line 160):

```cpp
out->SetGain(0.7);  // Range: 0.0 (silent) to 1.0 (full volume)
```

### Calibrate Touchscreen

If left/right detection is incorrect, adjust mapping in `handleTouch()`:

```cpp
int x = map(p.x, 200, 3700, 0, 320);  // Adjust min/max values
int y = map(p.y, 240, 3800, 0, 240);  // Adjust min/max values
```

To find correct values:
1. Open Serial Monitor
2. Tap corners of screen
3. Note raw coordinates printed
4. Update min (top-left) and max (bottom-right) values

### Physical Buttons (Optional)

Uncomment in `setup()` to enable physical buttons:

```cpp
pinMode(BTN_NEXT, INPUT);
pinMode(BTN_PREV, INPUT);
attachInterrupt(BTN_NEXT, incrFileNo, RISING);
attachInterrupt(BTN_PREV, decrFileNo, RISING);
```

**Note:** GPIO 34 and 35 are input-only and need external 10kΩ pulldown resistors.

## Technical Specifications

### CYD Pin Usage

| Function | GPIO | Notes |
|----------|------|-------|
| **Display** |
| LCD MOSI | 13 | Shared SPI |
| LCD MISO | 12 | Shared SPI |
| LCD SCK | 14 | Shared SPI |
| LCD CS | 15 | Display chip select |
| LCD DC | 2 | Data/Command |
| LCD BL | 21 | Backlight (also used by audio!) |
| **Touchscreen** |
| Touch CS | 33 | Touch chip select |
| Touch IRQ | 36 | Touch interrupt |
| Touch MOSI | 13 | Shared with display |
| Touch MISO | 12 | Shared with display |
| Touch SCK | 14 | Shared with display |
| **SD Card** |
| SD CS | 5 | SD chip select |
| SD MOSI | 23 | VSPI bus |
| SD MISO | 19 | VSPI bus |
| SD SCK | 18 | VSPI bus |
| **Audio** |
| Speaker | 26 | DAC2 output to SC8002B amp |
| **Optional Buttons** |
| BTN_NEXT | 35 | Input only (needs pulldown) |
| BTN_PREV | 34 | Input only (needs pulldown) |

### Performance

- **Max Video Resolution:** 320x240
- **Recommended FPS:** 15-24
- **Audio Sample Rate:** 16kHz - 22kHz recommended
- **SD Card Speed:** Class 10 or UHS-1 recommended
- **Typical Video File Size:** 5-15 MB per minute

### Memory Usage

- **MJPEG Buffer:** ~19KB (adjustable)
- **ESP32 Flash:** ~800KB program
- **ESP32 RAM:** ~150KB during playback

## Credits & License

### Original Project

- **Original Author:** Alex - Super Make Something
- **Original Project:** [Open Source Mini Video Player](https://youtu.be/67RFm2RMjC4)
- **Original License:** Creative Commons - Attribution - Non-Commercial

### CYD Adaptation

- **CYD Port:** 2026
- **Touchscreen Implementation:** Added double-tap gesture controls
- **Audio Fix:** Updated for SC8002B amplifier (DAC mode)
- **Display Configuration:** Optimized for ILI9341

### Special Thanks

- **bepaald** - Bug fixes and code cleanup on original project
- **Brian Lough** - CYD community resources and examples
- **Bodmer** - TFT_eSPI library (used as reference)
- **Paul Stoffregen** - XPT2046 touchscreen library
- **moononournation** - Arduino_GFX library

### Contributing

This project is open to community contributions! Some ideas:

- [ ] Add pause/play functionality
- [ ] Volume control via touchscreen slider
- [ ] Playlist/shuffle modes
- [ ] Battery monitoring for portable use
- [ ] WiFi-based video streaming
- [ ] Touch calibration routine
- [ ] Progress bar display

Feel free to fork and submit pull requests!

### License

Creative Commons - Attribution - Non-Commercial (CC BY-NC)

**You are free to:**
- ✅ Share and use this code
- ✅ Modify and adapt for personal use
- ✅ Learn from and teach with this code

**Under these conditions:**
- ⚠️ Attribution required
- ⚠️ Non-commercial use only
- ⚠️ Share modifications under same license

For commercial use, please contact the original author.

More information: [Creative Commons BY-NC License](http://creativecommons.org/licenses/by-nc/3.0/)

## Support & Community

### Getting Help

1. **Check Troubleshooting section** above
2. **Search existing GitHub Issues**
3. **Open a new Issue** with:
   - CYD board version
   - Library versions
   - Error messages from Serial Monitor
   - What you've already tried

### Resources

- [ESP32 CYD Community (GitHub)](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
- [Random Nerd Tutorials - CYD Guides](https://randomnerdtutorials.com/?s=CYD)
- [LVGL for CYD](https://randomnerdtutorials.com/lvgl-cheap-yellow-display-esp32-2432s028r/)
- [ESP32 Arduino Documentation](https://docs.espressif.com/projects/arduino-esp32/)

### Show Your Projects!

Built something cool with this? Share it!
- Post in GitHub Discussions
- Tag on social media
- Contribute improvements back

---

**Enjoy your CYD video player!** 🎬🎵

If this project helped you, please ⭐ star the repository!
