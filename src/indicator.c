#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/keymap.h>
#include <zmk/workqueue.h>

#include <zmk_vfx_indicator/indicator.h>

#define LED_GPIO_NODE_ID DT_COMPAT_GET_ANY_STATUS_OKAY(gpio_leds)
#define LED_COUNT 3 

// Define stack size and priority for animation workqueue
#define ANIMATION_WORK_Q_STACK_SIZE 1024
#define ANIMATION_WORK_Q_PRIORITY 5

#ifndef LED_BRIGHTNESS_MAX
#define LED_BRIGHTNESS_MAX 100
#endif

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);
BUILD_ASSERT(DT_NODE_EXISTS(DT_ALIAS(led0)),
             "An alias for a first LED is not found for VFX_INDICATOR");
BUILD_ASSERT(DT_NODE_EXISTS(DT_ALIAS(led1)),
             "An alias for a second LED is not found for VFX_INDICATOR");
BUILD_ASSERT(DT_NODE_EXISTS(DT_ALIAS(led2)),
             "An alias for a third LED is not found for VFX_INDICATOR");

// GPIO-based LED device and indices of LEDs inside its DT node
static const struct device *led_dev = DEVICE_DT_GET(DT_CHOSEN(zmk_backlight));
static const uint8_t led_idx[] = {DT_NODE_CHILD_IDX(DT_ALIAS(led0)),
                                  DT_NODE_CHILD_IDX(DT_ALIAS(led1)),
                                  DT_NODE_CHILD_IDX(DT_ALIAS(led2))};

static void smooth_led_off(uint32_t led_id, uint16_t duration_ms)
{
    uint16_t step_duration_ms = duration_ms / LED_BRIGHTNESS_MAX;
    for (int step = LED_BRIGHTNESS_MAX; step >= 0; step--)
    {
        led_set_brightness(led_dev, led_idx[led_id], step);
        k_msleep(step_duration_ms);
    }
}

static void smooth_led_on(uint32_t led_id, uint16_t duration_ms)
{
    uint16_t step_duration_ms = duration_ms / LED_BRIGHTNESS_MAX;
    for (int step = 0; step <= LED_BRIGHTNESS_MAX; step++)
    {
        led_set_brightness(led_dev, led_idx[led_id], step);
        k_msleep(step_duration_ms);
    }
}

static void mask_smooth_led_off(uint8_t mask, uint16_t duration_ms)
{
    uint16_t step_duration_ms = duration_ms / LED_BRIGHTNESS_MAX;
    for (int step = LED_BRIGHTNESS_MAX; step >= 0; step--)
    {
        for (int i = 0; i < LED_COUNT; i++)
        {
            if (mask & (1 << i)) {
                led_set_brightness(led_dev, led_idx[i], step);
            }
        }
        k_msleep(step_duration_ms);
    }
}

static void mask_smooth_led_on(uint8_t mask, uint16_t duration_ms)
{
    uint16_t step_duration_ms = duration_ms / LED_BRIGHTNESS_MAX;
    for (int step = 0; step <= LED_BRIGHTNESS_MAX; step++)
    {
        for (int i = 0; i < LED_COUNT; i++)
        {
            if (mask & (1 << i)) {
                led_set_brightness(led_dev, led_idx[i], step);
            }
        }
        k_msleep(step_duration_ms);
    }
}

static void usb_conn_animation()
{
    mask_smooth_led_on(0b101,300);
    mask_smooth_led_off(0b101,0);
    smooth_led_on(1,400);
    smooth_led_off(1,0);
    mask_smooth_led_on(0b101,300);
    mask_smooth_led_off(0b101,0);
}

static void ble_conn_on_animation()
{
    mask_smooth_led_on(0b111, 1000);
    mask_smooth_led_off(0b111, 200);
}

static void ble_conn_off_animation()
{
    mask_smooth_led_on(0b010, 1000);
    mask_smooth_led_off(0b010, 200);
}

static void ble_conn_open_animation()
{
    smooth_led_on(0, 100);
    smooth_led_on(1, 100);
    smooth_led_on(2, 100);
    mask_smooth_led_off(0b111,100);
    smooth_led_on(0, 100);
    smooth_led_on(1, 100);
    smooth_led_on(2, 100);
    mask_smooth_led_off(0b111,100);
    smooth_led_on(0, 100);
    smooth_led_on(1, 100);
    smooth_led_on(2, 100);
    mask_smooth_led_off(0b111,100);
}

static void unknown_trans_animation()
{
    smooth_led_on(0, 160);
    smooth_led_on(1, 160);
    smooth_led_on(2, 160);
    smooth_led_off(2, 160);
    smooth_led_off(1, 160);
    smooth_led_off(0, 160);
}

static void battery_animation()
{
    uint8_t battery_level = zmk_battery_state_of_charge();
    if (battery_level > CONFIG_VFX_INDICATOR_BATTERY_LEVEL_HIGH) {
        smooth_led_on(0, 330);
        smooth_led_on(1, 330);
        smooth_led_on(2, 330);
        mask_smooth_led_off(0b111, 400);
    } else if (battery_level > CONFIG_VFX_INDICATOR_BATTERY_LEVEL_MID) {
        smooth_led_on(0, 400);
        smooth_led_on(1, 400);
        mask_smooth_led_off(0b11, 400);
    } else if (battery_level > CONFIG_VFX_INDICATOR_BATTERY_LEVEL_LOW) {
        smooth_led_on(0, 400);
        smooth_led_off(0, 400);
    } else {
        mask_smooth_led_on(0b111, 160);
        mask_smooth_led_off(0b111, 160);
        mask_smooth_led_on(0b111, 160);
        mask_smooth_led_off(0b111, 160);
        mask_smooth_led_on(0b111, 160);
        mask_smooth_led_off(0b111, 160);
    }
}

static void profile_animation()
{
    uint8_t profile_index = zmk_ble_active_profile_index();
    smooth_led_on(profile_index, 500);
    smooth_led_off(profile_index, 500);
}

// Define stack area for animation workqueue
K_THREAD_STACK_DEFINE(animation_work_q_stack, ANIMATION_WORK_Q_STACK_SIZE);

// Define workqueue object
struct k_work_q animation_work_q;

struct k_work_delayable conn_work;
void conn_handler(struct k_work *work)
{
    switch (zmk_endpoints_selected().transport)
    {
    case ZMK_TRANSPORT_USB:
        usb_conn_animation();
        break;
    case ZMK_TRANSPORT_BLE:
        #if IS_ENABLED(CONFIG_ZMK_BLE)
        profile_animation();
        if (zmk_ble_active_profile_is_connected()) {
            ble_conn_on_animation();
        } else if (zmk_ble_active_profile_is_open()) {
            ble_conn_open_animation();
        } else {
            ble_conn_off_animation();
        }
        #endif
        break;
    default:
        unknown_trans_animation();
        break;
    }

    return;
}
K_WORK_DELAYABLE_DEFINE(conn_work, conn_handler);

struct k_work_delayable batt_work;
void battery_handler(struct k_work *work)
{
    battery_animation();
}
K_WORK_DELAYABLE_DEFINE(batt_work, battery_handler);

int conn_listener(const zmk_event_t *eh)
{
    k_work_schedule_for_queue(&animation_work_q, &conn_work, K_NO_WAIT);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(conn_state, conn_listener)
ZMK_SUBSCRIPTION(conn_state, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(conn_state, zmk_ble_active_profile_changed);

int battery_listener(const zmk_event_t *eh)
{
    uint8_t battery_level = as_zmk_battery_state_changed(eh)->state_of_charge;

    if (battery_level > 0 && battery_level <= CONFIG_VFX_INDICATOR_BATTERY_LEVEL_CRITICAL) {
        k_work_schedule_for_queue(&animation_work_q, &batt_work, K_NO_WAIT);
    }
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(battery_state, battery_listener);
ZMK_SUBSCRIPTION(battery_state, zmk_battery_state_changed);

static int init_animation(const struct device *dev) {
    k_work_queue_init(&animation_work_q);

    k_work_queue_start(&animation_work_q, animation_work_q_stack,
                       K_THREAD_STACK_SIZEOF(animation_work_q_stack), ANIMATION_WORK_Q_PRIORITY,
                       NULL);

    k_work_schedule_for_queue(&animation_work_q, &batt_work, K_SECONDS(1));
    return 0;
}

SYS_INIT(init_animation, APPLICATION, 32);