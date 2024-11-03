tiled2saturn
============

tiled2saturn converts between Tiled generated maps and Sega Saturn formats. This tool allows you to extract all components of a Tiled map (.tmx) and convert them into a single binary representation which can be parsed for use in Sega Saturn games.

A C library provides functionality to parse and manipulate Tiled2Saturn map data. The library allows you to easily read header, tilesets, layers and collision data from Tiled maps.

Supported Features
------------------

- BMP images in both indexed and raster formats
- 4, 8, 11 BPP
- Horizontal/Vertical Tile Flip
- Tile Transparency detected based on underlying layers
- RGB555 and RGB888 palette formats

Prerequisites
-------------

Before using tiled2saturn, make sure you have the following installed on your system:

-   Rust > 1.74: You can download and install Rust from <https://www.rust-lang.org/tools/install>.

Installation
------------

To use tiled2saturn, you need to build the program from source:

1.  Clone the repository:

    `git clone https://github.com/hywelandrews/tiled2saturn.git`

2.  Change your working directory to the tiled2saturn project folder:

    `cd tiled2saturn`

3.  Build the program using Rust's package manager, Cargo:

    `cargo build --release`

The tiled2saturn binary will be located in the "target/release" directory and can be executed from there.

Usage
-----

tiled2saturn provides a command-line interface with a single subcommand "extract" to convert Tiled maps into a Sega Saturn format. Here's how to use the program:

`tiled2saturn extract [OPTIONS] <TMX_FILE>`

-   `<TMX_FILE>` (Required): The path to the Tiled map (.tmx) file you want to extract.

### Configuration

There are two custom configuration properties that need to be added to tilesets:

`palette_bank` - bank number that PND data should reference, for 2048 color count images this should be 0 
`pnd_size` - value is either 1 or 2 dending on PND format SCL_PN_10BIT or 2 word.

### Example

```bash
tiled2saturn extract path/to/map.tmx
```

This command will extract the components of the specified Tiled map and convert them into a single binary representation. The resulting binary file will be named "data.bin" in the current working directory.

Getting Started with the C library
----------------------------------

You can either include the library in your project or install globally in your framework, only [Yaul](https://www.yaul.org) is supported currently. Here are the basic steps to get started:

### Manual installation

Copy the Tiled2Saturn files from libtiled2saturn/ into your project, adding both `tiled2saturn.h` and `tiled2saturn.c`. Your Makefile will need to include `tiled2saturn.c` in its `SRCs`. You can then include tiled2saturn locally. 

```C
#include "tiled2saturn.h"
```

### Yaul installation

Alternatively run `make install` from the libtiled2saturn directory with elevated privileges in a yaul environment.

```C
#include <tiled2saturn/tiled2saturn.h>
```

You will also need to include the shared build file in your Makefile

```Makefile
include $(YAUL_INSTALL_ROOT)/share/build.tiled2saturn.mk
```

And append the CFLAGS and LDFLAGS to yours

```Makefile
   SH_CFLAGS+= $(TILED2SATURN_CFLAGS)
   SH_LDFLAGS+= $(TILED2SATURN_LDFLAGS)
```

### Usage

Use the library to parse Tiled2Saturn map data by calling the tiled2saturn_parse function. This function returns a data structure containing the parsed map, including a header, tilesets, and layers.

```C
uint8_t* level; // pointer to raw data.bin
tiled2saturn_t* t2s = tiled2saturn_parse(level);

const uint8_t moon_layer_id = 1;
const uint8_t clouds_layer_id = 2;
const uint8_t floor_layer_id = 3;
```
Access and Manipulate Data: Access and manipulate the parsed map data as needed for your application. You can retrieve layers by their IDs, access tilesets, and more.
```C
// Load Moon background
tiled2saturn_layer_t* moon = get_layer_by_id(t2s, moon_layer_id);
// Load Clouds background
tiled2saturn_layer_t* clouds = get_layer_by_id(t2s, clouds_layer_id);
// Load Floor background
tiled2saturn_layer_t* floor = get_layer_by_id(t2s, floor_layer_id);
// Use collision data extracted from Tiled
parse_collisions(t2s->collisions);
```
Cleanup Resources: When you're done with the parsed data, be sure to free the memory allocated for the map and its components using the tiled2saturn_free function to avoid memory leaks.
```C
tiled2saturn_free(t2s);
```

Full examples for single and multiple layers can be found [here](https://github.com/hywelandrews/tiled2saturn/tree/master/examples).

License
-------

This software is released under the MIT License. You can find the full text of the license in the "LICENSE.md" file.

Acknowledgments
---------------

-   Satconv (https://git.sr.ht/~ndiddy/satconv/tree): The original converter of map.tmx (Tiled XML) to .map files for Saturn
-   Tiled (<https://www.mapeditor.org/>): Tiled is a popular open-source map editor that makes it easy to create and edit maps for various game engines.

If you encounter any issues or have suggestions for improvement, please open an issue on the project's GitHub repository.
