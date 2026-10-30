/*
 * Man hinh trang thai tuy chinh cho OLED SSD1306 128x64 (tuong duong phan OLED cua ban Vial).
 *
 * - Binh thuong: tieu de, ten layer, NUM / CAPS (den LED tu may tinh), phim bo tro dang giu.
 * - Khi bam phim / xoay nut: hien TEN PHIM bang chu to o giua, dong duoi liet ke cac phim dang giu.
 *   Giu 0,7 giay sau khi nha roi tro ve man thong tin.
 * - Tu tat man hinh sau CONFIG_ZMK_IDLE_TIMEOUT (xem macropad15.conf).
 *
 * Chi dung label cua LVGL (khong dung canvas) de tuong thich nhieu phien ban LVGL.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keymap.h>
#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/hid_indicators.h>
#endif

#include "oled_labels.h"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define HELD_MAX 4
#define SHOW_AFTER_MS 700

#define SHIFT_MODS (MOD_LSFT | MOD_RSFT)
#define CTRL_MODS (MOD_LCTL | MOD_RCTL)
#define ALT_MODS (MOD_LALT | MOD_RALT)
#define GUI_MODS (MOD_LGUI | MOD_RGUI)

struct key_id {
    uint16_t page;
    uint16_t id;
    bool shift;
};

static struct {
    struct key_id held[HELD_MAX];
    uint8_t held_n;
    struct key_id last;
    bool have_last;
    int64_t last_time;
} st;

static struct k_spinlock st_lock;
static bool ready;

static lv_obj_t *lbl_top;
static lv_obj_t *lbl_big;
static lv_obj_t *lbl_info;
static lv_obj_t *lbl_held;

static void refresh_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(refresh_work, refresh_handler);

static void request_refresh(void) {
    if (ready) {
        k_work_reschedule_for_queue(zmk_display_work_q(), &refresh_work, K_NO_WAIT);
    }
}

/* ---------------- theo doi phim ---------------- */
static void track_key(const struct zmk_keycode_state_changed *ev) {
    struct key_id k = {
        .page = ev->usage_page,
        .id = (uint16_t)ev->keycode,
        .shift = ((ev->implicit_modifiers | ev->explicit_modifiers) & SHIFT_MODS) != 0,
    };

    K_SPINLOCK(&st_lock) {
        if (ev->state) {
            bool found = false;
            for (uint8_t i = 0; i < st.held_n; i++) {
                if (st.held[i].page == k.page && st.held[i].id == k.id) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                if (st.held_n < HELD_MAX) {
                    st.held[st.held_n++] = k;
                } else {
                    for (uint8_t i = 1; i < HELD_MAX; i++) {
                        st.held[i - 1] = st.held[i];
                    }
                    st.held[HELD_MAX - 1] = k;
                }
            }
        } else {
            for (uint8_t i = 0; i < st.held_n; i++) {
                if (st.held[i].page == k.page && st.held[i].id == k.id) {
                    for (uint8_t j = i + 1; j < st.held_n; j++) {
                        st.held[j - 1] = st.held[j];
                    }
                    st.held_n--;
                    break;
                }
            }
        }
        st.last = k;
        st.have_last = true;
        st.last_time = k_uptime_get();
    }
}

static int oled_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);
    if (ev != NULL) {
        track_key(ev);
    }
    request_refresh();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(oled_status, oled_listener);
ZMK_SUBSCRIPTION(oled_status, zmk_keycode_state_changed);
ZMK_SUBSCRIPTION(oled_status, zmk_layer_state_changed);
#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
ZMK_SUBSCRIPTION(oled_status, zmk_hid_indicators_changed);
#endif

/* ---------------- ve man hinh ---------------- */
static const char *layer_name(void) {
    const char *n = zmk_keymap_layer_name(
        zmk_keymap_layer_index_to_id(zmk_keymap_highest_layer_active()));
    return n ? n : "Layer";
}

static const lv_font_t *big_font(size_t len) {
    if (len <= 2) {
        return &lv_font_montserrat_40;
    }
    if (len <= 5) {
        return &lv_font_montserrat_28;
    }
    return &lv_font_montserrat_20;
}

static void refresh_handler(struct k_work *work) {
    ARG_UNUSED(work);
    if (lbl_top == NULL) {
        return;
    }

    /* chup trang thai */
    struct key_id held[HELD_MAX];
    struct key_id last;
    uint8_t held_n;
    bool have_last;
    int64_t last_time;
    K_SPINLOCK(&st_lock) {
        memcpy(held, st.held, sizeof(held));
        held_n = st.held_n;
        last = st.last;
        have_last = st.have_last;
        last_time = st.last_time;
    }

    const int64_t age = k_uptime_get() - last_time;
    const bool show_key = have_last && (held_n > 0 || age < SHOW_AFTER_MS);
    char line[40];

    if (show_key) {
        snprintf(line, sizeof(line), "Layer: %s", layer_name());
        lv_label_set_text(lbl_top, line);
        lv_label_set_text(lbl_info, "");

        char big[12];
        oled_key_label(last.page, last.id, last.shift, big, sizeof(big));
        big[9] = 0;
        lv_obj_set_style_text_font(lbl_big, big_font(strlen(big)), LV_PART_MAIN);
        lv_label_set_text(lbl_big, big);

        line[0] = 0;
        if (held_n > 1) {
            for (uint8_t i = 0; i < held_n; i++) {
                char t[12];
                oled_key_label(held[i].page, held[i].id, held[i].shift, t, sizeof(t));
                if (strlen(line) + strlen(t) + 1 <= 15) {
                    strcat(line, t);
                    strcat(line, " ");
                }
            }
        }
        lv_label_set_text(lbl_held, line);

        if (held_n == 0) {
            /* het giu: len lich tro ve man thong tin */
            k_work_reschedule_for_queue(zmk_display_work_q(), &refresh_work,
                                        K_MSEC(SHOW_AFTER_MS - age + 1));
        }
        return;
    }

    /* man thong tin */
    uint8_t indicators = 0;
#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
    indicators = zmk_hid_indicators_get_current_profile();
#endif
    const bool num = (indicators & 0x01) != 0;
    const bool caps = (indicators & 0x02) != 0;
    const zmk_mod_flags_t mods = zmk_hid_get_explicit_mods();

    lv_label_set_text(lbl_top, "   MACROPAD15");
    lv_label_set_text(lbl_big, "");
    lv_label_set_text(lbl_held, "");

    snprintf(line, sizeof(line), "Layer: %s\n\n%s  %s\n\n%s %s %s %s", layer_name(),
             num ? "NUM" : "num", caps ? "CAPS" : "caps", (mods & SHIFT_MODS) ? "SFT" : "sft",
             (mods & CTRL_MODS) ? "CTL" : "ctl", (mods & ALT_MODS) ? "ALT" : "alt",
             (mods & GUI_MODS) ? "GUI" : "gui");
    lv_label_set_text(lbl_info, line);
}

lv_obj_t *zmk_display_status_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_text_color(screen, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(screen, &lv_font_unscii_8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    lbl_top = lv_label_create(screen);
    lv_obj_set_width(lbl_top, 128);
    lv_obj_align(lbl_top, LV_ALIGN_TOP_LEFT, 0, 0);

    lbl_info = lv_label_create(screen);
    lv_obj_set_width(lbl_info, 128);
    lv_obj_align(lbl_info, LV_ALIGN_TOP_LEFT, 0, 16);

    lbl_big = lv_label_create(screen);
    lv_obj_set_width(lbl_big, 128);
    lv_obj_set_style_text_align(lbl_big, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(lbl_big, LV_ALIGN_CENTER, 0, 0);

    lbl_held = lv_label_create(screen);
    lv_obj_set_width(lbl_held, 128);
    lv_obj_align(lbl_held, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    ready = true;
    request_refresh();
    return screen;
}
