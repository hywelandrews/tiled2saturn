#include "gamemath/defs.h"
#include "gamemath/fix16.h"
#include "gamemath/fix16/fix16_vec2.h"
#include "vdp2/scrn_macros.h"
#include "vdp2/scrn_shared.h"
#include <stdint.h>
#include <yaul.h>
#include <tga/tga.h>
#include <assert.h>
#include <stdbool.h>

#include <tiled2saturn/tiled2saturn.h>

#define NBGX_BMP         VDP2_VRAM_ADDR(0, 0x000000)
#define BACK_SCREEN      VDP2_VRAM_ADDR(3, 0x01FFFE)

extern uint8_t layer1[];
extern uint8_t layer1_end[];

static void _vblank_out_handler(void *work);

static smpc_peripheral_digital_t _digital;

int main(void)
{
        const vdp2_scrn_bitmap_format_t format = {
                .scroll_screen = VDP2_SCRN_NBG0,
                .ccc           = VDP2_SCRN_CCC_RGB_32768,
                .bitmap_size   = VDP2_SCRN_BITMAP_SIZE_512X256,
                .palette_base  = 0x00000000,
                .bitmap_base   = NBGX_BMP
        };

        const vdp2_vram_cycp_t vram_cycp = {
                .pt[0].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
                .pt[0].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
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

                .pt[2].t0 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t1 = VDP2_VRAM_CYCP_NO_ACCESS,
                .pt[2].t2 = VDP2_VRAM_CYCP_NO_ACCESS,
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

        vdp2_scrn_bitmap_format_set(&format);
        vdp2_scrn_priority_set(VDP2_SCRN_NBG0, 7);
        vdp2_scrn_display_set(VDP2_SCRN_DISP_NBG0);

        tiled2saturn_t* t2s = tiled2saturn_parse(layer1);

        scu_dma_transfer(0, (void *)NBGX_BMP, t2s->bitmap_layers[0]->bitmap, t2s->bitmap_layers[0]->bitmap_size);

        vdp2_scrn_scroll_x_set(VDP2_SCRN_NBG0, FIX16(0));
        vdp2_scrn_scroll_y_set(VDP2_SCRN_NBG0, FIX16(0));

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
}

static void _vblank_out_handler(void *work __unused)
{
        smpc_peripheral_intback_issue();
}