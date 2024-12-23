use std::{collections::{BTreeMap, HashMap, HashSet}, fmt::Debug, fs::{self}, sync::Arc};

use tiled::{PropertyValue, Tileset};
use embedded_graphics::pixelcolor::{RgbColor, IntoStorage};
use deku::prelude::*;
use tinybmp::RawBmp;

use crate::saturn_color_table::SaturnColorTable;

#[derive(Debug, PartialEq, DekuRead, DekuWrite)]
#[deku(endian = "endian", ctx = "endian: deku::ctx::Endian", ctx_default = "deku::ctx::Endian::Big")]
pub struct SaturnTileset {
    #[deku(update = "self.to_bytes().unwrap().len()")]
    pub tileset_size: u32,
    pub tile_width: u32,
    pub tile_height: u32,
    pub tile_count: u32,
    pub bpp: u16,
    pub words_per_palette: u8,
    number_of_colors: u16,
    pub palette_bank: u8,
    #[deku(update = "self.palette.len()")]
    pub palette_size: u32,
    #[deku(count = "palette_size", endian = "big")]
    palette: Vec<u8>,
    #[deku(update = "self.character_pattern.len()")]
    pub character_pattern_size: u32,
    #[deku(count = "character_pattern_size", endian = "big")]
    character_pattern: Vec<u8>
}

impl SaturnTileset {
    fn new(tile_width: u32, tile_height: u32, tile_count: u32, bpp: u16, words_per_palette: u8, number_of_colors:u16, palette_bank:u8) -> Result<Self, String> {
        Ok(SaturnTileset {
            tileset_size: Default::default(),
            tile_width,
            tile_height,
            tile_count,
            bpp,
            number_of_colors,
            words_per_palette,
            palette_size: Default::default(),
            palette: Default::default(),
            palette_bank,
            character_pattern_size: Default::default(),
            character_pattern: Default::default()
        })
    }

    fn read_8x8_tile(raw_image: &Vec<u32>, image_width: i32, image_height: i32, tile_x: i32, tile_y: i32) -> Result<Vec<u16>, String> {
        let mut result: Vec<u16> = Vec::new();
        let rows = raw_image.len() as i32 / image_height;
        for y in 0..8 {
            for x in 0..8 {
                if ((x + tile_x) > image_width) || ((y + tile_y) > image_height) {
                    return Err(format!("Error: Image width {} or height {} not a multiple of 8/16.", image_width, image_height));
                }
                let index = ((rows * (y + tile_y)) + (x + tile_x)) as usize;
                let value = raw_image.get(index).map(|p| *p);
                result.insert((y * 8 + x) as usize, value.ok_or(format!("No pixel value found at {} {}", (x + tile_x), (y + tile_y)))? as u16);
            }
        }
        return Ok(result);
    }

    fn get_pallette_data_32(color_table: &SaturnColorTable) -> Result<Vec<u32>, String> {
        let mut results: Vec<u32> = Vec::default();

        for i in 0..color_table.len(){
            let color = color_table.get(i.try_into().unwrap()).unwrap();
            results.push(u32::from(color.r()) | (u32::from(color.g()) << 8) | (u32::from(color.b()) << 16));
        }

        return Ok(results);
    }

    fn get_pallette_data_16(color_table: &SaturnColorTable) -> Result<Vec<u16>, String> {
        let mut results: Vec<u16> = Vec::default();

        for i in 0..color_table.len() as u32{
            let color = color_table.get(i).ok_or(format!("Unable to get color from color table for index {}", i))?;

            let r = (color.r() & 0xff).overflowing_shr(3);
            let g = ((color.g().overflowing_shr(8).0) & 0xff).overflowing_shr(3);
            let b = ((color.b().overflowing_shr(16).0) & 0xff).overflowing_shr(3);
            results.push(u16::from(r.0) | (u16::from(g.0) << 5) | (u16::from(b.0) << 10) | 0x8000);
        }

        return Ok(results);
    }

    fn get_character_pattern_data(self:&SaturnTileset, raw_image: Vec<u32>, image_width: i32, image_height: i32) -> Result<Vec<u8>, String> {
        let mut results: Vec<u8> = Vec::default();

        match (self.tile_width, self.tile_height) {
            //process for 8x8 tiles
            (8, 8) => {
                for y in (0..image_height).step_by(8) {
                    for x in (0..image_width).step_by(8) {
                        let pixel_data = SaturnTileset::read_8x8_tile(&raw_image, image_width, image_height, x, y)?;

                        match self.bpp {
                            4 => {
                                for i in (0..64).step_by(2) {
                                    results.push((((pixel_data[i] as u8) & 0xF) << 4) | ((pixel_data[i + 1] as u8) & 0xF));
                                }
                            }
                            8 => {
                                for i in 0..64 {
                                    results.push(pixel_data[i] as u8);
                                }
                            }
                            11 => {
                                for i in 0..64 {
                                    results.push(((pixel_data[i] >> 8) & 0x7) as u8);
                                    results.push(pixel_data[i] as u8)
                                }
                            }
                            _ => 
                                return Err::<Vec<u8>, String>(format!("Unsupported bpp for image/color table"))
                        }

                    }
                }

                return Ok(results); 
            }
            (16, 16) => {
                for y in (0..image_height).step_by(16) {
                    for x in (0..image_width).step_by(16) {
                        let mut  res = Vec::<u16>::default();
                        // top left
                        res.append(SaturnTileset::read_8x8_tile(&raw_image, image_width, image_height, x, y)?.as_mut());
                        // top right
                        res.append(SaturnTileset::read_8x8_tile(&raw_image, image_width, image_height, x + 8, y)?.as_mut());
                        // bottom left
                        res.append(SaturnTileset::read_8x8_tile(&raw_image, image_width, image_height, x, y + 8)?.as_mut());
                        // bottom right
                        res.append(SaturnTileset::read_8x8_tile(&raw_image, image_width, image_height, x + 8, y + 8)?.as_mut());
                        

                        match self.bpp {
                            4 => {
                                for i in (0..256).step_by(2) {
                                    results.push((((res[i] as u8) & 0xF) << 4) | ((res[i + 1] as u8) & 0xF));
                                }
                            }
                            8 => {
                                for i in 0..256 {
                                    results.push(res[i] as u8);
                                }
                            }
                            11 => {
                                for i in 0..256 {
                                    results.push(((res[i] >> 8) & 0x7) as u8);
                                    results.push(res[i] as u8);
                                }
                            }
                            _ => 
                                return Err::<Vec<u8>, String>(format!("Unsupported bpp for image/color table"))
                        }
                    }
                }

                return Ok(results);
            }
            _ => return Err(format!("Unsupported tile size {} {}", self.tile_width, self.tile_height))
        }
    }

    fn get_indexed_image<'a>(indexed_palette: &HashMap<u32, u32>, data: &RawBmp) -> Vec<u32> {
        let indexed_image: Vec<u32> = data.color_table().map_or_else(
         ||data.pixels().into_iter().map(|f| *indexed_palette.get(&f.color).unwrap()).collect(),
        |ct| data.pixels().into_iter().map(|f| *indexed_palette.get(&ct.get(f.color).unwrap().into_storage()).unwrap()).collect()
        );
        return indexed_image;       
    }

    fn get_indexed_palette(data: &RawBmp) -> HashMap<u32, u32>{
         let palette:Vec<u32> = data.color_table().map_or_else(
          || Vec::from_iter::<HashSet<u32>>(HashSet::from_iter(data.pixels().map(|p| p.color).into_iter())), 
          |ct|Vec::from_iter((0..ct.len() as u32).map(|i| ct.get(i).unwrap().into_storage()))
        );
        let indexed_palette:HashMap<u32, u32>= palette.iter().cloned().zip((0..palette.len() as u32).into_iter()).collect();
        return indexed_palette;
    }

    fn get_color_table(indexed_palette: &HashMap<u32, u32>) -> SaturnColorTable{
        let sorted: BTreeMap<u32,u32> = indexed_palette.iter().map(|(k,v)| (*v,*k)).collect();
        return SaturnColorTable::new(sorted.clone().into_values().collect()); 
    }

    fn get_number_of_colors(indexed_palette: &HashMap<u32, u32>) -> Result<u16, String> {
        return match indexed_palette.len() {
            1..=16      => Ok(16),
            17..=256    => Ok(256),
            257..=1024  => Ok(1024),
            1025..=2048 => Ok(2048),
            _ => Err(format!("Unsupported color table length {}", indexed_palette.len()))
        }
    }

    fn get_bpp(number_of_colors:u16) -> Result<u16, String> {
        return match number_of_colors {
            16   => Ok(4),
            256  => Ok(8),
            1024 => Ok(11),
            2048 => Ok(11),
            _ => Err(format!("Unsupported number of colors {}", number_of_colors))
        }
    }

    fn get_palette_bank(tileset:&Arc<Tileset>) -> Result<u8, String> {
        let palette_bank_property_value = tileset.properties.get("palette_bank").ok_or("No palette_bank property found for tileset")?;
        let palette_bank : u8  = match palette_bank_property_value  {
            PropertyValue::IntValue(s) => *s as u8,
            PropertyValue::StringValue(c) => c.parse().map_err(|e| format!("Invalid palette bank {:?}", e))?,
            _ => 0
        };

        Ok(palette_bank)
    }

    fn get_words_per_palette(tileset:&Arc<Tileset>) -> Result<u8, String> {
        let words_per_palette_property_value = tileset.properties.get("pnd_size").ok_or("No pnd_size property found for tileset")?;
        let words_per_palette : u8  = match words_per_palette_property_value  {
            PropertyValue::IntValue(s) => *s as u8,
            PropertyValue::StringValue(c) => c.parse().map_err(|e| format!("Invalid pnd_size {:?}", e))?,
            _ => Err("Invalid pnd_size")?
        };

        Ok(words_per_palette)
    }

    pub fn build(tilesets: &[Arc<Tileset>]) -> Result<Vec<Self>, String> {
        let mut results: Vec<SaturnTileset> = Vec::default();

        for tileset in tilesets.iter() {
            let image = tileset.as_ref().clone().image.ok_or("No Image for tileset found")?;
            let image_file = fs::read(image.source.as_path()).map_err(|op| op.to_string() + " " + image.source.as_path().to_str().unwrap())?;
            let raw_bmp = RawBmp::from_slice(&image_file).map_err(|op| format!("{:?}", op))?;
            let indexed_palette = &SaturnTileset::get_indexed_palette(&raw_bmp);
            let indexed_image = SaturnTileset::get_indexed_image(indexed_palette, &raw_bmp);
            let number_of_colors = SaturnTileset::get_number_of_colors(indexed_palette)?;
            let bpp = SaturnTileset::get_bpp(number_of_colors)?;
            let color_table = &SaturnTileset::get_color_table(indexed_palette);
            let palette_bank = SaturnTileset::get_palette_bank(tileset)?;
            let words_per_palette = SaturnTileset::get_words_per_palette(tileset)?;

            let mut saturn_tileset = SaturnTileset::new(tileset.tile_width, tileset.tile_height, tileset.tilecount, bpp, words_per_palette, number_of_colors, palette_bank)?;                                                              
 
            let mut pallete_data_bytes : Vec<u8> = if words_per_palette == 1 {
                let pallete_data = &mut SaturnTileset::get_pallette_data_16(color_table)?;
                pallete_data.iter().flat_map(|val| val.to_be_bytes()).collect()
            } else {
                let pallete_data = &mut SaturnTileset::get_pallette_data_32(color_table)?;
                pallete_data.iter().flat_map(|val| val.to_be_bytes()).collect()
            };

            saturn_tileset.palette.append(&mut pallete_data_bytes);

            let character_pattern_data = &mut SaturnTileset::get_character_pattern_data(&saturn_tileset, indexed_image, image.width, image.height)?;
            saturn_tileset.character_pattern.append(character_pattern_data);

            saturn_tileset.update().map_err(|op| op.to_string())?;

            results.push(saturn_tileset);
        }
        
        return Ok(results);
    }
}