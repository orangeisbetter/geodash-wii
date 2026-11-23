# Geometry Dash Wii

This is a clone of Geometry Dash for the Wii. The goal for this clone is to remain as close to the original as is reasonable, while retaining optimal performance on the Wii.

## Setting Up

This game does not include any assets. All game assets must be provided and placed in the appropriate directory.

Read textures/README.txt for information on textures.

### SD Card Setup

**PLEASE NOTE: THIS PROCESS WILL TAKE SOME TIME.**

I wish I could provide you with a working game directory, but unfortunately, that violates copyright law. You will need to follow this process if you wish to legally play this clone.

Begin by copying the `sd/gdwii` directory to the root of your SD card (the resulting path should look like `sd:/gdwii/assets/...`). This will provide the basis for the runtime game file structure.

You will need several files from Geometry Dash. All further paths in this section will be relative to `gdwii/assets/`, unless otherwise stated.

Feel free to explore this structure and the associated files in detail to figure out how it works.

#### Levels

Provide the levels you wish to use into the `levels/` directory. The levels must be provided as plaintext files, in the format used by Geometry Dash. For instance, copy files `1.txt` through `13.txt` into this directory.

You can copy more files than this as well. You can even copy and use editor levels as well, but the process for converting those levels is not provided here. For nerdy folks, the format of these files is a Base64-encoded zlib-compressed level string.

#### Level info

The file `level-info.xml` contains the metadata for each level. This information includes the level name and song name.

The level info XML schema is provided as `level-info.xsd`.

#### Mappacks

The mappack xml schema is provided as `mappacks/mappack.xsd` for reference.

Mappacks contain a list of level IDs that are part of that mappack. For now, only the `main` mappack is used (`mappacks/main.xml`).

#### Music

Music files must be placed into the `music/` directory. The names of the files must match what is defined in `level-info.xml`.

#### Textures

Textures in this clone are compiled into `.tpl` files, which is the native format used by several Wii games and applications. You must provide the `.png` files from Geometry Dash for this.

The provided png files will be converted into `.tpl` files during the build process, and those files must be placed directly into the assets directory. See the *Building* section for more information.

The textures required by the game are:

- `background.tpl`
- `bigFont-hd.tpl`
- `GJ_GameSheet.tpl`
- `ground.tpl`
- `squares.tpl`

Instructions and specific textures required are located in `textures/README.txt` (in project root).

#### Texture sheet info

You must provide the `*.plist` files from the actual game for each texture used. The following files are required, and can be found in the `Resources` directory of the Geometry Dash installation.

- `GJ_GameSheet.plist`
- `GJ_GameSheet02.plist`
- `GJ_GameSheet03.plist`
- `GJ_GameSheet04.plist`
- `GJ_GameSheetGlow.plist`
- `GJ_GameSheetIcons.plist`
- `GJ_LaunchSheet.plist`

#### Font

The file `bigFont-hd.fnt` must be placed into the assets directory. This can be found in Geometry Dash's `Resource` directory.

## Building

This project uses DevKitPro and libogc for building. Be sure to install these before continuing. In addition, the following devkitpro packages must be installed:

- `ppc-zlib`

cJSON is vendored in `source/vendor` and built as part of the project; no devkitPro portlib package is needed for it.

With all the game textures placed in the `textures` directory, type `make` and everything should be built. The game should be output as a `boot.dol` file in the project root.

## Important Note

This project is in early stages, so there is minimal documentation and resources.
