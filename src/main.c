#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include "pin_test.h"

LOG_MODULE_REGISTER(uwb_tag_firmware, CONFIG_LOG_DEFAULT_LEVEL);

/* Forward declaration of UWB driver functions */
extern int uwb_driver_init(void);
extern int uwb_send_blink(void);

/* LED for visual debug - P0.20 Red LED (ACTIVE_LOW) */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* P0.05 Front LED - Force OFF for power saving (before EXTON use) */
static const struct gpio_dt_spec led_front = {
    .port = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
    .pin = 5,
    .dt_flags = GPIO_ACTIVE_HIGH
};

/* Button (P0.04) - Arka taraftaki buton */
static const struct gpio_dt_spec button = {
    .port = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
    .pin = 4,
    .dt_flags = GPIO_ACTIVE_HIGH  // Buton normal durumda LOW, basınca HIGH
};

/* Button callback için */
static struct gpio_callback button_cb_data;

/* Debounce için son buton zamanı */
static int64_t last_button_time = 0;

/**
 * Buton basıldığında çağrılır
 * P0.20 LED'i 3 kez yanıp söndürür
 */
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    int64_t now = k_uptime_get();
    
    /* Debounce: 500ms içinde tekrar tetiklenmeyi görmezden gel */
    if ((now - last_button_time) < 500) {
        return;
    }
    last_button_time = now;
    
    /* Buton durumunu oku - Sadece LOW->HIGH geçişinde blink yap */
    int button_state = gpio_pin_get_dt(&button);
    if (button_state != 1) {
        return;  // Buton hala basılı veya LOW
    }
    
    printk("Button pressed! Blinking P0.20 LED 3x...\n");
    
    /* P0.20 LED'i 3 kez yanıp söndür */
    for (int i = 0; i < 3; i++) {
        if (gpio_is_ready_dt(&led)) {
            gpio_pin_set_dt(&led, 0);  // ACTIVE_LOW: 0=ON
            k_msleep(100);
            gpio_pin_set_dt(&led, 1);  // ACTIVE_LOW: 1=OFF
            k_msleep(100);
        }
    }
}

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

    /* P0.05 (Front LED) başlatma - Başlangıçta KAPALI */
    if (device_is_ready(led_front.port)) {
        gpio_pin_configure_dt(&led_front, GPIO_OUTPUT_INACTIVE);
        gpio_pin_set_dt(&led_front, 0);  // LED OFF
        printk("P0.05 (Front LED) initialized - Will blink every 5 TX (1 sec)\n");
    }

    /* P0.20 (Back LED) başlatma */
    if (gpio_is_ready_dt(&led)) {
        gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
        gpio_pin_set_dt(&led, 1);  // ACTIVE_LOW: 1=OFF
        printk("P0.20 (Back LED) initialized - Button controlled\n");
    }

    /* Buton başlatma (P0.04) */
    if (device_is_ready(button.port)) {
        /* Configure with internal pull-up */
        gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
        gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
        
        /* Callback ayarla */
        gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
        gpio_add_callback(button.port, &button_cb_data);
        
        printk("Button (P0.04) initialized - Press and release to blink P0.20 LED\n");
        printk("Button current state: %d (0=pressed, 1=released)\n", gpio_pin_get_dt(&button));
    } else {
        printk("WARNING: Button port not ready!\n");
    }

    printk("\n\n\n*** BOOT START ***\n");
    printk("===========================================\n");
    printk("UWB TAG FIRMWARE - TX MODE (TRANSMITTER)\n");
    printk("===========================================\n");
    printk("Platform: NRF52833\n");
    printk("Transceiver: Decawave DW3110\n");
    printk("RTOS: Zephyr\n");
    printk("===========================================\n");

    /* Hardware diagnostic */
    printk("\nRunning hardware diagnostic...\n");
    pin_diagnostic_test();

    /* Startup blink (3x) - Sadece P0.20 kullan */
    for (int i = 0; i < 3; i++) {
        if (gpio_is_ready_dt(&led)) {
            gpio_pin_set_dt(&led, 0);
            k_busy_wait(100000);
            gpio_pin_set_dt(&led, 1);
            k_busy_wait(100000);
        }
    }

    /* UWB driver init */
    printk("About to call uwb_driver_init...\n");
    ret = uwb_driver_init();
    if (ret) {
        printk("Init failed: %d\n", ret);
        while (1) {
            if (gpio_is_ready_dt(&led)) {
                gpio_pin_toggle_dt(&led);
                k_busy_wait(100000);
            }
        }
    }

    printk("=== INIT COMPLETE ===\n");
    printk("=== Starting BLINK TX Mode ===\n");
    printk("TX Rate: 5 Hz (every 200ms)\n");
    printk("Front LED (P0.05): Blinks every 5 TX (1 sec) for power saving\n");
    printk("Back LED (P0.20): OFF (not used)\n\n");
    
    uint32_t tx_count = 0;
    
    /* Main loop: Send BLINK frames with LED pulse every 5 TX */
    while (1) {
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
        
        /* LED yanıp sönsün - Her 5 TX'te bir (1 saniyede bir) */
        if (tx_count % 5 == 0) {
            if (device_is_ready(led_front.port)) {
                gpio_pin_set_dt(&led_front, 1);  // LED ON
                k_msleep(50);                     // 50ms yanık kal
                gpio_pin_set_dt(&led_front, 0);  // LED OFF
                k_msleep(150);                    // 150ms bekle
            }
        } else {
            k_msleep(200);  // LED yanmadan 200ms bekle
        }
    }

    return 0;
}
