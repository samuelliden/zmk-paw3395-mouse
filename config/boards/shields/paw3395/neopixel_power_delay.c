#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define LED_PWR_NODE DT_ALIAS(led_power)

static int led_power_init(void)
{
    // Wait 2 seconds before enabling LED power
    k_sleep(K_MSEC(2000));

    const struct device *gpio_dev = DEVICE_DT_GET(DT_GPIO_CTLR(LED_PWR_NODE, gpios));
    if (!device_is_ready(gpio_dev)) {
        return -ENODEV;
    }

    int ret = gpio_pin_configure(gpio_dev, DT_GPIO_PIN(LED_PWR_NODE, gpios),
                                 GPIO_OUTPUT_ACTIVE | DT_GPIO_FLAGS(LED_PWR_NODE, gpios));
    if (ret < 0) {
        return ret;
    }

    return gpio_pin_set(gpio_dev, DT_GPIO_PIN(LED_PWR_NODE, gpios), 1);
}

SYS_INIT(led_power_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);