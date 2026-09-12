# Vibrant Engine
A rendering engine focused on 2D lighting and the use of normals to create beautiful scenes.
![](docs/screenshot.png)

## Features
  * Dynamic Lighting
  * Normal Support
  * Framebuffer Scaling

## Optimization

Vibrant is optimized through and through, with the following notable optimization tricks used: (Tested with a scene with 10000 objects)

|              Name              | Perf. Before | Perf. After |
|--------------------------------|--------------|-------------|
| Use of C++ (low level language)| ---          | ---         |
| Optimized object structure     | 3.2 GB       | 1.9 GB      |

## Installation
### Linux
1. Install dependencies
`pugixml`
  * Example
`sudo pacman -Sy pugixml` (For Arch Linux)
You will also need `kdialog` OR `xdialog` OR `applescript` for UI input. If you lack any of these, tinyfiledialogs will fall back to console input.
2. Download the latest Linux release from the [releases page](https://github.com/SoHiEarth/vibrant/releases)
3. cd into the folder and run the executable
`cd /path/to/vibrant/executable;./vibrant`
  ** If the program throws an error complaining about missing dependencies, install them from your distro's package manager. **
### Windows
1. Download the latest Windows release from the [releases page](https://github.com/SoHiEarth/vibrant/releases)
2. Run the executable (`vibrant.exe`)
