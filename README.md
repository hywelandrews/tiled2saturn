tiled2saturn
============

tiled2saturn is a converter between Tiled generated maps and Sega Saturn formats. This tool allows you to extract all components of a Tiled map (.tmx) and convert them into a single binary representation compatible with Sega Saturn.

There is also a C library that provides functionality to parse and manipulate Tiled2Saturn map data. The library allows you to extract header information, tilesets, and layers from Tiled2Saturn maps, as well as search for layers by their IDs.

Prerequisites
-------------

Before using tiled2saturn, make sure you have the following prerequisites installed on your system:

-   Rust: You can download and install Rust from <https://www.rust-lang.org/tools/install>.

Installation
------------

To use tiled2saturn, you need to build the program from source:

1.  Clone the repository:

    `git clone https://github.com/your-username/tiled2saturn.git`

2.  Change your working directory to the tiled2saturn project folder:

    `cd tiled2saturn`

3.  Build the program using Rust's package manager, Cargo:

    `cargo build --release`

The tiled2saturn binary will be located in the "target/release" directory and can be executed from there.

Usage
-----

tiled2saturn provides a command-line interface with a single subcommand "extract" to convert Tiled maps into Sega Saturn format. Here's how to use the program:

`tiled2saturn extract [OPTIONS] <TMX_FILE>`

-   `<TMX_FILE>` (Required): The path to the Tiled map (.tmx) file you want to extract.

### Options

-   `-w, --WORDS <WORDS>`: Pallete word size (Required). The number of words to represent the palette.

### Example

`tiled2saturn extract -w 1 path/to/map.tmx`

This command will extract the components of the specified Tiled map and convert them into a single binary representation suitable for Sega Saturn. The resulting binary file will be named "data.bin" in the current working directory.

Getting Started with the C library
----------------------------------

To get started, you can either include the library in your C project or build an application that uses the library. Here are the basic steps to get started:

Include the Library: Copy the Tiled2Saturn files from c_lib/ into your project by adding the tiled2saturn.h header file and including the c source in your build file.

Parse Tiled2Saturn Map: Use the library to parse Tiled2Saturn map data by calling the tiled2saturn_parse function. This function returns a data structure containing the parsed map, including header, tilesets, and layers.

Access and Manipulate Data: Access and manipulate the parsed map data as needed for your application. You can retrieve layers by their IDs, access tilesets, and more.

Cleanup Resources: When you're done with the parsed data, be sure to free the memory allocated for the map and its components using the tiled2saturn_free function to avoid memory leaks.

License
-------

This software is released under the MIT License. You can find the full text of the license in the "LICENSE.md" file.

Acknowledgments
---------------

-   Satconv (https://git.sr.ht/~ndiddy/satconv/tree): The original converter of map.tmx (Tiled XML) to .map files for Saturn
-   Tiled (<https://www.mapeditor.org/>): Tiled is a popular open-source map editor that makes it easy to create and edit maps for various game engines.
-   Sega Saturn: The Sega Saturn was a gaming console released by Sega in the 1990s.

If you encounter any issues or have suggestions for improvement, please open an issue on the project's GitHub repository.