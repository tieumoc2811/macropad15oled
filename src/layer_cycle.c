/*
 * Behavior &layer_cycle: moi lan bam chuyen sang layer ke tiep trong danh sach
 * "cycle-layers" (het danh sach thi quay lai layer dau).
 *
 * - Phan tu dau cua danh sach la layer mac dinh (luon bat, khong can kich hoat).
 * - Chi cac layer nam trong danh sach bi thay doi; layer giu phim (vi du FN) van
 *   hoat dong binh thuong, nen co the bam &layer_cycle ngay ca khi dang giu FN.
 * - Bat layer moi TRUOC roi moi tat layer cu, de layer cao nhat khong bi nhay ve 0
 *   trong luc chuyen (tranh LED bao layer nhay sai).
 */

#define DT_DRV_COMPAT zmk_behavior_layer_cycle

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct lc_config {
    const uint32_t *layers;
    uint8_t len;
};

static int lc_pressed(struct zmk_behavior_binding *binding,
                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);

    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct lc_config *cfg = dev->config;

    /* Layer hien tai = layer cuoi cung trong danh sach dang bat (mac dinh: phan tu 0). */
    uint8_t cur = 0;
    for (uint8_t i = 1; i < cfg->len; i++) {
        if (zmk_keymap_layer_active(cfg->layers[i])) {
            cur = i;
        }
    }

    const uint8_t next = (cur + 1) % cfg->len;

    if (next != 0) {
        zmk_keymap_layer_activate(cfg->layers[next], false);
    }
    for (uint8_t i = 1; i < cfg->len; i++) {
        if (i != next) {
            zmk_keymap_layer_deactivate(cfg->layers[i], true);
        }
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static int lc_released(struct zmk_behavior_binding *binding,
                       struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api lc_api = {
    .binding_pressed = lc_pressed,
    .binding_released = lc_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define LC_INST(n)                                                                                 \
    static const uint32_t lc_layers_##n[] = DT_INST_PROP(n, cycle_layers);                         \
    BUILD_ASSERT(ARRAY_SIZE(lc_layers_##n) >= 2, "cycle-layers can it nhat 2 layer");              \
    static const struct lc_config lc_cfg_##n = {                                                   \
        .layers = lc_layers_##n,                                                                   \
        .len = ARRAY_SIZE(lc_layers_##n),                                                          \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &lc_cfg_##n, POST_KERNEL,                         \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &lc_api);

DT_INST_FOREACH_STATUS_OKAY(LC_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
