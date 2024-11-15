#include "gamemath/defs.h"
#include "gamemath/fix16.h"
#include "gamemath/fix16/fix16_vec2.h"
#include "vdp2/scrn_macros.h"
#include "vdp2/scrn_shared.h"
#include <stdint.h>
#include <yaul.h>
#include <assert.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>

#include <tiled2saturn/tiled2saturn.h>

#define NBG0_BMP         VDP2_VRAM_ADDR(1, 0x000000)
#define NBG1_CPD         VDP2_VRAM_ADDR(0, 0x000000)
#define NBG1_MAP         VDP2_VRAM_ADDR(0, 0x008000)
#define NBGX_MAP_EMPTY   VDP2_VRAM_ADDR(0, 0x008800)
#define NBG1_PAL         VDP2_CRAM_MODE_1_OFFSET(0, 0, 0)
#define BACK_SCREEN      VDP2_VRAM_ADDR(3, 0x01FFFE)


#define VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX  (0)
#define VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX        (1)
#define VDP1_CMDT_ORDER_BALL_START_INDEX          (2)


#define VDP1_VRAM_CMDT_COUNT    (1 + 3)
#define VDP1_VRAM_TEXTURE_SIZE  (0x0005BF60)
#define VDP1_VRAM_GOURAUD_COUNT (0)
#define VDP1_VRAM_CLUT_COUNT    (0)

extern uint8_t layer1[];
extern uint8_t layer1_end[];

static void _vblank_out_handler(void *work);

static smpc_peripheral_digital_t _digital;

static vdp1_vram_t _sprite_tex_base;
static vdp2_cram_t _sprite_pal_base;

extern uint8_t asset_ball_tex[];
extern uint8_t asset_ball_tex_end[];
extern uint8_t asset_ball_pal[];
extern uint8_t asset_ball_pal_end[];

typedef struct ball {
        uint16_t* pos_tile_x;
        uint16_t* pos_tile_y;
        fix16_t* pos_x;
        fix16_t* pos_y;
        int16_t* cmd_xa;
        int16_t* cmd_ya;
} ball_t;

static uint16_t _balls_tile_pos_x[1] __aligned(0x1000);
static uint16_t _balls_tile_pos_y[1] __aligned(0x1000);

static fix16_t _balls_pos_x[1] __aligned(0x1000);
static fix16_t _balls_pos_y[1] __aligned(0x1000);

static int16_t _balls_cmd_xa[2][1] __aligned(0x1000);
static int16_t _balls_cmd_ya[2][1] __aligned(0x1000);

const uint16_t resolution_width = 320;
const uint16_t resolution_height = 224;

const uint16_t ball_size = 16;
const uint16_t ball_size_half = ball_size/2;
const uint32_t ball_speed = 0x1;


typedef struct collision {
  bool collides;
  bool top;
  bool bottom;
  bool left;
  bool right;
} collision_t;

collision_t** tiled2saturn_collisions_convertor(tiled2saturn_collision_t** tiled2saturn_collisions, uint32_t number_of_collisions){
        collision_t** collisions = (collision_t**)malloc(number_of_collisions * sizeof(collision_t*));
        const uint8_t tile_row_size = 32;
        for(uint32_t i = 0; i<number_of_collisions; i++){
                collision_t* collision = (collision_t*)malloc(sizeof(collision_t));
                collision->collides = tiled2saturn_collisions[i]->collision_type != (tiled2saturn_collision_type_t)EMPTY;
                collision->top = (i >= tile_row_size ? tiled2saturn_collisions[i-tile_row_size]->collision_type == (tiled2saturn_collision_type_t)EMPTY : false);
                collision->bottom = (i < (number_of_collisions - tile_row_size) ? tiled2saturn_collisions[i+tile_row_size]->collision_type == (tiled2saturn_collision_type_t)EMPTY : false);
                collision->left = (i > 0 ? tiled2saturn_collisions[i-1]->collision_type == (tiled2saturn_collision_type_t)EMPTY : false);
                collision->right = (i < number_of_collisions ? tiled2saturn_collisions[i+1]->collision_type == (tiled2saturn_collision_type_t)EMPTY : false);
                collisions[i] = collision;
        }
        return collisions;
}

void balls_assets_init()
{
        vdp1_vram_partitions_t vdp1_vram_partitions;

        vdp1_vram_partitions_get(&vdp1_vram_partitions);

        _sprite_tex_base = (vdp1_vram_t)vdp1_vram_partitions.texture_base;
        _sprite_pal_base = VDP2_CRAM_MODE_1_OFFSET(1, 0, 0x0000);
}

void balls_assets_load()
{
        scu_dma_transfer(0, (void *)_sprite_tex_base, asset_ball_tex, asset_ball_tex_end - asset_ball_tex);
        scu_dma_transfer(0, (void *)_sprite_pal_base, asset_ball_pal, asset_ball_pal_end - asset_ball_pal);
}

void balls_cmdts_put(uint16_t index)
{
        const vdp1_cmdt_draw_mode_t draw_mode = {
                .color_mode           = 0,
                .trans_pixel_disable  = false,
                .pre_clipping_disable = true,
                .end_code_disable     = false
        };

        vdp1_cmdt_t *cmdt;
        cmdt = (vdp1_cmdt_t *)VDP1_CMD_TABLE(index, 0);

        vdp1_cmdt_normal_sprite_set(cmdt);

        vdp1_cmdt_draw_mode_set(cmdt, draw_mode);

        const uint32_t rand_index = rand() & 15;
        const uint16_t palette_offset =
                (_sprite_pal_base + (rand_index << 4)) & (VDP2_CRAM_SIZE - 1);
        const uint16_t palette_number = palette_offset >> 1;

        const vdp1_cmdt_color_bank_t color_bank ={
                .type_0.dc = palette_number & VDP2_SPRITE_TYPE_0_DC_MASK
        };

        vdp1_cmdt_color_mode0_set(cmdt, color_bank);
        vdp1_cmdt_char_size_set(cmdt, ball_size, ball_size);
        vdp1_cmdt_char_base_set(cmdt, _sprite_tex_base);

        cmdt->cmd_xa = 0;
        cmdt->cmd_ya = 0;

        vdp1_cmdt_end_set(cmdt);
}

static inline fix16_t _ball_position_update(fix16_t pos, fix16_t speed)
{
        /* Map bit 0 as direction:
         *   dir_bit: 0 -> ((0-1)^0xFF)=0x00 -> positive
         *   dir_bit: 1 -> ((1-1)^0xFF)=0xFF -> negative */
        const int8_t dir_bit = pos & 0x0001;
        const uint8_t d = (dir_bit - 1) ^ 0xFF;
        const fix16_t fixed_dir = FIX16(((int8_t)d ^ speed));

        return (pos + fixed_dir) | dir_bit;
}

void balls_position_update(const ball_t * balls, uint32_t speed)
{
        fix16_t* pos_x = balls->pos_x;
        fix16_t* pos_y = balls->pos_y;

        *pos_x = _ball_position_update(*pos_x, speed);
        *pos_y = _ball_position_update(*pos_y, speed);
}

void balls_collision_update(const ball_t * balls, uint32_t speed, collision_t** collisions)
{
        fix16_t* pos_x = balls->pos_x;
        fix16_t* pos_y = balls->pos_y;
        uint16_t previous_tile_pos_x = *balls->pos_tile_x;
        uint16_t previous_tile_pos_y = *balls->pos_tile_y;

        const fix16_t  horizontal_clamp  = FIX16(resolution_width/2.0);
        const fix16_t  vertical_clamp    = FIX16(resolution_height/2.0);
        const uint8_t  tile_size         = 16;
        const fix16_t  adjusted_x        = *pos_x + horizontal_clamp;
        const fix16_t  adjusted_y        = *pos_y + vertical_clamp;
        const uint16_t tile_pos_x        = fix16_int32_to(adjusted_x)/tile_size;
        const uint16_t tile_pos_y        = fix16_int32_to(adjusted_y)/tile_size;
        
        collision_t* component_collision = collisions[(tile_pos_y*32)+tile_pos_x];   

        if (component_collision->collides){
                bool collision_on_boundary_top = previous_tile_pos_y < tile_pos_y;
                bool collision_on_boundary_bottom = previous_tile_pos_y > tile_pos_y;
                bool collision_on_boundary_left = previous_tile_pos_x  < tile_pos_x;
                bool collision_on_boundary_right = previous_tile_pos_x > tile_pos_x;

                if(collision_on_boundary_top && component_collision->top){
                        *pos_y = (*pos_y - speed) | 0x0001;   
                }else if (collision_on_boundary_bottom && component_collision->bottom){
                         *pos_y = (*pos_y + speed) & ~0x0001;
                }
                
                if(collision_on_boundary_left && component_collision->left){
                        *pos_x = (*pos_x - speed) | 0x0001;
                }else if (collision_on_boundary_right && (component_collision->right)){
                        *pos_x = (*pos_x + speed) & ~0x0001;
                }

                if(!component_collision->top && !component_collision->right && 
                    collision_on_boundary_top && collision_on_boundary_right){
                        *pos_y = (*pos_y - speed) | 0x0001; 
                        *pos_x = (*pos_x + speed) & ~0x0001;
                }else if(!component_collision->bottom && !component_collision->left && 
                          collision_on_boundary_bottom && collision_on_boundary_left){
                        *pos_x = (*pos_x - speed) | 0x0001;
                        *pos_y = (*pos_y + speed) & ~0x0001;

                }

               if(!component_collision->top && !component_collision->left && 
                   collision_on_boundary_top && collision_on_boundary_left){
                        *pos_y = (*pos_y + speed) & ~0x0001;
                        *pos_x = (*pos_x + speed) & ~0x0001;
                }else if(!component_collision->bottom && !component_collision->right && 
                          collision_on_boundary_bottom && collision_on_boundary_right){
                        *pos_x = (*pos_x - speed) | 0x0001;
                        *pos_y = (*pos_y - speed) | 0x0001; 
                }
        }

        *balls->pos_tile_x = tile_pos_x;
        *balls->pos_tile_y = tile_pos_y;
}

void balls_cmdts_update(const ball_t * balls)
{
        fix16_t* pos_x = balls->pos_x;
        fix16_t* pos_y = balls->pos_y;

        int16_t* cmd_xa = balls->cmd_xa;
        int16_t* cmd_ya = balls->cmd_ya;

        *cmd_xa = fix16_int32_to(*pos_x);
        *cmd_ya = fix16_int32_to(*pos_y);
}

void balls_cmdts_position_put(const ball_t *balls, uint16_t index, uint16_t count)
{
        vdp1_sync_cmdt_stride_put(balls->cmd_xa,
            count,
            6, /* CMDXA */
            index);

        vdp1_sync_cmdt_stride_put(balls->cmd_ya,
            count,
            7, /* CMDYA */
            index);
}

int main(void)
{
        const vdp2_scrn_bitmap_format_t format_nbg0 = {
                .scroll_screen = VDP2_SCRN_NBG0,
                .ccc           = VDP2_SCRN_CCC_RGB_32768,
                .bitmap_size   = VDP2_SCRN_BITMAP_SIZE_512X256,
                .palette_base  = 0x00000000,
                .bitmap_base   = NBG0_BMP
        };

        const vdp2_scrn_cell_format_t format_nbg1 = {
                .scroll_screen = VDP2_SCRN_NBG1,
                .ccc           = VDP2_SCRN_CCC_PALETTE_16,
                .char_size     = VDP2_SCRN_CHAR_SIZE_2X2,
                .pnd_size      = 1,
                .aux_mode      = VDP2_SCRN_AUX_MODE_0,
                .plane_size    = VDP2_SCRN_PLANE_SIZE_1X1,
                .cpd_base      = NBG1_CPD,
                .palette_base  = NBG1_PAL
        };

        const vdp2_scrn_normal_map_t nbg1_normal_map = {
                .plane_a = NBG1_MAP,
                .plane_b = NBGX_MAP_EMPTY,
                .plane_c = NBGX_MAP_EMPTY,
                .plane_d = NBGX_MAP_EMPTY
        };

        const vdp2_vram_cycp_t vram_cycp = {
                .pt[0].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG1,
                .pt[0].t1 = VDP2_VRAM_CYCP_PNDR_NBG1,
                .pt[0].t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

                .pt[1].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[1].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

                .pt[2].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[2].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

                .pt[3].t0 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t1 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[3].t7 = VDP2_VRAM_CYCP_NO_ACCESS
        };

        const ball_t balls[] = {
                {
                        .pos_tile_x = _balls_tile_pos_x,
                        .pos_tile_y = _balls_tile_pos_y,
                        .pos_x  = _balls_pos_x,
                        .pos_y  = _balls_pos_y,
                        .cmd_xa = _balls_cmd_xa[0],
                        .cmd_ya = _balls_cmd_ya[0]
                }
        };

        /* XXX: Seed properly */
        srand(0xBEEFCAFE);

        vdp2_vram_cycp_set(&vram_cycp);

        vdp2_scrn_bitmap_format_set(&format_nbg0);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 5);

        tiled2saturn_t* t2s = tiled2saturn_parse(layer1);

        tiled2saturn_bitmap_layer_t* bitmap_layer = get_bitmap_layer_by_id(t2s, 1);

        scu_dma_transfer(0, (void *)NBG0_BMP, bitmap_layer->bitmap, bitmap_layer->bitmap_size);

        tiled2saturn_layer_t* tileset_layer = get_layer_by_id(t2s, 2);

        scu_dma_transfer(0, (void *)NBG1_PAL, tileset_layer->tileset->palette, tileset_layer->tileset->palette_size);
        scu_dma_transfer(0, (void *)NBG1_MAP, tileset_layer->pattern_name_data, tileset_layer->pattern_name_data_size);
        scu_dma_transfer(0, (void *)NBG1_CPD, tileset_layer->tileset->character_pattern, tileset_layer->tileset->character_pattern_size);
        
        vdp2_scrn_cell_format_set(&format_nbg1, &nbg1_normal_map);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG1, 6);
        
        vdp2_scrn_display_set(  VDP2_SCRN_DISP_NBG0 | VDP2_SCRN_DISPTP_NBG1);

        vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG0, FIX16(0));
        vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG0, FIX16(0));

        collision_t** collisions = tiled2saturn_collisions_convertor(t2s->collisions, (t2s->header->width * t2s->header->height));

        vdp2_sync();
        vdp2_sync_wait();

        vdp2_tvmd_display_set();

        balls_assets_init();
        balls_assets_load();
        balls_cmdts_put(VDP1_CMDT_ORDER_BALL_START_INDEX);

         while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                balls_position_update(balls, ball_speed);
                balls_collision_update(balls, ball_speed, collisions);
                balls_cmdts_update(balls);
                balls_cmdts_position_put(balls, VDP1_CMDT_ORDER_BALL_START_INDEX, 1);
                vdp1_cmdt_end_clear((vdp1_cmdt_t *)VDP1_CMD_TABLE(VDP1_CMDT_ORDER_BALL_START_INDEX, 0));
                vdp1_cmdt_end_set( (vdp1_cmdt_t *)VDP1_CMD_TABLE(VDP1_CMDT_ORDER_BALL_START_INDEX + 1, 0));

                /* Call to render */
                vdp1_sync_render();
                /* Call to sync with frame change -- does not block */
                vdp1_sync();
                vdp2_sync();
                vdp2_sync_wait();
        }

}

void user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_color_set(BACK_SCREEN, RGB1555(1, 0, 3, 15));

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        smpc_peripheral_init();

        const struct vdp1_env vdp1_env = {
                .bpp          = VDP1_ENV_BPP_16,
                .rotation     = VDP1_ENV_ROTATION_0,
                .color_mode   = VDP1_ENV_COLOR_MODE_PALETTE,
                .sprite_type  = 0,
                .erase_color  = RGB1555(0, 0, 0, 0),
                .erase_points = {
                        {                0,                 0 },
                        { resolution_width, resolution_height }
                }
        };

        const int16_vec2_t local_coords =
            INT16_VEC2_INITIALIZER((resolution_width / 2) - ball_size_half - 1,
                                   (resolution_height / 2) - ball_size_half - 1);

        const int16_vec2_t system_clip_coords =
            INT16_VEC2_INITIALIZER(resolution_width,
                                   resolution_height);

        vdp1_cmdt_t * const cmdt = (vdp1_cmdt_t *)VDP1_CMD_TABLE(0, 0);

        vdp1_cmdt_system_clip_coord_set(&cmdt[VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX]);
        vdp1_cmdt_vtx_system_clip_coord_set(&cmdt[VDP1_CMDT_ORDER_SYSTEM_CLIP_COORDS_INDEX],
            system_clip_coords);

        vdp1_cmdt_local_coord_set(&cmdt[VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX]);
        vdp1_cmdt_vtx_local_coord_set(&cmdt[VDP1_CMDT_ORDER_LOCAL_COORDS_INDEX],
            local_coords);

        vdp1_env_set(&vdp1_env);

        vdp1_vram_partitions_set(VDP1_VRAM_CMDT_COUNT,
            VDP1_VRAM_TEXTURE_SIZE,
            VDP1_VRAM_GOURAUD_COUNT,
            VDP1_VRAM_CLUT_COUNT);

        vdp1_sync_interval_set(0);

        vdp2_sprite_priority_set(0, 6);
}

static void _vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}