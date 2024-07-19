# MW2MID
## Martin Walker (GB/GG) to MIDI converter

This tool converts music from Game Boy and Game Gear games using Martin Walker's sound engine to MIDI format.

It works with ROM images. To use it, you must specify the name of the ROM followed by the number of the bank containing the sound data (in hex).
For games that contain multiple banks of music (i.e. most of the Pinball games), you must run the program multiple times specifying where each different bank is located. However, in order to prevent files from being overwritten, the MIDI files from the previous bank must either be moved to a separate folder or renamed.

Examples:
* MW2MID "Earthworm Jim (E) [!].gb" 4 gb
* MW2MID "Megaman (U) [!].gg" C gg
* MW2MID "Pinball Fantasies (U).gb" 8 gb
* MW2MID "Pinball Fantasies (U).gb" A gb

This tool was based on my own reverse-engineering of the sound engine, with little disassembly involved. The sequence format happened to be practically identical between Game Boy and Game Gear, so it supports both systems.
Despite the Game Boy and Game Gear having four audio channels, this driver only supports 3 "voices" at a time, with channel 4 being called indirectly, which alludes to that this appears to be based on Martin Walker's Commodore 64 sound engine (the C64 only has 3 channels of audio). Speedball 2: Brutal Deluxe uses an early version of the driver which has many significant differences between the final version. Support for that game was also added, but MIDI conversion isn't quite perfect yet, as it has desync issues, since I still don't understand the sequence format quite as much as the later driver used in all other games. This version of the driver is also probably very close to the original C64 driver, and the title music closely resembles the C64 version of the same game.
Pitch bend/portamento is partially implemented; the events are included in conversion, but the pitch doesn't actually change, as it is apparently difficult to get that working properly in MIDI.

Supported games:

Game Boy:
* Arcade Classic No. 4: Defender & Joust
* Chuck Rock
* Cliffhanger
* Cutthroat Island
* Earthworm Jim
* Foreman for Real
* Mortal Kombat 3
* Pinball Deluxe
* Pinball Dreams
* Pinball Fantasies
* Pinball Mania
* Speedball 2: Brutal Deluxe
* Street Racer
* Tarzan (GB)
* Waterworld
* Yogi Bear's Gold Rush

Game Gear
 * Arena: Maze of Death
 * Cutthroat Island
 * Earthworm Jim
 * Foreman for Real
 * Mega Man
 * Mortal Kombat 3
 * Pinball Dreams
 * Sports Trivia: Championship Edition
 * Tarzan
 * VR Troopers

## To do:
  * Proper pitch bend/portamento support
  * Fixes for Speedball 2
  * Support for other versions of the sound engine, especially SNES and MegaDrive as well as computers
  * GBS (and SGC?) file support
