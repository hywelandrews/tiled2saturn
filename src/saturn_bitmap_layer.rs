use std::fs;

use deku::prelude::*;
use embedded_graphics::pixelcolor::{Rgb888, RgbColor, Rgb555, Bgr888, Bgr555};
use tiled::{ImageLayer, Layer};
use tinybmp::{RawBmp, Bmp};

#[repr(C)]
#[derive(Debug, PartialEq, DekuWrite)]
#[deku(endian = "endian", ctx = "endian: deku::ctx::Endian", ctx_default = "deku::ctx::Endian::Big")]
pub struct SaturnBitmapLayer{
    id: u32,
    #[deku(update = "self.to_bytes().unwrap().len()")]
    pub layer_size: u32,
    width: u32,
    height: u32,
    #[deku(update = "self.bitmap.len()")]
    bitmap_size: u32,
    #[deku(count = "bitmap_size", endian = "big")]
    bitmap:Vec<u8>
}

impl SaturnBitmapLayer {
    fn new(id: u32, width: u32, height: u32, bitmap: Vec<u8>) -> Result<Self, String> {
        Ok(SaturnBitmapLayer {
            id,
            layer_size: Default::default(),
            width,
            height,
            bitmap_size: Default::default(),
            bitmap,
        })
    }

    pub fn build<'a>(layers: impl ExactSizeIterator<Item = Layer<'a>>) -> Result<Vec<Self>, String> {
        let mut results: Vec<SaturnBitmapLayer> = Vec::default();

        let bitmap_layers: Vec<(u32, ImageLayer)> = layers.filter_map(|layer| match layer.layer_type() {
            tiled::LayerType::Image(image_layer) => Some((layer.id(), image_layer)),
            _ => None,
        }).collect();

        for (id, image_layer) in bitmap_layers.iter() {
            let image = image_layer.image.as_ref();
            let width = image.map(|i| i.width).filter(|i| *i == 512 || *i == 1024).ok_or(format!("Unable to get valid width for layer {}", id))?;
            let height = image.map(|i| i.height).filter(|i| *i == 256 || *i == 512).ok_or(format!("Unable to get valid height for layer {}", id))?;
            let source = image.map(|i| i.source.clone()).ok_or(format!("Unable to get source for layer {}", id))?;
            let image_file = fs::read(source.as_path()).map_err(|op| op.to_string() + " " + source.as_path().to_str().unwrap())?;
            //let raw_bmp = RawBmp::from_slice(&image_file).map_err(|op| format!("{:?}", op))?;
            let bmp = Bmp::<Bgr888>::from_slice(&image_file).map_err(|op| format!("{:?}", op))?;

            let iter = bmp.pixels().into_iter();
            let mut bitmap_results: Vec<u16> = Vec::default();
            for pixel in iter {
                let color = pixel.1;
                let r = (color.r() & 0xff).overflowing_shr(3);
                let g = ((color.g().overflowing_shr(8).0) & 0xff).overflowing_shr(3);
                let b = ((color.b().overflowing_shr(16).0) & 0xff).overflowing_shr(3);
                bitmap_results.push(u16::from(r.0) | (u16::from(g.0) << 5) | (u16::from(b.0) << 10) | 0x8000);
            }

           let mut saturn_bitmap_layer = SaturnBitmapLayer::new(*id, width as u32, height as u32, bitmap_results.iter().flat_map(|val| val.to_be_bytes()).collect())?;
            // let mut saturn_bitmap_layer = SaturnBitmapLayer::new(*id, width as u32, height as u32, bmp.as_raw().image_data().to_vec())?;

            saturn_bitmap_layer.update().map_err(|op| op.to_string())?;
            results.push(saturn_bitmap_layer);
        }

        return Ok(results);
    }
}