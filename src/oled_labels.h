/*
 * Doi ma phim HID (usage page + id) thanh ten ngan (toi da 6 ky tu) de hien tren OLED.
 * Khong phu thuoc ZMK/LVGL nen co the test tren may tinh.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define HID_PAGE_KEYBOARD 0x07
#define HID_PAGE_CONSUMER 0x0C

/* shift = true: keycode dang di kem Shift (de hien "?" thay vi "/", "!" thay vi "1") */
static inline void oled_key_label(uint16_t page, uint16_t id, bool shift, char *out, size_t n) {
    const char *s = NULL;
    char tmp[8];

    if (page == HID_PAGE_KEYBOARD) {
        if (id >= 0x04 && id <= 0x1D) {
            tmp[0] = 'A' + (id - 0x04);
            tmp[1] = 0;
            s = tmp;
        } else if (id >= 0x1E && id <= 0x27) {
            if (shift && id == 0x1E) {
                s = "!";
            } else {
                tmp[0] = (id == 0x27) ? '0' : '1' + (id - 0x1E);
                tmp[1] = 0;
                s = tmp;
            }
        } else if (id >= 0x3A && id <= 0x45) {
            snprintf(tmp, sizeof(tmp), "F%d", id - 0x3A + 1);
            s = tmp;
        } else if (id >= 0x59 && id <= 0x61) { /* keypad 1..9 */
            tmp[0] = '1' + (id - 0x59);
            tmp[1] = 0;
            s = tmp;
        } else {
            switch (id) {
            case 0x28: s = "ENTER"; break;
            case 0x29: s = "ESC"; break;
            case 0x2A: s = "BKSP"; break;
            case 0x2B: s = "TAB"; break;
            case 0x2C: s = "SPACE"; break;
            case 0x2D: s = "-"; break;
            case 0x2E: s = "="; break;
            case 0x36: s = ","; break;
            case 0x37: s = "."; break;
            case 0x38: s = shift ? "?" : "/"; break;
            case 0x49: s = "INS"; break;
            case 0x4A: s = "HOME"; break;
            case 0x4B: s = "PG UP"; break;
            case 0x4C: s = "DEL"; break;
            case 0x4D: s = "END"; break;
            case 0x4E: s = "PG DN"; break;
            case 0x4F: s = "RIGHT"; break;
            case 0x50: s = "LEFT"; break;
            case 0x51: s = "DOWN"; break;
            case 0x52: s = "UP"; break;
            case 0x54: s = "/"; break;
            case 0x55: s = "*"; break;
            case 0x56: s = "-"; break;
            case 0x57: s = "+"; break;
            case 0x58: s = "ENTER"; break;
            case 0x62: s = "0"; break;
            case 0x63: s = "."; break;
            case 0xE0: case 0xE4: s = "CTRL"; break;
            case 0xE1: case 0xE5: s = "SHIFT"; break;
            case 0xE2: case 0xE6: s = "ALT"; break;
            case 0xE3: case 0xE7: s = "GUI"; break;
            default: break;
            }
        }
    } else if (page == HID_PAGE_CONSUMER) {
        switch (id) {
        case 0xE9: s = "VOL +"; break;
        case 0xEA: s = "VOL -"; break;
        case 0xE2: s = "MUTE"; break;
        case 0xCD: s = "PLAY"; break;
        case 0xB5: s = "NEXT"; break;
        case 0xB6: s = "PREV"; break;
        case 0x192: s = "CALC"; break;
        default: break;
        }
    }

    snprintf(out, n, "%s", s ? s : "KEY");
}
