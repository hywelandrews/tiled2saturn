use deku::prelude::*;
use tiled::{Layer, TileLayer};

use crate::saturn_tileset::SaturnTileset;

#[repr(C)]
#[derive(Debug, PartialEq, DekuWrite)]
#[deku(endian = "endian", ctx = "endian: deku::ctx::Endian", ctx_default = "deku::ctx::Endian::Big")]
pub struct SaturnLayer{
    id: u32,
    #[deku(update = "self.to_bytes().unwrap().len()")]
    pub layer_size: u32,
    width: u32,
    height: u32,
    tileset_index: u16,
    tile_flip_enabled: bool,
    tile_transparency_enabled: bool,
    #[deku(update = "self.pattern_name_data.len()")]
    pattern_name_data_size: u32,
    #[deku(count = "character_pattern_size", endian = "big")]
    pattern_name_data:Vec<u8>
}

impl SaturnLayer {
    fn new(id: u32, width: u32, height: u32, tileset_index:u16, tile_flip_enabled:bool, tile_transparency_enabled: bool) -> Result<Self, String> {
        Ok(SaturnLayer {
            id,
            layer_size: Default::default(),
            width,
            height,
            tileset_index,
            tile_flip_enabled,
            tile_transparency_enabled,
            pattern_name_data_size: Default::default(),
            pattern_name_data: Default::default(),
        })
    }

    fn get_pattern_name_data<'a>(self:&SaturnLayer, tile_layer: &TileLayer<'a>, tilesets:&Vec<SaturnTileset>) -> Result<Vec<u8>, String> {
        let mut results: Vec<u8> = Vec::default();

        let tileset = tilesets.get(self.tileset_index as usize).expect(format!("Invalid tileset index {} for layer", self.tileset_index).as_str());

        // We sequentially reference tile ids based on the previous tilesets that exist in the map, this assumes you load tilesets in the same order
        let current_tile_index : u16 = (0..self.tileset_index as usize).flat_map(|num| tilesets.get(num).map(|t| (t.tile_count) as u16)).sum();

        let nunber_of_tiles_per_map = match (tileset.tile_height, tileset.tile_width) {
            (16, 16) => Ok(32),
            (8, 8) => Ok(64),
            e => Err(format!("Invalid tile size {:?} for saturn map", e))
        }?;

        let number_of_maps_y = self.height / nunber_of_tiles_per_map;
        let number_of_maps_x = self.width  / nunber_of_tiles_per_map;

        for map_index_y in 0..number_of_maps_y {
            let start_y_offset = nunber_of_tiles_per_map * map_index_y;
            let end_y_offset   = (nunber_of_tiles_per_map * map_index_y) + nunber_of_tiles_per_map;
            for map_index_x in 0..number_of_maps_x {
                let start_x_offset = nunber_of_tiles_per_map * map_index_x;
                let end_x_offset   = (nunber_of_tiles_per_map * map_index_x) + nunber_of_tiles_per_map;
                for y in start_y_offset..end_y_offset{
                    for x in start_x_offset..end_x_offset{
                        let (tile_id, flip_horizontal, flip_vertical) = tile_layer.get_tile(x as i32,y as i32).map(|f| (f.id(), f.flip_h, f.flip_v)).unwrap_or((u32::MAX, false, false));
                        let in_val = tile_id;
                        if tileset.words_per_pallete == 1 {
                            let mut out_val = if tileset.bpp == 11{
                                    (in_val as u16 & 0x3ff) << 2
                                } else if tileset.bpp == 8 {
                                    (in_val as u16 & 0x3ff) << 1
                                } else if tileset.bpp == 4 {
                                    in_val as u16 & 0x3ff
                                } else {0};
                            
                            // is tile horizontally flipped?
                            if flip_horizontal {
                                out_val |= 0x400;
                            }
                            // is tile vertically flipped?
                            if flip_vertical {
                                out_val |= 0x800;
                            }
                            
                            //  current_tile_index - this is the tileset count we are currently at given all previous tilesets that exist - unless our tile is transparent, increment by index
                            if tile_id != u32::MAX {
                                out_val += current_tile_index; 
                            }
                            
                            results.append(&mut out_val.to_be_bytes().to_vec());
                        } else {
                            let mut out_val = (in_val & 0x7fff) << 1;
                    
                            // is tile horizontally flipped?
                            if flip_horizontal {
                                out_val |= 0x40000000;
                            }
                            // is tile vertically flipped?
                            if flip_vertical { // Is this a bug from original satconv? verify.
                                out_val |= 0x40000000;
                            }
                            results.append(&mut out_val.to_be_bytes().to_vec());
                        }
                    }
                }
            }
        }

       return Ok(results);
    }

    pub fn build<'a>(layers: impl ExactSizeIterator<Item = Layer<'a>>, tilesets:&Vec<SaturnTileset>) -> Result<Vec<Self>, String> {
        let mut results: Vec<SaturnLayer> = Vec::default();

        let tile_layers: Vec<(u32, TileLayer)> = layers.filter_map(|layer| match layer.layer_type() {
            tiled::LayerType::Tiles(tile_layer) => Some((layer.id(), tile_layer)),
            _ => None,
        }).collect();

        fn are_all_elements_equal<T: PartialEq>(elems: &[T]) -> Option<&T> {
            match elems {
                [head, tail @ ..] => tail.iter().all(|x| x == head).then(|| head),
                [] => None,
            }
        }

        fn tileset_index_for_layer(height:u32, width:u32, tile_layer:&TileLayer) -> Result<u16, String> {
            let mut res : Vec<Option<usize>> = Vec::default();

            for y in 0..height {
                for x in 0..width {
                    let t = tile_layer.get_tile(x as i32,y as i32);
                    let o = t.map(|f| f.tileset_index());
                    res.push(o);
                }
            }

            let filtered : Vec<&usize> = res.iter().flatten().collect();
            let index = are_all_elements_equal(&filtered).map(|f| **f as u16).ok_or(format!("Layers can only contain tiles from a single tileset"));

            return index;
        }

        fn tile_flip_enabled(height:u32, width:u32, tile_layer:&TileLayer) -> bool {
            for y in 0..height {
                for x in 0..width {
                    let t = tile_layer.get_tile(x as i32,y as i32);
                    let o = t.map(|f| f.flip_d | f.flip_h | f.flip_v);
                    if o.is_some() && o.unwrap() {
                        return true
                    }
                }
            }
            return false;
        }

        fn tile_transparency_enabled<'a>(height:u32, width:u32, previous_layers: impl Iterator<Item = &'a(u32,TileLayer<'a>)>) -> bool {
            for layer in previous_layers {
                for y in 0..height {
                    for x in 0..width {
                        let previous_tile = layer.1.get_tile(x as i32,y as i32);
                        if previous_tile.is_some() {
                            return true
                        }
                    }
                }
            }

            return false;
        }

        let mut index = 1_usize;
        
        for (id, tile_layer) in tile_layers.iter() {
            let width = tile_layer.width().ok_or(format!("Unable to get width for layer {}", id))?;
            let height = tile_layer.height().ok_or(format!("Unable to get height for layer {}", id))?;
            
            let tileset_index = tileset_index_for_layer(height, width, tile_layer)?;
            let tile_flip_enabled = tile_flip_enabled(height, width, tile_layer);

            let previous_layers = tile_layers.iter().take(index.saturating_sub(1));

            let tile_transparency_enabled = tile_transparency_enabled(height, width, previous_layers);

            let mut saturn_layer = SaturnLayer::new(*id, width, height, tileset_index, tile_flip_enabled, tile_transparency_enabled)?;

            let pattern_data = &mut SaturnLayer::get_pattern_name_data(&saturn_layer, tile_layer, &tilesets)?;
            saturn_layer.pattern_name_data.append(pattern_data);
            saturn_layer.update().map_err(|op| op.to_string())?;

            results.push(saturn_layer);

            index+=1;
        }

        return Ok(results);
    }
}