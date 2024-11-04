#include "gamemath/defs.h"
#include "gamemath/fix16.h"
#include "gamemath/fix16/fix16_vec2.h"
#include "vdp2/cram.h"
#include "vdp2/scrn_cell.h"
#include "vdp2/scrn_macros.h"
#include "vdp2/scrn_shared.h"
#include <stdint.h>
#include <yaul.h>

#include <assert.h>
#include <stdbool.h>

#include <tiled2saturn/tiled2saturn.h>

#define NBG0_CPD         VDP2_VRAM_ADDR(0, 0x000000)
#define NBG0_PAL         VDP2_CRAM_MODE_1_OFFSET(0, 0, 0)

#define NBG1_CPD         VDP2_VRAM_ADDR(1, 0x000000)
#define NBG1_PAL         VDP2_CRAM_MODE_1_OFFSET(0, 1, 0)

#define NBG2_CPD         VDP2_VRAM_ADDR(1, 0x001200)
#define NBG2_PAL         VDP2_CRAM_MODE_1_OFFSET(0, 2, 0)

#define NBG0_MAP         VDP2_VRAM_ADDR(2, 0x000000)
#define NBG1_MAP         VDP2_VRAM_ADDR(2, 0x000800)
#define NBG2_MAP         VDP2_VRAM_ADDR(2, 0x001000)
#define NBGX_MAP_EMPTY   VDP2_VRAM_ADDR(2, 0x001800)

#define BACK_SCREEN      VDP2_VRAM_ADDR(3, 0x01FFFE)

extern uint8_t layers[];
extern uint8_t layers_end[];

static void _vblank_out_handler(void *work);

static smpc_peripheral_digital_t _digital;

int main(void)
{
        vdp2_scrn_cell_format_t nbg0_format = {
                .scroll_screen = VDP2_SCRN_NBG0,
                .ccc           = VDP2_SCRN_CCC_PALETTE_16,
                .char_size     = VDP2_SCRN_CHAR_SIZE_2X2,
                .pnd_size      = 1,
                .aux_mode      = VDP2_SCRN_AUX_MODE_1,
                .plane_size    = VDP2_SCRN_PLANE_SIZE_1X1,
                .cpd_base      = NBG0_CPD,
                .palette_base  = NBG0_PAL
        };

        const vdp2_scrn_normal_map_t nbg0_normal_map = {
                .plane_a = NBG0_MAP,
                .plane_b = NBGX_MAP_EMPTY,
                .plane_c = NBGX_MAP_EMPTY,
                .plane_d = NBGX_MAP_EMPTY
        };

        vdp2_scrn_cell_format_t nbg1_format = {
                .scroll_screen = VDP2_SCRN_NBG1,
                .ccc           = VDP2_SCRN_CCC_PALETTE_16,
                .char_size     = VDP2_SCRN_CHAR_SIZE_2X2,
                .pnd_size      = 1,
                .aux_mode      = VDP2_SCRN_AUX_MODE_1,
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

        vdp2_scrn_cell_format_t nbg2_format = {
                .scroll_screen = VDP2_SCRN_NBG2,
                .ccc           = VDP2_SCRN_CCC_PALETTE_16,
                .char_size     = VDP2_SCRN_CHAR_SIZE_2X2,
                .pnd_size      = 1,
                .aux_mode      = VDP2_SCRN_AUX_MODE_1,
                .plane_size    = VDP2_SCRN_PLANE_SIZE_1X1,
                .cpd_base      = NBG2_CPD,
                .palette_base  = NBG2_PAL
        };

        const vdp2_scrn_normal_map_t nbg2_normal_map = {
                .plane_a = NBG2_MAP,
                .plane_b = NBGX_MAP_EMPTY,
                .plane_c = NBGX_MAP_EMPTY,
                .plane_d = NBGX_MAP_EMPTY
        };

         const vdp2_vram_cycp_t vram_cycp = {
                .pt[0].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t1 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[0].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

                .pt[1].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG1,
                .pt[1].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG2,
                .pt[1].t2 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t3 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t4 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t5 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t6 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[1].t7 = VDP2_VRAM_CYCP_NO_ACCESS,

                .pt[2].t0 = VDP2_VRAM_CYCP_PNDR_NBG0,
                .pt[2].t1 = VDP2_VRAM_CYCP_PNDR_NBG1,
                .pt[2].t2 = VDP2_VRAM_CYCP_PNDR_NBG2,
                .pt[2].t3 = VDP2_VRAM_CYCP_NO_ACCESS,
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

        vdp2_vram_cycp_set(&vram_cycp);

        tiled2saturn_t* t2s = tiled2saturn_parse(layers);

        tiled2saturn_layer_t* t2s_layer_1 =  get_layer_by_id(t2s,1);

        tiled2saturn_layer_t* t2s_layer_2 =  get_layer_by_id(t2s,2);

        tiled2saturn_layer_t* t2s_layer_3 =  get_layer_by_id(t2s,3);
        
        scu_dma_transfer(0, (void *)NBG0_PAL, t2s_layer_1->tileset->palette, t2s_layer_1->tileset->palette_size);
        scu_dma_transfer(0, (void *)NBG0_MAP, t2s_layer_1->pattern_name_data, t2s_layer_1->pattern_name_data_size);
        scu_dma_transfer(0, (void *)NBG0_CPD, t2s_layer_1->tileset->character_pattern, t2s_layer_1->tileset->character_pattern_size);
        
        vdp2_scrn_cell_format_set(&nbg0_format, &nbg0_normal_map);

        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 5);

        scu_dma_transfer(0, (void *)NBG1_PAL, t2s_layer_2->tileset->palette, t2s_layer_2->tileset->palette_size);
        scu_dma_transfer(0, (void *)NBG1_MAP, t2s_layer_2->pattern_name_data, t2s_layer_2->pattern_name_data_size);
        scu_dma_transfer(0, (void *)NBG1_CPD, t2s_layer_2->tileset->character_pattern, t2s_layer_2->tileset->character_pattern_size);

        vdp2_scrn_cell_format_set(&nbg1_format, &nbg1_normal_map);

        vdp2_scrn_priority_set(VDP2_SCRN_NBG1, 6);

        scu_dma_transfer(0, (void *)NBG2_PAL, t2s_layer_3->tileset->palette, t2s_layer_3->tileset->palette_size);
        scu_dma_transfer(0, (void *)NBG2_MAP, t2s_layer_3->pattern_name_data, t2s_layer_3->pattern_name_data_size);
        scu_dma_transfer(0, (void *)NBG2_CPD, t2s_layer_3->tileset->character_pattern, t2s_layer_3->tileset->character_pattern_size);

        vdp2_scrn_cell_format_set(&nbg2_format, &nbg2_normal_map);

        vdp2_scrn_priority_set(VDP2_SCRN_NBG2, 7);

        vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG0, FIX16(320.0f/2));
        vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG0, FIX16(224.0f/2));

        vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG1, FIX16(320.0f/2));
        vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG1, FIX16(224.0f/2));


        vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG2, FIX16(320.0f/2));
        vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG2, FIX16(224.0f/2));

        vdp2_scrn_disp_t disp_mask;
        disp_mask = VDP2_SCRN_DISP_NBG0 | VDP2_SCRN_DISPTP_NBG1 | VDP2_SCRN_DISPTP_NBG2;
        vdp2_scrn_display_set(disp_mask);

        vdp2_sync();
        vdp2_sync_wait();

        vdp2_tvmd_display_set();

        fix16_vec2_t pos = FIX16_VEC2_INITIALIZER(0.0f, 0.0f);

        while (true) {
                smpc_peripheral_process();
                smpc_peripheral_digital_port(1, &_digital);

                if (_digital.held.button.start) {
                        pos.x = FIX16(0.0f);
                        pos.y = FIX16(0.0f);
                } else {

                        if (_digital.pressed.button.up) {
                                pos.y += FIX16(-0.5f);
                        } else if (_digital.pressed.button.down) {
                                pos.y += FIX16( 0.5f);
                        }

                        if (_digital.pressed.button.left) {
                                pos.x += FIX16(-0.5f);
                        } else if (_digital.pressed.button.right) {
                                pos.x += FIX16( 0.5f);
                        }

                        pos.x = clamp(pos.x, FIX16(-4.0f), FIX16(4.0f));
                        pos.y = clamp(pos.y, FIX16(-4.0f), FIX16(4.0f));
                }

                const fix16_vec2_t clamped_pos = {
                        .x = pos.x & 0xFFFF0000,
                        .y = pos.y & 0xFFFF0000
                };

                vdp2_scrn_scroll_x_update(VDP2_SCRN_NBG0, clamped_pos.x);
                vdp2_scrn_scroll_y_update(VDP2_SCRN_NBG0, clamped_pos.y);

                vdp2_scrn_scroll_x_update(VDP2_SCRN_NBG1, clamped_pos.x);
                vdp2_scrn_scroll_y_update(VDP2_SCRN_NBG1, clamped_pos.y);


                vdp2_scrn_scroll_x_update(VDP2_SCRN_NBG2, clamped_pos.x);
                vdp2_scrn_scroll_y_update(VDP2_SCRN_NBG2, clamped_pos.y);

                vdp2_sync();
                vdp2_sync_wait();
        }

        tiled2saturn_free(t2s);
}

void user_init(void)
{
        vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, VDP2_TVMD_HORZ_NORMAL_A,
            VDP2_TVMD_VERT_224);

        vdp2_scrn_back_color_set(BACK_SCREEN, RGB1555(1, 0, 3, 15));

        vdp_sync_vblank_out_set(_vblank_out_handler, NULL);

        smpc_peripheral_init();
}

static void _vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}