use std::fs;
use std::io::Write; // bring trait into scope
use tiled::{Loader, Map};
use clap::{Command, arg};

use crate::saturn_map::SaturnMap;
mod saturn_map;
mod saturn_tileset;
mod saturn_color_table;
mod saturn_layer;
mod saturn_collisions;

use deku::prelude::*;

fn cli() -> Command {
    Command::new("tiled2saturn")
        .about("A converter between Tiled generated maps and sega saturn formats")
        .subcommand_required(true)
        .arg_required_else_help(true)
        .subcommand(
            Command::new("extract")
                .about("Extracts all componenets of a tmx map into a single binary representation")
                .arg(arg!(-w<WORDS>).value_parser(clap::value_parser!(u8).range(1..2)))
                .arg(arg!(<TMX_FILE> "The tmx file to extract from"))
                .arg_required_else_help(true),
        )
}

fn load_tmx(filename: &str) -> Map {
    let mut loader = Loader::new();
    return loader.load_tmx_map(filename).unwrap();
}

fn main() {

    let matches = cli().get_matches();

    match matches.subcommand() {
        Some(("extract", sub_matches)) => {
            let pallete_size = *sub_matches.get_one::<u8>("WORDS").expect("Pallete word size is required");
            let filename = sub_matches.get_one::<String>("TMX_FILE").expect("TMX file to process is required");
            println!("Extracting {} with pallete word size {}", filename, pallete_size);
            let tmx_file = load_tmx(filename);
            let saturn_map = SaturnMap::build(tmx_file, pallete_size);

            let map_bytes = match saturn_map {
                Ok(map) => map.to_bytes().map_err(|err| err.to_string()),
                Err(msg) => Err(msg)
            };

            match map_bytes {
                Ok(map) =>  {
                    let mut file = fs::OpenOptions::new()
                        .create(true) 
                        .write(true)
                        .open("data.bin").expect("Unable to open file data.bin");
                    file.write_all(&map).expect("Failed to write bytes to output file");
                    println!("Completed")
                }
                Err(err) => println!("{}", err)
            }
        }
        _ => unreachable!(), // If all subcommands are defined above, anything else is unreachable!()
    }
}
