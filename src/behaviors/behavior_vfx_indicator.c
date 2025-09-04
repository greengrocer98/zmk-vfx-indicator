#define DT_DRV_COMPAT zmk_behavior_vfx_indicator

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <zmk/behavior.h>

#include <zmk_vfx_indicator/indicator.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct behavior_vfx_ind_config
{
    bool check_battery;
    bool check_connection;
};

static int behavior_vfx_ind_init(const struct device *dev) { return 0; }

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event)
{
#if IS_ENABLED(CONFIG_VFX_INDICATOR)
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_vfx_ind_config *cfg = dev->config;

    if (cfg->check_battery)
    {
        k_work_reschedule(&batt_work, K_NO_WAIT);
    }
    if (cfg->check_connection)
    {
        k_work_reschedule(&conn_work, K_NO_WAIT);
    }
#endif // IS_ENABLED(CONFIG_VFX_INDICATOR)

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event)
{
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_vfx_ind_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif // IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
};

#define VFXIND_INST(n)                                                                          \
    static struct behavior_vfx_ind_config behavior_vfx_ind_config_##n = {                       \
        .check_battery = DT_INST_PROP(n, check_battery),                                        \
        .check_connection = DT_INST_PROP(n, check_connection),                                  \
    };                                                                                          \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_vfx_ind_init, NULL, NULL, &behavior_vfx_ind_config_##n, \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                   \
                            &behavior_vfx_ind_driver_api);

DT_INST_FOREACH_STATUS_OKAY(VFXIND_INST)