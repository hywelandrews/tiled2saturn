#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "tiled2saturn.h"

#define LONG(raw_bytes, position)  (uint32_t)(((BYTE(raw_bytes, position)) << 24) | ((BYTE(raw_bytes, position+1)) << 16) | ((BYTE(raw_bytes, position+2)) << 8) | (BYTE(raw_bytes, position+3)))
#define SHORT(raw_bytes, position) (uint16_t)((BYTE(raw_bytes, position)) << 8) | (BYTE(raw_bytes, position+1))
#define BYTE(raw_bytes, position)  (uint8_t)*(raw_bytes+(position))

/**
 * @brief Parse a byte stream to extract a Tiled2Saturn header.
 *
 * Parse a byte stream to extract the header information. The header contains various fields, 
 * including magic, version, width, height, tileset count, tileset offset, layer count, layer offset and collision offset. 
 * It performs validation checks on some fields and returns a dynamically allocated structure containing 
 * the parsed header data.
 *
 * @param raw_bytes Pointer to the byte stream containing the Tiled2Saturn header.
 *
 * @return A dynamically allocated `tiled2saturn_header_t` structure containing the parsed header.
 *         The caller is responsible for freeing this memory when it is no longer needed using `free()`.
 *
 * @note This function expects a well-formed byte stream with a specific structure, and it assumes
 *       the input adheres to the Tiled2Saturn file format. Malformed or incorrect data may lead to
 *       assertion failures or undefined behavior.
 *
 * @warning The caller must free the memory allocated for the parsed header structure to prevent memory leaks.
 */
static tiled2saturn_header_t* parse_header(uint8_t* bytes){

    tiled2saturn_header_t* header = (tiled2saturn_header_t*)malloc(sizeof(tiled2saturn_header_t));

    uint32_t magic = LONG(bytes, 0);  //4 0-3
    assert(magic == 0x894D4150);
    header->version = LONG(bytes, 4); //4 4-7 
    assert(header->version == 2);
    header->width = LONG(bytes, 8);   //4 8-11
    assert((header->width % 8) == 0);
    header->height = LONG(bytes, 12); //4 12-15
    assert((header->height % 8) == 0);
    header->tileset_count = BYTE(bytes, 16); //1 16
    assert(header->tileset_count > 0);
    header->tileset_offset = LONG(bytes, 17); //4 17-20
    assert(header->tileset_offset > 0);
    header->layer_count = BYTE(bytes, 21); //1 21
    assert(header->layer_count > 0);
    header->layer_offset = LONG(bytes, 22); //4 22-25
    assert(header->layer_offset > 0);
    header->collision_offset = LONG(bytes, 26); //4 26-29
    assert(header->collision_offset > 0);
    return header; 
}

/**
 * @brief Parse a tileset from a byte stream.
 *
 * Parse a tileset from a byte stream, extracting various properties of the tileset,
 * such as its size, dimensions, tile count, color depth, palette information, and character patterns.
 * It performs validation checks on some fields and returns a dynamically allocated `tiled2saturn_tileset_t`
 * structure containing the parsed tileset data.
 *
 * @param bytes Pointer to the byte stream containing the tileset data.
 * @param offset The offset in the byte stream where the tileset data begins.
 *
 * @return A dynamically allocated `tiled2saturn_tileset_t` structure containing the parsed tileset.
 *         The caller is responsible for freeing this memory when it is no longer needed using `free()`.
 *
 * @note This function expects a well-formed byte stream with a specific structure, and it assumes
 *       the input adheres to the Tiled2Saturn tileset format. Malformed or incorrect data may lead to
 *       assertion failures or undefined behavior.
 *
 * @warning The caller must free the memory allocated for the parsed tileset structure to prevent memory leaks.
 */
static tiled2saturn_tileset_t* parse_tileset(uint8_t* bytes, uint32_t offset){
    tiled2saturn_tileset_t* tileset = (tiled2saturn_tileset_t*)malloc(sizeof(tiled2saturn_tileset_t));
    tileset->tileset_size = LONG(bytes, offset); //4 25-28
    assert(tileset->tileset_size > 0);
    tileset->tile_width = LONG(bytes, offset + 4); //4 29-32
    assert(tileset->tile_width == 16);
    tileset->tile_height = LONG(bytes, offset + 8); //4 33-36
    assert(tileset->tile_height > 0);
    tileset->tile_count = LONG(bytes, offset + 12); //4 37-40
    assert(tileset->tile_count > 0);
    tileset->bpp = SHORT(bytes, offset + 16); //2 41-42
    assert(tileset->bpp == 4 || tileset->bpp == 8);
    tileset->words_per_palette = BYTE(bytes, offset + 18); //1 43
    assert(tileset->words_per_palette == 1 || tileset->words_per_palette == 2);
    tileset->number_of_colors = SHORT(bytes, offset + 19); //2 44-45
    assert(tileset->number_of_colors == 16 || tileset->number_of_colors == 256 || tileset->number_of_colors == 2048);

    tileset->palette_size = LONG(bytes, offset + 21); //4 44-47
    assert(tileset->palette_size > 0);
    tileset->palette = (uint8_t*)malloc(tileset->palette_size);
    memcpy(tileset->palette, bytes+offset+25, tileset->palette_size);

    tileset->character_pattern_size = LONG(bytes, tileset->palette_size+offset+25); //4 48-51
    assert(tileset->character_pattern_size > 0);
    tileset->character_pattern = (uint8_t*)malloc(tileset->character_pattern_size);
    memcpy(tileset->character_pattern, bytes+tileset->palette_size+offset+29, tileset->character_pattern_size);

    return tileset;
}

/**
 * @brief Parse a layer from a byte stream.
 *
 * This function parses a layer from a byte stream, extracting various properties of the layer,
 * including its ID, size, dimensions, and pattern name data. It performs validation checks on
 * some fields and returns a dynamically allocated `tiled2saturn_layer_t` structure containing the
 * parsed layer data.
 *
 * @param bytes Pointer to the byte stream containing the layer data.
 * @param offset The offset in the byte stream where the layer data begins.
 * @param tilesets The array of tilesets previously parsed in the byte stream.
 * 
 * @return A dynamically allocated `tiled2saturn_layer_t` structure containing the parsed layer.
 *         The caller is responsible for freeing this memory when it is no longer needed using `free()`.
 *
 * @note This function expects a well-formed byte stream with a specific structure, and it assumes
 *       the input adheres to the Tiled2Saturn layer format. Malformed or incorrect data may lead to
 *       assertion failures or undefined behavior.
 *
 * @warning The caller must free the memory allocated for the parsed layer structure to prevent memory leaks.
 *
 */
static tiled2saturn_layer_t* parse_layer(uint8_t* bytes, uint32_t offset, tiled2saturn_tileset_t** tilesets){
    tiled2saturn_layer_t* layer = (tiled2saturn_layer_t*)malloc(sizeof(tiled2saturn_layer_t));
    layer->id = LONG(bytes, offset); //4 52-55
    assert(layer->id != 0);
    layer->layer_size = LONG(bytes, offset+4); //4 52-55
    assert(layer->layer_size > 0);
    layer->layer_width = LONG(bytes, offset+8); //4 56-59
    assert(layer->layer_width > 0);
    layer->layer_height = LONG(bytes, offset+12); //4 60-63
    assert(layer->layer_height > 0);
    uint16_t tileset_index = SHORT(bytes, offset+16); //2 63-64

    layer->tile_flip_enabled = BYTE(bytes, offset+18); //1 65
    assert(layer->tile_flip_enabled < 2);
    layer->tile_transparency_enabled = BYTE(bytes, offset+19); //1 65
    assert(layer->tile_transparency_enabled < 2);
    
    layer->pattern_name_data_size = LONG(bytes, offset+20); //4 66-69
    assert(layer->pattern_name_data_size > 0);

    layer->pattern_name_data = (uint8_t*)malloc(layer->pattern_name_data_size);
    memcpy(layer->pattern_name_data, bytes+offset+24, layer->pattern_name_data_size);

    layer->tileset = tilesets[tileset_index];

    return layer;
}

/**
 * @brief Parse a collision set from a byte stream.
 *
 * The function begins by dynamically allocating memory for an array of pointers to tiled2saturn_collision_t structures, with the number of elements equal to the size parameter.
 *   It iterates over the byte array size times, parsing each collision and storing it in the allocated array.
 *   Each collision includes:
 *      collision_type: A byte value representing the type of collision.
 *      collision_size: A 32-bit integer representing the size of the collision data.
 *      point_count: A 32-bit integer representing the number of points in the collision.
 *      points: An array of pointers to tiled2saturn_point_t structures representing the points of the collision.
 *
 * @param bytes A pointer to an array of bytes representing the collision data.
 * @param offset: The starting position in the byte array from where the parsing should begin.
 * @param size: The number of collisions to parse from the byte array.
 *
 * @return Returns a pointer to an array of pointers to tiled2saturn_collision_t structures, 
 *         each representing a collision parsed from the byte array.
 *
 * @note The function is designed to be used in scenarios where collision data is stored in a compressed byte format and 
 *       needs to be parsed into a structured format for further processing or analysis.
 *
 * @warning It is the caller's responsibility to ensure that the allocated memory is properly freed to avoid memory leaks.
 */

static tiled2saturn_collision_t** parse_collision(uint8_t* bytes, uint32_t offset, uint32_t size){
    tiled2saturn_collision_t** collisions = (tiled2saturn_collision_t**)malloc(size * sizeof(tiled2saturn_collision_t*));
    uint32_t collision_position = 0;
    for(uint32_t i = 0; i<size; i++){
        tiled2saturn_collision_t* collision = (tiled2saturn_collision_t*)malloc(sizeof(tiled2saturn_collision_t));
        collision->collision_type = BYTE(bytes, collision_position+ offset); //1 1
        assert(collision->collision_type >= 0 && collision->collision_type <= 3 );
        collision->collision_size = LONG(bytes, collision_position + (offset+1)); // 4 2-5
        assert(collision->collision_size > 0);
        collision->point_count =  LONG(bytes, collision_position + (offset+5));   // 4 6-9
        assert(collision->point_count <= 256);
        collision->points = (tiled2saturn_point_t**)malloc(collision->point_count * sizeof(tiled2saturn_point_t*));
        uint32_t point_position = 0;
        for(uint8_t j = 0; j<collision->point_count;j++){
            tiled2saturn_point_t* point = (tiled2saturn_point_t*)malloc(sizeof(tiled2saturn_point_t));
            point->x = BYTE(bytes, collision_position + point_position + (offset+10)); // 1 10
            point->y = BYTE(bytes, collision_position + point_position + (offset+11)); // 1 11
            collision->points[j] = point;
            point_position+=2;
        }
        collisions[i] = collision;
        collision_position += collision->collision_size;
    }

    return collisions;
}

/**
 * @brief Parse a Tiled2Saturn map from a byte stream.
 *
 * This function parses a Tiled2Saturn map from a byte stream, including its header, tilesets, and layers.
 * It dynamically allocates memory for the map structure, tilesets, and layers and populates the structure with
 * the parsed data. The caller is responsible for freeing the memory when it is no longer needed using `tiled2saturn_free()`.
 *
 * @param bytes Pointer to the byte stream containing the Tiled2Saturn map data.
 *
 * @return A dynamically allocated `tiled2saturn_t` structure containing the parsed Tiled2Saturn map, including
 *         header, tilesets, and layers. The caller is responsible for freeing this memory when it is no longer
 *         needed using `free_tiled2saturn()`.
 *
 * @note This function expects a well-formed byte stream with a specific structure, and it assumes
 *       the input adheres to the Tiled2Saturn map format. Malformed or incorrect data may lead to
 *       assertion failures or undefined behavior.
 *
 * @warning The caller must free the memory allocated for the parsed map, including header, tilesets, and layers,
 *          to prevent memory leaks. Use `free_tiled2saturn(tiled2saturn_t*)` to properly deallocate all resources.
 */

tiled2saturn_t* tiled2saturn_parse(uint8_t* bytes) {
    tiled2saturn_t* saturn_map = (tiled2saturn_t*)malloc(sizeof(tiled2saturn_t));
    saturn_map->header = parse_header(bytes);
    
    size_t tileset_offset = saturn_map->header->tileset_offset;
    saturn_map->tilesets = (tiled2saturn_tileset_t**)malloc(sizeof(tiled2saturn_tileset_t*) * saturn_map->header->tileset_count);
    for(uint8_t i = 0; i<saturn_map->header->tileset_count; i++){
        saturn_map->tilesets[i] = parse_tileset(bytes, tileset_offset);
        tileset_offset += saturn_map->tilesets[i]->tileset_size;
    }

    size_t layer_offset = saturn_map->header->layer_offset;
    saturn_map->layers = (tiled2saturn_layer_t**)malloc(sizeof(tiled2saturn_layer_t*) * saturn_map->header->layer_count);
    for(uint8_t i = 0; i<saturn_map->header->layer_count; i++){
        saturn_map->layers[i] = parse_layer(bytes, layer_offset, saturn_map->tilesets);
        layer_offset += saturn_map->layers[i]->layer_size;
    }

    size_t collision_offset = saturn_map->header->collision_offset;
    uint32_t count = saturn_map->header->width * saturn_map->header->height;
    saturn_map->collisions =  parse_collision(bytes, collision_offset, count);

    return saturn_map;
}

/**
 * @brief Free the memory allocated for a Tiled2Saturn map and its components.
 *
 * This function deallocates the memory associated with a parsed Tiled2Saturn map, including its header, tilesets,
 * layers, and their respective components. It ensures proper cleanup of dynamically allocated memory to prevent memory leaks.
 *
 * @param tiled2saturn Pointer to the `tiled2saturn_t` structure to be deallocated.
 *
 * @warning This function is responsible for releasing all memory allocated for the Tiled2Saturn map, its header, tilesets,
 *          layers, and their components. Calling this function is crucial to avoid memory leaks once the map is no longer needed.
 *
 */
void tiled2saturn_free(tiled2saturn_t* tiled2saturn){
    for (uint32_t i = 0; i < (tiled2saturn->header->width*tiled2saturn->header->height); i++) {
        for(uint8_t j = 0; j<tiled2saturn->collisions[i]->point_count; j++){
            free(tiled2saturn->collisions[i]->points[j]);
        }
        
        free(tiled2saturn->collisions[i]);
    }

    for (uint8_t i = 0; i < tiled2saturn->header->layer_count; i++) {
        free(tiled2saturn->layers[i]->pattern_name_data);
        free(tiled2saturn->layers[i]);
    }

    for (uint8_t i = 0; i < tiled2saturn->header->tileset_count; i++) {
        free(tiled2saturn->tilesets[i]->palette);
        free(tiled2saturn->tilesets[i]->character_pattern);
        free(tiled2saturn->tilesets[i]);
    }

    free(tiled2saturn->header);
    free(tiled2saturn);
}

/**
 * @brief Retrieve a Tiled2Saturn layer by its ID.
 *
 * This function searches for a Tiled2Saturn layer with the specified ID within a Tiled2Saturn map and returns
 * a pointer to the layer if found. If no layer with the specified ID is found, it returns NULL.
 *
 * @param self Pointer to the `tiled2saturn_t` structure representing the Tiled2Saturn map to search within.
 * @param id The ID of the layer to retrieve.
 *
 * @return A pointer to the `tiled2saturn_layer_t` structure representing the found layer, or NULL if the layer
 *         with the specified ID was not found.
 *
 * @note This function assumes that the input Tiled2Saturn map structure (`tiled2saturn_t`) is valid and contains
 *       a valid array of layers. If the map is not properly initialized, using this function may result in
 *       undefined behavior.
 */
tiled2saturn_layer_t* get_layer_by_id(tiled2saturn_t* self, uint32_t id){
    for(uint8_t i = 0; i < self->header->layer_count; i++){
        if(self->layers[i]->id == id){
            return self->layers[i];
        }
    }

    return NULL;
}