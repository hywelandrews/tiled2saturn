use embedded_graphics::{
    pixelcolor::{raw::RawU24, Rgb888},
    prelude::*,
};

/// Color table.
///
/// This struct provides access to the color table.
#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct SaturnColorTable {
    data: Vec<u32>,
}

impl<'a> SaturnColorTable {
    pub const fn new(data: Vec<u32>) -> Self {
        Self { data }
    }

    /// Returns the number of entries.
    pub fn len(&self) -> usize {
        self.data.len()
    }

    pub fn contains(&self, value:u32) -> bool {
        self.data.contains(&value)
    }

    /// Returns a color table entry.
    ///
    /// `None` is returned if `index` is out of bounds.
    pub fn get(&self, index: u32) -> Option<Rgb888> {
        let offset = index as usize;
        let raw = self.data.get(offset)?;

        Some(RawU24::from_u32(*raw).into())
    }
}
