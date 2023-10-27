use tiled::Map;
use crate::saturn_tileset::SaturnTileset;
use crate::saturn_layer::SaturnLayer;

use deku::prelude::*;

#[repr(C)]
#[derive(Debug, PartialEq, DekuRead, DekuWrite)]
#[deku(endian = "endian", ctx = "endian: deku::ctx::Endian", ctx_default = "deku::ctx::Endian::Big")]
struct SaturnMapHeader {
    magic: u32,
    version: u32,
    width: u32,
    height: u32,
    tileset_count: u8, 
    #[deku(update = "self.to_bytes().unwrap().len()")]
    tileset_offset: u32,
    layer_count: u8,
    #[deku(update = "(self.to_bytes().unwrap().len() as u32) + self.layer_offset")]
    layer_offset: u32,
}

impl SaturnMapHeader {
    fn new(width: u32, height: u32, tileset_count:u8, tileset_size: u32, layer_count: u8) -> Self {
        SaturnMapHeader {
            magic: 0x894D4150,
            version: 1, 
            width,
            height,
            tileset_count,
            tileset_offset: Default::default(),
            layer_count,
            layer_offset: tileset_size 
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
    layers: Vec<SaturnLayer>
}

impl SaturnMap {
    pub fn build(map: Map, words_per_pallate: u8) -> Result<SaturnMap, String> {
        let width = map.width;
        let height = map.height;

        let tilesets = SaturnTileset::build(map.tilesets(), words_per_pallate)?;
        let tileset_count = u8::try_from(tilesets.len()).map_err(|e| e.to_string())?;
        let tileset_size: u32 = tilesets.iter().map(|f| f.tileset_size).sum();

        let layers = SaturnLayer::build(map.layers(), &tilesets)?;
        let layer_count = u8::try_from(layers.len()).map_err(|e| e.to_string())?;

        let mut header = SaturnMapHeader::new(width, height, tileset_count, tileset_size, layer_count);
        header.update().map_err(|e| e.to_string())?;

        return Ok(SaturnMap {
            header,
            tilesets,
            layers
        });
    }
}