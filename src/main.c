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

    /* Setup LED for visual feedback - start OFF */
    if (gpio_is_ready_dt(&led)) {
        gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
        gpio_pin_set_dt(&led, 1); /* ACTIVE_LOW: 1=OFF, 0=ON */
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

    /* Blink LED 3x to show startup */
    for (int i = 0; i < 3; i++) {
        if (gpio_is_ready_dt(&led)) {
            gpio_pin_set_dt(&led, 0); /* ACTIVE_LOW: 0=ON */
            k_busy_wait(100000); /* 100ms ON */
            gpio_pin_set_dt(&led, 1); /* ACTIVE_LOW: 1=OFF */
            k_busy_wait(100000); /* 100ms OFF */
        }
    }

    printk("About to call uwb_driver_init...\n");

    /* Initialize UWB driver and DW3210 transceiver */
    ret = uwb_driver_init();
    if (ret) {
        printk("Init failed: %d\n", ret);
        /* Fast blink on error */
        while (1) {
            if (gpio_is_ready_dt(&led)) {
                gpio_pin_toggle_dt(&led);
                k_busy_wait(100000); /* 100ms fast blink */
            }
        }
    }

    printk("=== INIT COMPLETE ===\n");
    printk("=== Starting BLINK TX Mode ===\n");
    printk("TX Rate: 5 Hz (every 200ms)\n\n");
    
    uint32_t tx_count = 0;
    
    /* Main loop: Send BLINK frames periodically */
    while (1) {
        /* LED ON just before TX (ACTIVE_LOW: 0=ON) */
        if (gpio_is_ready_dt(&led)) {
            gpio_pin_set_dt(&led, 0);
        }
        
        /* Send BLINK frame */
        ret = uwb_send_blink();
        if (ret == 0) {
            tx_count++;
            if (tx_count % 10 == 0) {
                printk("TX Count: %u\n", tx_count);
            }
        } else {
            printk("TX failed: %d\n", ret);
        }
        
        /* Keep LED ON for 50ms to make it visible */
        k_msleep(50);
        
        /* LED OFF (ACTIVE_LOW: 1=OFF) */
        if (gpio_is_ready_dt(&led)) {
            gpio_pin_set_dt(&led, 1);
        }
        
        /* Wait 150ms more (total 200ms between TX = 5 Hz rate) */
        k_msleep(150);
    }

    return 0;
}
