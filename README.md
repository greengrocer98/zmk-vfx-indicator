# VFX indicator

ZMK module for showing battery and connection status by led animation

## Features
* unibody support only
* three leds for animations
* pwm support for smooth animation

### Battery animation
Battery animation can show different states of battery level which can be configured through following options:
* `CONFIG_VFX_INDICATOR_BATTERY_LEVEL_HIGH`
* `CONFIG_VFX_INDICATOR_BATTERY_LEVEL_MID`
* `CONFIG_VFX_INDICATOR_BATTERY_LEVEL_LOW`
* `CONFIG_VFX_INDICATOR_BATTERY_LEVEL_CRITICAL`

### Connection animation
Connection animation shows usb connection animation or profile index animation and next bluetooth connection animation if keyboard is connected using BLE.
Profile animation can show three different profile indices by default. *You can change profile animation for showing more profile indices though*.

## Installation
To use, first add this module to your `config/west.yml` by adding a new entry to `remotes` and `projects`:

```yaml west.yml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: ggrocer  # <-- new entry
      url-base: https://github.com/greengrocer98
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-vfx-indicator  # <-- new entry
      remote: ggrocer
      revision: main
  self:
    path: config
```

For more information, including instructions for building locally, check out the ZMK docs on [building with modules](https://zmk.dev/docs/features/modules#building-with-modules).

Then, add support of module in `<keyboard>.conf` by adding `CONFIG_VFX_INDICATOR=y`.
Add custom behaviors for showing battery and connection status by pressing buttons in `<keyboard>.keymap`:

```dts
#include <behaviors/vfx_indicator.dtsi>  // needed to use the behaviors

/ {
    keymap {
        ...
        some_layer {
            bindings = <
                ...
                &check_bat   // check battery level
                &check_conn  // check connection status
                ...
            >;
        };
    };
};
```

And finally configure led pins in `<keyboard>.overlay`:

```overlay
/ {
    backlight: pwmleds {
        compatible = "pwm-leds";
        pwm_led_0: pwm_led_0 {
            pwms = <&pwm0 0 PWM_MSEC(10) PWM_POLARITY_NORMAL>;
        };
        pwm_led_1: pwm_led_1 {
            pwms = <&pwm0 1 PWM_MSEC(10) PWM_POLARITY_NORMAL>;
        };
        pwm_led_2: pwm_led_2 {
            pwms = <&pwm0 2 PWM_MSEC(10) PWM_POLARITY_NORMAL>;
        };
    };

    aliases {
        led0 = &pwm_led_0;
        led1 = &pwm_led_1;
        led2 = &pwm_led_2;
    };
};

&pinctrl {
    pwm0_default: pwm0_default {
        group1 {
            psels = <NRF_PSEL(PWM_OUT0, 0, 31)>, // LED 0
                    <NRF_PSEL(PWM_OUT1, 0, 29)>, // LED 1
                    <NRF_PSEL(PWM_OUT2, 0, 2)>;  // LED 2
        };
    };
    pwm0_sleep: pwm0_sleep {
        group1 {
            psels = <NRF_PSEL(PWM_OUT0, 0, 31)>, // LED 0
                    <NRF_PSEL(PWM_OUT1, 0, 29)>, // LED 1
                    <NRF_PSEL(PWM_OUT2, 0, 2)>;  // LED 2
            low-power-enable;
        };
    };
};

&pwm0 {
    status = "okay";
    pinctrl-0 = <&pwm0_default>;
    pinctrl-1 = <&pwm0_sleep>;
    pinctrl-names = "default", "sleep";
};
```

## References
This module was inspired by
* [zmk-rgbled-widget](https://github.com/caksoylar/zmk-rgbled-widget/) by [caksoylar](https://github.com/caksoylar)
* [status_led](https://github.com/aroum/zmk-kabarga/blob/kabarga/config/boards/shields/kabarga/status_led.c) by [aroum](https://github.com/aroum)

## future work
* Add support for split keyboard if someone would be interested