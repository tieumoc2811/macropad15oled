/*
 * Behavior kieu Nokia (multi-tap): bam lien tiep CUNG MOT PHIM trong "tapping-term-ms"
 * de doi ky tu. Ky tu hien ngay khi bam: moi lan bam them thi gui Backspace roi gui
 * ky tu ke tiep trong danh sach "keys".
 *
 * Chuoi ket thuc khi: het thoi gian, bam phim multi-tap khac, hoac bam bat ky phim nao
 * khac (de khong xoa nham ky tu cua phim khac).
 *
 * Shift (vi du sticky shift &sk LSHFT) duoc ghi nho o lan bam dau va ap lai cho cac lan
 * doi ky tu sau, vi sticky shift da nha ra ngay sau ky tu dau tien.
 * Shift CHI ap dung cho chu cai (a-z); chu so, space, '.' va ',' khong bi shift
 * ('?' va '!' da co shift san trong keycode).
 */

#define DT_DRV_COMPAT zmk_behavior_multitap

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/hid.h>
#include <zmk/keys.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

#define BSPC_ENC ZMK_HID_USAGE(HID_USAGE_KEY, HID_USAGE_KEY_KEYBOARD_DELETE_BACKSPACE)
#define SHIFT_MODS (MOD_LSFT | MOD_RSFT)

struct mt_config {
    const uint32_t *keys;
    uint8_t len;
    uint16_t term_ms;
};

static const struct device *chain_dev; /* behavior dang trong chuoi (NULL = khong co chuoi) */
static uint8_t chain_idx;              /* chi so ky tu hien tai trong keys[] */
static int64_t chain_ts;               /* thoi diem bam gan nhat (ms) */
static uint8_t chain_mods;             /* shift ghi nho o lan bam dau */
static bool sending;                   /* dang tu gui phim, bo qua o listener */

/* Keycode la chu cai a-z (trang HID 0x07, id 0x04..0x1D)? */
static bool is_letter(uint32_t encoded) {
    const uint32_t page = (encoded >> 16) & 0xFF;
    const uint32_t id = encoded & 0xFFFF;
    return page == 0x07 && id >= 0x04 && id <= 0x1D;
}

static void tap(uint32_t encoded, uint8_t mods) {
    const uint32_t enc = encoded | ((uint32_t)mods << 24);

    sending = true;
    raise_zmk_keycode_state_changed_from_encoded(enc, true, k_uptime_get());
    raise_zmk_keycode_state_changed_from_encoded(enc, false, k_uptime_get());
    sending = false;
}

static int mt_pressed(struct zmk_behavior_binding *binding,
                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);

    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct mt_config *cfg = dev->config;
    const int64_t now = k_uptime_get();

    if (chain_dev == dev && (now - chain_ts) <= cfg->term_ms) {
        tap(BSPC_ENC, 0); /* xoa ky tu vua go, thay bang ky tu ke tiep */
        chain_idx = (chain_idx + 1) % cfg->len;
    } else {
        chain_dev = dev;
        chain_idx = 0;
        chain_mods = zmk_hid_get_explicit_mods() & SHIFT_MODS;
    }

    tap(cfg->keys[chain_idx], is_letter(cfg->keys[chain_idx]) ? chain_mods : 0);
    chain_ts = now;
    return ZMK_BEHAVIOR_OPAQUE;
}

static int mt_released(struct zmk_behavior_binding *binding,
                       struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

/* Phim khac (khong phai do minh gui) duoc bam -> ket thuc chuoi. */
static int mt_listener(const zmk_event_t *eh) {
    const struct zmk_keycode_state_changed *ev = as_zmk_keycode_state_changed(eh);

    if (ev != NULL && ev->state && !sending) {
        chain_dev = NULL;
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(multitap, mt_listener);
ZMK_SUBSCRIPTION(multitap, zmk_keycode_state_changed);

static const struct behavior_driver_api mt_api = {
    .binding_pressed = mt_pressed,
    .binding_released = mt_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define MT_INST(n)                                                                                 \
    static const uint32_t mt_keys_##n[] = DT_INST_PROP(n, keys);                                   \
    BUILD_ASSERT(ARRAY_SIZE(mt_keys_##n) >= 1, "keys khong duoc rong");                            \
    static const struct mt_config mt_cfg_##n = {                                                   \
        .keys = mt_keys_##n,                                                                       \
        .len = ARRAY_SIZE(mt_keys_##n),                                                            \
        .term_ms = DT_INST_PROP(n, tapping_term_ms),                                               \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &mt_cfg_##n, POST_KERNEL,                         \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &mt_api);

DT_INST_FOREACH_STATUS_OKAY(MT_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
