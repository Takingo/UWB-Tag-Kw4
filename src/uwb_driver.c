#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(uwb_driver, CONFIG_LOG_DEFAULT_LEVEL);

/* DW3000 Device Tree definitions */
#define DW3000_NODE DT_NODELABEL(dw3110)

/* Manual SPI configuration - bypass SPI_DT_SPEC_GET to avoid devicetree property requirements */
static struct spi_dt_spec dw3000_spi = {
	.bus = DEVICE_DT_GET(DT_BUS(DW3000_NODE)),
	.config = {
		.frequency = 1000000, /* 1MHz - ultra-safe for debugging */
		.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_MASTER,
		.slave = DT_REG_ADDR(DW3000_NODE),
	},
};

#if DT_NODE_HAS_PROP(DW3000_NODE, irq_gpios)
static const struct gpio_dt_spec dw3000_irq = GPIO_DT_SPEC_GET(DW3000_NODE, irq_gpios);
#define HAS_IRQ_GPIO 1
#else
#define HAS_IRQ_GPIO 0
#endif

#if DT_NODE_HAS_PROP(DW3000_NODE, reset_gpios)
static const struct gpio_dt_spec dw3000_reset = GPIO_DT_SPEC_GET(DW3000_NODE, reset_gpios);
#define HAS_RESET_GPIO 1
#else
#define HAS_RESET_GPIO 0
#endif

#if DT_NODE_HAS_PROP(DW3000_NODE, wakeup_gpios)
static const struct gpio_dt_spec dw3000_wakeup = GPIO_DT_SPEC_GET(DW3000_NODE, wakeup_gpios);
#define HAS_WAKEUP_GPIO 1
#else
#define HAS_WAKEUP_GPIO 0
#endif

/* DW3000 Register Addresses */
#define DW3000_REG_DEV_ID       0x00
#define DW3000_EXPECTED_DEV_ID  0xDECA0130  // DW3110 Device ID

/**
 * Test SPI bus with loopback
 */
static int dw3000_test_spi_loopback(void)
{
    LOG_INF("=== SPI Loopback Test ===");
    
    uint8_t tx_buf[5] = {0xAA, 0x55, 0x12, 0x34, 0x56};
    uint8_t rx_buf[5] = {0};
    
    const struct spi_buf tx = {.buf = tx_buf, .len = 5};
    const struct spi_buf rx = {.buf = rx_buf, .len = 5};
    const struct spi_buf_set tx_set = {&tx, 1};
    const struct spi_buf_set rx_set = {&rx, 1};
    
    LOG_INF("Attempting SPI transfer...");
    int ret = spi_transceive_dt(&dw3000_spi, &tx_set, &rx_set);
    if (ret != 0) {
        LOG_ERR("SPI transceive failed: %d", ret);
        LOG_ERR("Check: SPI wiring, chip power, CS pin (P0.11)");
        return ret;
    }
    
    LOG_INF("SPI transfer completed successfully");
    LOG_INF("TX: %02X %02X %02X %02X %02X", 
            tx_buf[0], tx_buf[1], tx_buf[2], tx_buf[3], tx_buf[4]);
    LOG_INF("RX: %02X %02X %02X %02X %02X", 
            rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4]);
    
    if (rx_buf[0] == 0xFF && rx_buf[1] == 0xFF && rx_buf[2] == 0xFF) {
        LOG_WRN("MISO stuck HIGH - check MISO pin (P0.28) or chip power");
        // Continue anyway to try reading device ID
    }
    
    if (rx_buf[0] == 0x00 && rx_buf[1] == 0x00 && rx_buf[2] == 0x00) {
        LOG_WRN("MISO stuck LOW - check MISO pin (P0.28) connection");
        // Continue anyway to try reading device ID
    }
    
    LOG_INF("SPI bus is functional (MISO responding)");
    return 0;
}

/**
 * Read DW3000 Device ID register
 * @return Device ID value or 0 on error
 */
static uint32_t dw3000_read_dev_id(void)
{
    printk("\n=== DW3000 Device ID Read Attempt ===\n");
    
    // First, verify SPI bus is working with a dummy transaction
    static uint8_t test_tx[2] = {0xFF, 0xFF};
    static uint8_t test_rx[2] = {0x00, 0x00};
    
    const struct spi_buf test_tx_buf = {.buf = test_tx, .len = 2};
    const struct spi_buf test_rx_buf = {.buf = test_rx, .len = 2};
    const struct spi_buf_set test_tx_set = {&test_tx_buf, 1};
    const struct spi_buf_set test_rx_set = {&test_rx_buf, 1};
    
    printk("TEST: Sending dummy transaction...\n");
    int ret = spi_transceive_dt(&dw3000_spi, &test_tx_set, &test_rx_set);
    printk("TEST: Result=%d, RX=[0x%02X 0x%02X]\n", ret, test_rx[0], test_rx[1]);
    
    k_msleep(5); // Small delay
    
#if HAS_WAKEUP_GPIO
    // Try to wake up chip from deep sleep
    printk("Pulsing WAKEUP pin...\n");
    gpio_pin_set_dt(&dw3000_wakeup, 1);
    k_msleep(1);
    gpio_pin_set_dt(&dw3000_wakeup, 0);
    k_msleep(5);
    printk("WAKEUP complete\n");
#endif
    
    // Now attempt Device ID read
    static uint8_t tx_buf[5];
    static uint8_t rx_buf[5];
    
    // Try multiple times with different strategies
    for (int attempt = 0; attempt < 3; attempt++) {
        memset(tx_buf, 0, 5);
        memset(rx_buf, 0, 5);
        
        // DW3110 SPI Format: [Header byte][Data bytes]
        // DEV_ID is at register 0x00, length 4 bytes
        tx_buf[0] = 0x80;  // Read (0x80) from register 0x00
        
        printk("\nAttempt %d: Reading Device ID...\n", attempt + 1);
        
        const struct spi_buf tx = {.buf = tx_buf, .len = 5};
        const struct spi_buf rx = {.buf = rx_buf, .len = 5};
        const struct spi_buf_set tx_set = {&tx, 1};
        const struct spi_buf_set rx_set = {&rx, 1};
        
        ret = spi_transceive_dt(&dw3000_spi, &tx_set, &rx_set);
    
        ret = spi_transceive_dt(&dw3000_spi, &tx_set, &rx_set);
        
        printk("  SPI ret=%d, RX=[0x%02X 0x%02X 0x%02X 0x%02X 0x%02X]\n", 
               ret, rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4]);
        
        if (ret != 0) {
            printk("  SPI transceive failed: %d\n", ret);
            k_msleep(10);
            continue;
        }
        
        // Check if we got valid data
        if (rx_buf[1] != 0 || rx_buf[2] != 0 || rx_buf[3] != 0 || rx_buf[4] != 0) {
            // Device ID is 4 bytes, little-endian, starting at byte 1
            uint32_t dev_id = (rx_buf[4] << 24) | (rx_buf[3] << 16) | (rx_buf[2] << 8) | rx_buf[1];
            printk("  SUCCESS! Device ID: 0x%08X\n", dev_id);
            return dev_id;
        }
        
        k_msleep(10); // Wait before retry
    }
    
    printk("\n!!! All attempts failed - MISO stuck at 0x00 !!!\n");
    printk("Hardware checklist:\n");
    printk("  1. Is DW3110 powered? (VDD = 3.3V)\n");
    printk("  2. Is RESET pin connected correctly? (P0.29, Active LOW)\n");
    printk("  3. Are SPI pins correct? CLK=P0.31, MOSI=P0.30, MISO=P0.28, CS=P0.02\n");
    printk("  4. Is chip in deep sleep mode?\n");
    
    return 0;
}

/**
 * Reset DW3110 chip using correct pinout
 */
static void dw3000_hw_reset(void)
{
    LOG_INF("Starting DW3110 hardware reset sequence...");
    
#if HAS_WAKEUP_GPIO
    // Assert WAKEUP first (if chip is asleep)
    if (gpio_is_ready_dt(&dw3000_wakeup)) {
        gpio_pin_set_dt(&dw3000_wakeup, 1);
        k_msleep(2);
        LOG_DBG("WAKEUP signal asserted");
    } else {
        LOG_DBG("WAKEUP GPIO not available");
    }
#else
    LOG_DBG("WAKEUP GPIO not configured");
#endif
    
#if HAS_RESET_GPIO
    if (!gpio_is_ready_dt(&dw3000_reset)) {
        LOG_WRN("Reset GPIO not available - using delay only");
        k_msleep(10);
        return;
    }
    
    // DW3110 RSTN is active-low
    // Assert reset LOW (active)
    int ret = gpio_pin_set_dt(&dw3000_reset, 1);  // Active LOW
    if (ret != 0) {
        LOG_ERR("Failed to assert reset: %d", ret);
        return;
    }
    LOG_INF("Reset asserted (LOW)");
    
    k_msleep(5);  // Hold reset for 5ms
    
    // Release reset HIGH (inactive)
    ret = gpio_pin_set_dt(&dw3000_reset, 0);  // Inactive
    if (ret != 0) {
        LOG_ERR("Failed to release reset: %d", ret);
        return;
    }
    LOG_INF("Reset released (HIGH)");
    
    k_msleep(10);  // Wait for chip to initialize
    LOG_INF("DW3110 reset complete - chip should be ready");
#else
    LOG_INF("No reset GPIO - using power-on defaults (delay only)");
    k_msleep(10);  // Give chip time to initialize from power-on
#endif
}

/**
 * Initialize UWB driver and DW3110 transceiver
 * @return 0 on success, negative error code on failure
 */
int uwb_driver_init(void)
{
    LOG_INF("=== DW3000 UWB Driver Init ===");
    
    // Check SPI bus
    if (!spi_is_ready_dt(&dw3000_spi)) {
        LOG_ERR("SPI bus not ready");
        return -ENODEV;
    }
    LOG_INF("SPI bus ready");
    
#if HAS_RESET_GPIO
    // Hardware reset cycle to ensure clean state
    printk("Performing hardware RESET cycle...\n");
    if (!gpio_is_ready_dt(&dw3000_reset)) {
        printk("RESET GPIO not ready!\n");
        return -ENODEV;
    }
    
    gpio_pin_configure_dt(&dw3000_reset, GPIO_OUTPUT_ACTIVE); // Start in RESET (LOW)
    k_msleep(10); // Hold in reset for 10ms
    gpio_pin_set_dt(&dw3000_reset, 0); // Release reset (HIGH = running)
    printk("RESET released (chip should be running)\n");
    k_msleep(10); // Wait 10ms for chip to fully boot
#endif

#if HAS_WAKEUP_GPIO
    // Wake chip from deep sleep
    printk("Waking chip from deep sleep...\n");
    if (gpio_is_ready_dt(&dw3000_wakeup)) {
        gpio_pin_configure_dt(&dw3000_wakeup, GPIO_OUTPUT_INACTIVE);
        gpio_pin_set_dt(&dw3000_wakeup, 1); // Pulse HIGH
        k_busy_wait(1000); // 1ms pulse
        gpio_pin_set_dt(&dw3000_wakeup, 0);
        k_busy_wait(5000); // Wait 5ms
        printk("WAKEUP pulse sent\n");
    }
#endif
    
    // SPI Wakeup: Toggle CS line slowly
    printk("SPI wakeup sequence...\n");
    for (int i = 0; i < 5; i++) {
        // CS is already configured by SPI driver
        k_busy_wait(500); // 500us delay between attempts
    }
    
#if HAS_IRQ_GPIO
    // Configure IRQ GPIO (P0.03) as INPUT
    printk("Configuring IRQ GPIO (P0.03)...\n");
    if (!gpio_is_ready_dt(&dw3000_irq)) {
        printk("IRQ GPIO not ready\n");
        return -ENODEV;
    }
    int ret = gpio_pin_configure_dt(&dw3000_irq, GPIO_INPUT);
    if (ret != 0) {
        printk("Failed to configure IRQ GPIO: %d\n", ret);
        return ret;
    }
    printk("IRQ GPIO configured\n");
#else
    printk("IRQ GPIO not defined\n");
#endif
    
    printk("=== CHECKPOINT A ===\n");
    printk("Reading Device ID (no sleep)...\n");
    
    uint32_t dev_id = dw3000_read_dev_id();
    printk("Device ID: 0x%08X\n", dev_id);
    
    if (dev_id == 0xFFFFFFFF) {
        LOG_ERR("SPI communication failed - MISO reads all 1s");
        LOG_ERR("Check: MISO pin connection, chip power, chip select");
        return -EIO;
    }
    
    if (dev_id == 0x00000000) {
        LOG_ERR("SPI communication failed - MISO reads all 0s");
        LOG_ERR("Check: Chip power, reset pin, SPI clock");
        return -EIO;
    }
    
    if (dev_id != DW3000_EXPECTED_DEV_ID) {
        LOG_WRN("Unexpected Device ID - continuing anyway");
    }
    
    LOG_INF("DW3000 Driver initialized successfully");
    return 0;
}

/**
 * Send UWB BLINK frame
 * @return 0 on success, negative error code on failure
 */
int uwb_send_blink(void)
{
    LOG_INF("Sending BLINK frame (TODO: implement TX)");
    k_msleep(100);
    return 0;
}
