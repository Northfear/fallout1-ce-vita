# Fallout Community Edition port for PS Vita

## Install
Download fallout-ce.vpk file and install it to your PS Vita.

Copy ```MASTER.DAT```, ```CRITTER.DAT``` and ```DATA``` folder from the installed Fallout game folder into to ```ux0:data/fallout/```. Copy ```fallout.cfg``` too, if you're using non-english Fallout version (or make sure that ```language``` setting is properly set in it (```language=german```, ```language=french```, etc)).

```diff
! ⚠️ Please note that the copied file names should be upper case.
```

## Building

### Prerequisites
- VitaSDK
- SDL2

### Build
```
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake -DCMAKE_BUILD_TYPE=None
make
```

## Port info

### Controls

- Left analog stick - Cursor movement
- Right analog stick - Map scrolling
- × - Left mouse button
- ○ - Right mouse button
- □ - Skill list/selection
- △ - Inventory
- D-Pad Up - Character screen
- D-Pad Down - Pip-Boy
- D-Pad Left - Start combat
- D-Pad Right - End turn
- L1 - Toggle active item
- R1 (hold) - Cursor movement speedup
- SELECT - Esc
- START - On screen keyboard
- ○ + L1 - Quick save
- ○ + R1 - Quick load

### Touchpad controls

You can change the control mode of front or rear touchpad by editing ```ux0:data/fallout/f1_res.ini``` and changing ```FRONT_TOUCH_MODE``` and ```REAR_TOUCH_MODE``` parameters.

Following touchpad modes are supported:

```0``` - Touchpad disabled

```1``` - Direct control (front touchpad only)

```2``` - Trackpad control

In trackpad control mode you can use double tap to "click" left mouse button in the current cursor position and triple tap to "click" right mouse button.

### Other

You can change the game resolution by editing ```ux0:data/fallout/f1_res.ini``` file and modifying ```SCR_WIDTH``` and ```SCR_HEIGHT``` parameters. Recommended resolutions are ```640x480```, ```848x480``` and ```960x544```.

Cursor speed can be changed with ```MOUSE SENSITIVITY``` slider in game preferences.

Fallout CE is still in early stages of development, so all kinds of bugs and crashes are to be expected.

# Fallout Community Edition

Fallout Community Edition is a fully working re-implementation of Fallout, with the same original gameplay, engine bugfixes, and some quality of life improvements, that works (mostly) hassle-free on multiple platforms.

## Installation

You must own the game to play. Purchase your copy on [GOG](https://www.gog.com/game/fallout) or [Steam](https://store.steampowered.com/app/38400). Download latest release or build from source.

### Windows

Download and copy `fallout-ce.exe` to your `Fallout` folder. It serves as a drop-in replacement for `falloutw.exe`.

### Linux

- Use Windows installation as a base - it contains data assets needed to play. Copy `Fallout` folder somewhere, for example `/home/john/Desktop/Fallout`.

- Download and copy `fallout-ce` to this folder.

- Install [SDL2](https://libsdl.org/download-2.0.php):

```console
$ sudo apt install libsdl2-2.0-0
```

- Run `./fallout-ce`.

### macOS

> **NOTE**: macOS 10.11 (El Capitan) or higher is required. Runs natively on Intel-based Macs and Apple Silicon.

- Use Windows installation as a base - it contains data assets needed to play. Copy `Fallout` folder somewhere, for example `/Applications/Fallout`.

- Alternatively you can use Fallout from MacPlay/The Omni Group as a base - you need to extract game assets from the original bundle. Mount CD/DMG, right click `Fallout` -> `Show Package Contents`, navigate to `Contents/Resources`. Copy `GameData` folder somewhere, for example `/Applications/Fallout`.

- Download and copy `fallout-ce.app` to this folder.

- Run `fallout-ce.app`.

### Android

> **NOTE**: Fallout was designed with mouse in mind. There are many controls that require precise cursor positioning, which is not possible with fingers. When playing on Android you'll use fingers to move mouse cursor, not a character, or a map. Double tap to "click" left mouse button in the current cursor position, triple tap to "click" right mouse button. It might feel awkward at first, but it's super handy - you can play with just a thumb. This is not set in stone and might change in the future.

- Use Windows installation as a base - it contains data assets needed to play. Copy `Fallout` folder to your device, for example to `Downloads`. You need `master.dat`, `critter.dat`, and `data` folder.

- Download `fallout-ce.apk` and copy it to your device. Open it with file explorer, follow instructions (install from unknown source).

- When you run the game for the first time it will immediately present file picker. Select the folder from the first step. Wait until this data is copied. A loading dialog will appear, just wait for about 30 seconds. The game will start automatically.

### iOS

> **NOTE**: See Android note on controls.

- Download `fallout-ce.ipa`. Use sideloading applications ([AltStore](https://altstore.io/) or [Sideloadly](https://sideloadly.io/)) to install it to your device. Alternatively you can always build from source with your own signing certificate.

- Run the game once. You'll see error message saying "Couldn't find/load text fonts". This step is needed for iOS to expose the game via File Sharing feature.

- Use Finder (macOS Catalina and later) or iTunes (Windows and macOS Mojave or earlier) to copy `master.dat`, `critter.dat`, and `data` folder to "Fallout" app ([how-to](https://support.apple.com/HT210598)).

## Contributing

Here is a couple of current goals. Open up an issue if you have suggestion or feature request.

- **Update to v1.2**. This project is based on Reference Edition which implements v1.1 released in November 1997. There is a newer v1.2 released in March 1998 which at least contains important multilingual support.

- **Backport some Fallout 2 features**. Fallout 2 (with some Sfall additions) added many great improvements and quality of life enhancements to the original Fallout engine. Many deserve to be backported to Fallout 1. Keep in mind this is a different game, with slightly different gameplay balance (which is a fragile thing on its own).

## License

The source code is this repository is available under the [Sustainable Use License](LICENSE.md).
