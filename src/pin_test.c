#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

void pin_diagnostic_test(void)
{
    printk("\n=== PIN DIAGNOSTIC TEST ===\n");
    printk("Testing hardware connections...\n\n");
    
    const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    if (!device_is_ready(gpio0)) {
        printk("ERROR: GPIO0 not ready!\n");
        return;
    }
    
    // Test each SPI pin
    struct {
        uint8_t pin;
        const char *name;
    } spi_pins[] = {
        {31, "CLK (P0.31)"},
        {30, "MOSI (P0.30)"},
        {28, "MISO (P0.28)"},
        {2, "CS (P0.02)"},
        {29, "RESET (P0.29)"},
        {11, "IRQ (P0.11)"},
    };
    
    for (int i = 0; i < 6; i++) {
        // Configure as input with pull-up
        gpio_pin_configure(gpio0, spi_pins[i].pin, GPIO_INPUT | GPIO_PULL_UP);
        k_msleep(1);
        int val_pullup = gpio_pin_get(gpio0, spi_pins[i].pin);
        
        // Configure as input with pull-down
        gpio_pin_configure(gpio0, spi_pins[i].pin, GPIO_INPUT | GPIO_PULL_DOWN);
        k_msleep(1);
        int val_pulldown = gpio_pin_get(gpio0, spi_pins[i].pin);
        
        printk("%-15s: Pull-UP=%d, Pull-DOWN=%d", spi_pins[i].name, val_pullup, val_pulldown);
        
        if (val_pullup == 1 && val_pulldown == 0) {
            printk(" [FLOATING - OK]\n");
        } else if (val_pullup == 1 && val_pulldown == 1) {
            printk(" [STUCK HIGH!]\n");
        } else if (val_pullup == 0 && val_pulldown == 0) {
            printk(" [STUCK LOW or SHORT!]\n");
        } else {
            printk(" [UNKNOWN STATE]\n");
        }
    }
    
    printk("\n=== END PIN TEST ===\n\n");
}
