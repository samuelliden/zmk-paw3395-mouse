#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define NEOPIXEL_PWR_NODE DT_ALIAS(neopixel_power)

static int neopixel_power_init(const struct device *dev)
{
    ARG_UNUSED(dev);

    // Wait 2 seconds before enabling power
    k_sleep(K_MSEC(2000));

    const struct device *gpio_dev = DEVICE_DT_GET(DT_GPIO_CTLR(NEOPIXEL_PWR_NODE, gpios));
    gpio_pin_configure(gpio_dev, DT_GPIO_PIN(NEOPIXEL_PWR_NODE, gpios),
                       GPIO_OUTPUT_ACTIVE | DT_GPIO_FLAGS(NEOPIXEL_PWR_NODE, gpios));
    gpio_pin_set(gpio_dev, DT_GPIO_PIN(NEOPIXEL_PWR_NODE, gpios), 1);

    return 0;
}

SYS_INIT(neopixel_power_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);