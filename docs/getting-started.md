---
title: Getting started
nav_order: 1
---

# Getting started

{: .note }
Photo Cam is not released yet. It needs a [DAOLib](https://github.com/Whocares66-Dev/DAOLib) newer than 0.2.0, with the free view, the held cursor and the screenshot hook.

## Install

1. Install DAOLib.
2. Extract Photo Cam's zip into the game's folder, so that this is there:

   ```
   bin_ship\daolib\plugins\photocam\photocam.dll
   bin_ship\daolib\plugins\photocam\photocam.swf
   bin_ship\daolib\plugins\photocam\photocam_navbar.swf
   ```

3. Start the game and load a save.

## First shot

1. Press <kbd>F8</kbd>, or click the camera in the top bar. The world pauses and the interface hides.
2. Hold the right mouse button and move the mouse to look. Fly with <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd>, <kbd>Q</kbd> and <kbd>E</kbd>.
3. Click **Capture** in the panel, or press the game's screenshot key. The picture goes where the game's always do: `Documents\BioWare\Dragon Age\Screenshots\User`.
4. Press <kbd>F8</kbd> or <kbd>Esc</kbd>, or click **Exit**. The camera returns to the party.

Photo Cam starts from the close view, in exploring or in a fight. It does not start in the tactical view, a menu or a conversation, or while a load is still fading in.

## Remove

Delete `bin_ship\daolib\plugins\photocam`.

## Build from source

Visual Studio 2022, vcpkg, Python with Pillow, Java and [JPEXS FFDec](https://github.com/jindrapetrik/jpexs-decompiler). The [README](https://github.com/Whocares66-Dev/DAOPhotoCam#building) has the steps.
