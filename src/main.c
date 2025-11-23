#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include "pin_test.h"

LOG_MODULE_REGISTER(uwb_tag_firmware, CONFIG_LOG_DEFAULT_LEVEL);

/* Forward declaration of UWB driver functions */
extern int uwb_driver_init(void);
extern int uwb_send_blink(void);

/* LED for visual debug */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/**
 * Main application entry point
 * UWB TAG FIRMWARE - TX Mode (Transmitter/BLINK)
 * 
 * This firmware runs the NRF52833 in TX-only mode:
 * - Sends UWB BLINK frames every 500ms
 * - Enters sleep mode for 490ms between transmissions
 * - NO RX (receive) functionality
 */
int main(void)
{
    int ret = 0;

    /* Setup LED for visual feedback */
    if (gpio_is_ready_dt(&led)) {
        gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
        gpio_pin_set_dt(&led, 1); /* Turn on - we're alive! */
    }

    /* Force RTT init */
    printk("\n\n\n*** BOOT START ***\n");
    
    printk("===========================================\n");
    printk("UWB TAG FIRMWARE - TX MODE (TRANSMITTER)\n");
    printk("===========================================\n");
    printk("Platform: NRF52833\n");
    printk("Transceiver: Decawave DW3110\n");
    printk("RTOS: Zephyr\n");
    printk("===========================================\n");

    /* Run pin diagnostic test BEFORE UWB init */
    printk("\nRunning hardware diagnostic...\n");
    pin_diagnostic_test();

    /* Blink LED to show we got here */
    for (int i = 0; i < 3; i++) {
        if (gpio_is_ready_dt(&led)) {
            gpio_pin_set_dt(&led, 0);
            k_busy_wait(100000); /* 100ms */
            gpio_pin_set_dt(&led, 1);
            k_busy_wait(100000);
        }
    }

    printk("About to call uwb_driver_init...\n");

    /* Initialize UWB driver and DW3210 transceiver */
    ret = uwb_driver_init();
    if (ret) {
        printk("Init failed: %d\n", ret);
        /* Rapid blink on error */
        while (1) {
            if (gpio_is_ready_dt(&led)) {
                gpio_pin_toggle_dt(&led);
                k_busy_wait(50000); /* 50ms rapid blink */
            }
        }
    }

    printk("=== INIT COMPLETE ===\n");
    printk("Entering busy wait loop (no sleep)\n");
    
    /* Slow blink = success */
    while (1) {
        if (gpio_is_ready_dt(&led)) {
            gpio_pin_toggle_dt(&led);
        }
        k_busy_wait(500000); /* 500ms slow blink = success */
    }

    return 0;
}
