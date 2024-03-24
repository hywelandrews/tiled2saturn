#pragma once

#include <stdint.h>

typedef struct tiled2saturn_header {
    uint32_t version;
    uint32_t width;
    uint32_t height;
    uint8_t tileset_count;
    size_t tileset_offset;
    uint8_t layer_count;
    size_t layer_offset;
    size_t collision_offset;
} tiled2saturn_header_t;

typedef struct tiled2saturn_tileset {
    uint32_t tileset_size;
    uint32_t tile_width;
    uint32_t tile_height;
    uint32_t tile_count;
    uint16_t bpp;
    uint8_t  words_per_palette;
    uint16_t number_of_colors;
    uint32_t palette_size;
    uint8_t* palette;
    uint32_t character_pattern_size;
    uint8_t* character_pattern;
} tiled2saturn_tileset_t;

typedef struct tiled2saturn_layer {
    uint32_t id;
    uint32_t layer_size;
    uint32_t layer_width;
    uint32_t layer_height;
    uint8_t  tile_flip_enabled;
    uint8_t  tile_transparency_enabled;
    uint32_t pattern_name_data_size;
    uint8_t* pattern_name_data;
    tiled2saturn_tileset_t* tileset;
} tiled2saturn_layer_t;

typedef struct tiled2saturn_point{
    uint8_t x, y;
} tiled2saturn_point_t;

typedef enum {
    EMPTY = 0,
    RECT = 1,
    POLY = 2
} tiled2saturn_collision_type_t;

typedef struct tiled2saturn_collision{
    tiled2saturn_collision_type_t collision_type;
    uint32_t collision_size;
    tiled2saturn_point_t** points;
    uint32_t point_count;
} tiled2saturn_collision_t;

typedef struct tiled2saturn {
    tiled2saturn_header_t*      header;
    tiled2saturn_tileset_t**    tilesets;
    tiled2saturn_layer_t**      layers;
    tiled2saturn_collision_t**  collisions;
} tiled2saturn_t;

tiled2saturn_t* tiled2saturn_parse(uint8_t* raw_bytes);
void tiled2saturn_free(tiled2saturn_t* tiled2saturn);
tiled2saturn_layer_t* get_layer_by_id(tiled2saturn_t* self, uint32_t id);