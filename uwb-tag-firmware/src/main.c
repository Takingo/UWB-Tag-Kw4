#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uwb_tag_firmware, CONFIG_LOG_DEFAULT_LEVEL);

/* Forward declaration of UWB driver functions */
extern int uwb_driver_init(void);
extern int uwb_send_blink(void);

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

    LOG_INF("===========================================");
    LOG_INF("UWB TAG FIRMWARE - TX MODE (TRANSMITTER)");
    LOG_INF("===========================================");
    LOG_INF("Platform: NRF52833");
    LOG_INF("Transceiver: Qorvo DW3210");
    LOG_INF("RTOS: Zephyr");
    LOG_INF("===========================================");

    /* Initialize UWB driver and DW3210 transceiver */
    ret = uwb_driver_init();
    if (ret) {
        LOG_ERR("Failed to initialize UWB driver: %d", ret);
        return ret;
    }

    LOG_INF("UWB Driver initialized successfully");
    LOG_INF("Entering TX transmission loop...");
    LOG_INF("Transmission interval: 500ms");
    k_msleep(500);

    /* Main TX loop - Send BLINK frame every 500ms */
    while (1) {
        /* Send UWB BLINK frame (TX operation) */
        ret = uwb_send_blink();
        if (ret) {
            LOG_ERR("Failed to send BLINK frame: %d", ret);
        }

        /* Sleep for 490ms to achieve 500ms transmission interval */
        /* (100ms transmission time + 490ms sleep = 500ms cycle) */
        LOG_DBG("Entering sleep mode for 490ms...");
        k_msleep(490);
    }

    return 0;
}
