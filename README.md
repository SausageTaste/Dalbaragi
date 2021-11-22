![Main Image](/screenshot/main_screenshot.jpg)

# What is this?

My personal game engine project powered by Vulkan.

My goal is to catch up my previous [OpenGL game engine project](https://github.com/SausageTaste/Little-Ruler) in terms of rendering techniques and image quality.

Supported platforms are Windows, Linux and Android.

# How to build

* Download [LunarG Vulkan SDK](https://www.lunarg.com/vulkan-sdk/) and install it
* *CMake* is the only supported build system so please use it
* *Git* is required for CMake to fetch dependencies automatically

## For Windows

* Build CMake target `dalbaragi_windows` and execute it
* Building target `all` will produce error. Sorry about that
  ![Main Image](/screenshot/vscode_config.png)

## For Linux

* I have only tested it on Ubuntu
* Build CMake target `dalbaragi_linux` and execute it
* On Ubuntu you may need to execute followings to configure CMake without error
    ```
    sudo apt-get install libx11-dev
    sudo apt-get install xorg-dev libglu1-mesa-dev
    ```

## For Android

* Open directory `{repo root}/app/android` on Android Studio and build it there

# Control

Most commands are consistent across all supported platforms unless otherwise specified.

On Android, you can use Bluetooth keyboard or just stick to touch screen control.

## Keyboard control

| Key | Command
|-|-
| WASD | Move
| Arrow keys | Look around
| SPACE | Ascend vertically
| Left Ctrl | Descend vertically
| Q or E | Tilt left or right
| Alt + Enter | Toggle full screen (Windows and Linux)

## Touch control

| Action | Command
|-|-
| Swipe on left side of window | Move
| Swipe on right side of window | Look around
