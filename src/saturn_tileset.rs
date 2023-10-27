use std::{sync::Arc, fs::{self}};

use tiled::Tileset;
use tinybmp::{RawBmp, ColorTable};
use embedded_graphics::geometry::Point;
use embedded_graphics::pixelcolor::RgbColor;
use deku::prelude::*;

#[derive(Debug, PartialEq, DekuRead, DekuWrite)]
#[deku(endian = "endian", ctx = "endian: deku::ctx::Endian", ctx_default = "deku::ctx::Endian::Big")]
pub struct SaturnTileset {
    #[deku(update = "self.to_bytes().unwrap().len()")]
    pub tileset_size: u32,
    pub tile_width: u32,
    pub tile_height: u32,
    pub tile_count: u32,
    pub bpp: u16,
    pub words_per_pallete: u8,
    number_of_colors: u16,
    #[deku(update = "self.palatte.len()")]
    pub palatte_size: u32,
    #[deku(count = "palatte_size", endian = "big")]
    palatte: Vec<u8>,
    #[deku(update = "self.character_pattern.len()")]
    pub character_pattern_size: u32,
    #[deku(count = "character_pattern_size", endian = "big")]
    character_pattern: Vec<u8>
}

impl SaturnTileset {
    fn new(tile_width: u32, tile_height: u32, tile_count: u32, bpp: u16, words_per_pallete: u8, number_of_colors:u16) -> Result<Self, String> {
        Ok(SaturnTileset {
            tileset_size: Default::default(),
            tile_width,
            tile_height,
            tile_count,
            bpp,
            number_of_colors,
            words_per_pallete,
            palatte_size: Default::default(),
            palatte: Default::default(),
            character_pattern_size: Default::default(),
            character_pattern: Default::default()
        })
    }

    fn read_8x8_tile(bmp: RawBmp<'_>, image_width: i32, image_heigt: i32, tile_x: i32, tile_y: i32) -> Result<Vec<u8>, String> {
        let mut result: Vec<u8> = Vec::new();
        for y in 0..8 {
            for x in 0..8 {
                if ((x + tile_x) > image_width) || ((y + tile_y) > image_heigt) {
                    return Err(format!("Error: Image width {} or height {} not a multiple of 8/16.", image_width, image_heigt));
                }
                let value = bmp.pixel(Point::new(x + tile_x, y + tile_y));
                result.insert((y * 8 + x) as usize, value.ok_or(format!("No pixel value found at {} {}", (x + tile_x), (y + tile_y)))? as u8);
            }
        }
        return Ok(result);
    }

    fn get_pallette_data_32(self:&SaturnTileset, color_table: &ColorTable<'_>) -> Result<Vec<u32>, String> {
        let mut results: Vec<u32> = Vec::default();

        for i in 0..color_table.len(){
            let color = color_table.get(i.try_into().unwrap()).unwrap();
            results.push(u32::from(color.r()) | (u32::from(color.g()) << 8) | (u32::from(color.b()) << 16));
        }

        return Ok(results);
    }

    fn get_pallette_data_16(self:&SaturnTileset, color_table: &ColorTable<'_>) -> Result<Vec<u16>, String> {
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

    fn get_character_pattern_data(self:&SaturnTileset, bmp: RawBmp<'_>, image_width: i32, image_height: i32) -> Result<Vec<u8>, String> {
        let mut results: Vec<u8> = Vec::default();

        if (self.bpp != 4) && (self.bpp != 8) {
            return Err::<Vec<u8>, String>(format!("Error: Only indexed images supported for tile mode"));
        }

        match (self.tile_width, self.tile_height) {
            (8, 8) => {
                for y in (0..image_height).step_by(8) {
                    for x in (0..image_width).step_by(8) {
                        let pixel_data = SaturnTileset::read_8x8_tile(bmp, image_width, image_height, x, y)?;

                        if self.bpp == 4 {
                            for i in (0..64).step_by(2) {
                                results.push(((pixel_data[i] & 0xF) << 4) | (pixel_data[i + 1] & 0xF));
                            }
                        }
                        else {
                            for i in 0..64 {
                                results.push(pixel_data[i]);
                            }
                        }

                    }
                }

                return Ok(results); //process for 8x8 tiles
            }
            (16, 16) => {
                for y in (0..image_height).step_by(16) {
                    for x in (0..image_width).step_by(16) {
                        let mut  res = Vec::<u8>::default();
                        // top left
                        res.append(SaturnTileset::read_8x8_tile(bmp, image_width, image_height, x, y)?.as_mut());
                        // top right
                        res.append(SaturnTileset::read_8x8_tile(bmp, image_width, image_height, x + 8, y)?.as_mut());
                        // bottom left
                        res.append(SaturnTileset::read_8x8_tile(bmp, image_width, image_height, x, y + 8)?.as_mut());
                        // bottom right
                        res.append(SaturnTileset::read_8x8_tile(bmp, image_width, image_height, x + 8, y + 8)?.as_mut());
                        
                        if self.bpp == 4 {
                            for i in (0..256).step_by(2) {
                                results.push(((res[i] & 0xF) << 4) | (res[i + 1] & 0xF));
                            }
                        }
                        else {
                            for i in 0..256 {
                                results.push(res[i]);
                            }
                        }
                    }
                }

                return Ok(results);
            }
            _ => return Err(format!("Unsupported tile size {} {}", self.tile_width, self.tile_height))
        }
    }

    pub fn build(tilesets: &[Arc<Tileset>], words_per_pallete: u8) -> Result<Vec<Self>, String> {
        let mut results: Vec<SaturnTileset> = Vec::default();

        for tileset in tilesets.iter() {
            let image = tileset.as_ref().clone().image.ok_or("No Image for tileset found")?;
            let image_file = fs::read(image.source.as_path()).map_err(|op| op.to_string() + " " + image.source.as_path().to_str().unwrap())?;
            let raw_bmp = RawBmp::from_slice(&image_file).map_err(|op| format!("{:?}", op))?;
            let color_table = raw_bmp.color_table().ok_or("Color table missing")?;
            let mut saturn_tileset = SaturnTileset::new(tileset.tile_width, tileset.tile_height, tileset.tilecount, raw_bmp.header().bpp.bits(), words_per_pallete, color_table.len() as u16)?;                                                              
            
            let mut pallete_data_bytes : Vec<u8> = if words_per_pallete == 1 {
                let pallete_data = &mut SaturnTileset::get_pallette_data_16(&saturn_tileset, color_table)?;
                pallete_data.iter().flat_map(|val| val.to_be_bytes()).collect()
            } else {
                let pallete_data = &mut SaturnTileset::get_pallette_data_32(&saturn_tileset, color_table)?;
                pallete_data.iter().flat_map(|val| val.to_be_bytes()).collect()
            };

            saturn_tileset.palatte.append(&mut pallete_data_bytes);

            let character_pattern_data = &mut SaturnTileset::get_character_pattern_data(&saturn_tileset, raw_bmp, image.width, image.height)?;
            saturn_tileset.character_pattern.append(character_pattern_data);

            saturn_tileset.update().map_err(|op| op.to_string())?;

            results.push(saturn_tileset);
        }

        return Ok(results);
    }
}