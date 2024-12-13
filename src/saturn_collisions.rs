use deku::prelude::*;
use tiled::{Layer, TileLayer};

#[repr(u8)]
#[derive(Debug, PartialEq, DekuWrite, Clone)]
#[deku(type = "u8", endian = "endian", ctx = "endian: deku::ctx::Endian", ctx_default = "deku::ctx::Endian::Big")]
enum CollisionType{
    Empty = 0,
    Rect = 1,
    Polygon = 2
}

#[repr(C)]
#[derive(Debug, PartialEq, DekuWrite, Clone)]
#[deku(endian = "endian", ctx = "endian: deku::ctx::Endian", ctx_default = "deku::ctx::Endian::Big")]
pub struct SaturnCollision {
    collision_type: CollisionType,
    #[deku(update = "self.to_bytes().unwrap().len()")]
    collision_size: u32,
    points_count:u32,
    points:Vec<(u8, u8)>
}

impl SaturnCollision { 

    fn new(collision_type: CollisionType, points: Vec<(u8, u8)>) -> Result<Self, String> {
        let count = points.len() as u32;
        Ok(SaturnCollision {
            collision_type,
            collision_size: Default::default(),
            points_count: count,
            points,
        })
    }

    pub fn build<'a>(map_width:u32, map_height:u32, layers: impl ExactSizeIterator<Item = Layer<'a>>) -> Result<Vec<Self>, String> {
        let mut empty = SaturnCollision::new( CollisionType::Empty, Vec::default())?;
        empty.update().map_err(|op| op.to_string())?; 
        
        let mut results: Vec<SaturnCollision> = vec![empty; (map_width * map_height) as usize];

        let tile_layers: Vec<(u32, TileLayer)> = layers.filter_map(|layer| match layer.layer_type() {
            tiled::LayerType::Tiles(tile_layer) => Some((layer.id(), tile_layer)),
            _ => None,
        }).collect();

        for tile_packed in tile_layers.iter() {
            let layer_id = tile_packed.0;
            let tile_layer = &tile_packed.1;
            let width = tile_layer.width().ok_or(format!("Unable to get width for layer {}", layer_id))?;
            let height = tile_layer.height().ok_or(format!("Unable to get height for layer {}", layer_id))?;

            for y in 0..height {
                for x in 0..width {
                    let layer_tile = tile_layer.get_tile(x as i32,y as i32);
                    let tile = layer_tile.map(|f| f.get_tile()).flatten();
                    if tile.is_some() {
                        let object_layer_tile = &tile.unwrap().collision;
                        if object_layer_tile.is_some() {
                            let od = object_layer_tile.clone().unwrap();
                            let obj_d = od.object_data().into_iter();
                            
                            for object_data in obj_d {
                                let rect = match object_data.shape {
                                    tiled::ObjectShape::Rect{ width, height} => Some((width, height)),
                                    _ => None
                                };
                                
                                if rect.is_some() {
                                    let (width, height) = rect.unwrap();
                                    let width_narrow = width.round() as u8;
                                    let height_narrow = height.round() as u8;
                                    let x_narrow = object_data.x.round() as u8;
                                    let y_narrow = object_data.y.round() as u8;
                                    let points = vec![(x_narrow, y_narrow),
                                                                     (width_narrow, y_narrow), 
                                                                     (width_narrow, height_narrow), 
                                                                     (x_narrow, height_narrow)];
                                    
                                    let mut solid = SaturnCollision::new(CollisionType::Rect, points)?;
                            
                                    solid.update().map_err(|op| op.to_string())?;
                                    results[((map_width*(y))+(x)) as usize] = solid;
                                }
                            }
                        }

                    }
                }
            }
        }

        return Ok(results);
    }
}