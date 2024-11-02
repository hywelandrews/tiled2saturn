use tiled::Map;
use crate::saturn_bitmap_layer::SaturnBitmapLayer;
use crate::saturn_tileset::SaturnTileset;
use crate::saturn_layer::SaturnLayer;
use crate::saturn_collisions::SaturnCollision;

use deku::prelude::*;

#[repr(C)]
#[derive(Debug, PartialEq, DekuRead, DekuWrite)]
#[deku(endian = "endian", ctx = "endian: deku::ctx::Endian", ctx_default = "deku::ctx::Endian::Big")]
struct SaturnMapHeader {
    magic: u32, 
    version: u32,
    width: u32, // Width of the map, in tiles.
    height: u32,
    tileset_count: u8, 
    #[deku(update = "self.to_bytes().unwrap().len()")]
    tileset_offset: u32,
    layer_count: u8,
    #[deku(update = "(self.to_bytes().unwrap().len() as u32) + self.layer_offset")]
    layer_offset: u32,
    bitmap_layer_count: u8,
    #[deku(update = "(self.to_bytes().unwrap().len() as u32) + self.bitmap_layer_offset")]
    bitmap_layer_offset: u32,
    #[deku(update = "(self.to_bytes().unwrap().len() as u32) + self.collision_offset")]
    collision_offset: u32
}

impl SaturnMapHeader {
    fn new(width: u32, height: u32, tileset_count:u8, tilesets_size: u32, layer_count: u8, layers_size: u32, bitmap_layer_count: u8, bitmap_layers_size: u32) -> Self {
        SaturnMapHeader {
            magic: 0x894D4150, 
            version: 4, 
            width, 
            height,
            tileset_count,
            tileset_offset: Default::default(),
            layer_count,
            layer_offset: tilesets_size,
            bitmap_layer_count,
            bitmap_layer_offset: tilesets_size + layers_size,
            collision_offset: tilesets_size + layers_size + bitmap_layers_size
        }
    }
}

#[derive(Debug, PartialEq, DekuWrite)]
#[deku(ctx = "endian: deku::ctx::Endian", ctx_default = "deku::ctx::Endian::Big")]
pub struct SaturnMap {
    header: SaturnMapHeader,
    #[deku(count = "header.tileset_count", endian = "big")]
    tilesets: Vec<SaturnTileset>,
    #[deku(count = "header.layer_count", endian = "big")]
    layers: Vec<SaturnLayer>,
    #[deku(count = "header.bitmap_layer_count", endian = "big")]
    bitmap_layers: Vec<SaturnBitmapLayer>,
    #[deku(count = "header.width * header.height", endian = "big")]
    collisions: Vec<SaturnCollision>
}

impl SaturnMap {
    pub fn build(map: Map, words_per_pallete: u8) -> Result<SaturnMap, String> {
        let width = map.width;
        let height = map.height;

        let tilesets = SaturnTileset::build(map.tilesets(), words_per_pallete)?;
        let tileset_count = u8::try_from(tilesets.len()).map_err(|e| e.to_string())?;
        let tilesets_size: u32 = tilesets.iter().map(|f| f.tileset_size).sum();

        let layers = SaturnLayer::build(map.layers(), &tilesets)?;
        let layer_count = u8::try_from(layers.len()).map_err(|e| e.to_string())?;
        let layers_size: u32 = layers.iter().map(|f| f.layer_size).sum();

        let bitmap_layers = SaturnBitmapLayer::build(map.layers(), words_per_pallete)?;
        let bitmap_layer_count = u8::try_from(bitmap_layers.len()).map_err(|e| e.to_string())?;
        let bitmap_layers_size = bitmap_layers.iter().map(|f| f.layer_size).sum();

        let collisions = SaturnCollision::build(width, height, map.layers())?;
        
        let mut header = SaturnMapHeader::new(width, height, tileset_count, tilesets_size, 
                                                               layer_count, layers_size, bitmap_layer_count, 
                                                               bitmap_layers_size);
                                                               
        header.update().map_err(|e| e.to_string())?;

        return Ok(SaturnMap {
            header,
            tilesets,
            layers,
            bitmap_layers,
            collisions
        });
    }
}