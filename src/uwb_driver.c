#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(uwb_driver, CONFIG_LOG_DEFAULT_LEVEL);

/* DW3000 Device Tree definitions */
#define DW3000_NODE DT_NODELABEL(dw3110)

/* SPI configuration - NO hardware CS! We'll control CS manually like Decawave SDK */
static struct spi_config dw3000_spi_cfg = {
	.frequency = 1000000, /* 1MHz for debugging */
	.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA,
	.slave = 0,
	.cs = {0}, /* NO hardware CS control */
};

static const struct device *spi_dev = DEVICE_DT_GET(DT_BUS(DW3000_NODE));

/* Manual CS pin control (like Decawave SDK does) */
#if DT_SPI_DEV_HAS_CS_GPIOS(DW3000_NODE)
static const struct gpio_dt_spec dw3000_cs = GPIO_DT_SPEC_GET_BY_IDX(DT_BUS(DW3000_NODE), cs_gpios, DT_REG_ADDR(DW3000_NODE));
#define HAS_CS_GPIO 1
#else
#define HAS_CS_GPIO 0
#endif

/* GPIO definitions from devicetree */
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

#if DT_NODE_HAS_PROP(DW3000_NODE, exton_gpios)
static const struct gpio_dt_spec dw3000_exton = GPIO_DT_SPEC_GET(DW3000_NODE, exton_gpios);
#define HAS_EXTON_GPIO 1
#else
#define HAS_EXTON_GPIO 0
#endif

/* DW3000 Register Addresses */
#define DW3000_REG_DEV_ID       0x00
#define DW3000_EXPECTED_DEV_ID  0xDECA0130  // DW3110 Device ID

/**
 * DW3110 Power-On and Wake-Up Sequence
 * Per DW3110 datasheet, proper power-on sequence is critical:
 * NOTE: This board only has RESET and IRQ pins connected.
 *       EXTON and WAKEUP are NOT present in schematic.
 * 1. Release RESET (if used)
 * 2. Wait for chip boot (5ms min)
 */
static int dw3000_power_on_sequence(void)
{
    int ret;
    
    printk("\n=== DW3110 Power-On Sequence ===\n");
    printk("CRITICAL: EXTON (P0.05) controls AP3406 buck converter for DW3110 +1V8 power\n");
    
#if HAS_EXTON_GPIO
    /* STEP 1: Enable DW3110 power via EXTON -> AP3406 EN pin */
    if (!device_is_ready(dw3000_exton.port)) {
        printk("ERROR: EXTON GPIO not ready!\n");
        return -ENODEV;
    }
    
    ret = gpio_pin_configure_dt(&dw3000_exton, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("ERROR: Failed to configure EXTON pin: %d\n", ret);
        return ret;
    }
    
    /* Set EXTON HIGH to enable AP3406 buck converter (U7) */
    gpio_pin_set_dt(&dw3000_exton, 1);  /* HIGH = Enable +1V8 power */
    printk("EXTON: HIGH - AP3406 buck converter ENABLED, +1V8 power ON\n");
    k_msleep(10);  /* Wait for power to stabilize */
#else
    printk("ERROR: EXTON not defined - DW3110 will NOT have power!\n");
    return -ENODEV;
#endif
    
#if HAS_RESET_GPIO
    /* Step 2: Release RESET (Active LOW, so set HIGH to release) */
    if (!device_is_ready(dw3000_reset.port)) {
        printk("ERROR: RESET GPIO not ready!\n");
        return -ENODEV;
    }
    
    ret = gpio_pin_configure_dt(&dw3000_reset, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("ERROR: Failed to configure RESET pin: %d\n", ret);
        return ret;
    }
    
    printk("RESET: Asserting reset (LOW)...\n");
    gpio_pin_set_dt(&dw3000_reset, 1);  /* Assert reset (Active LOW = logic 0) */
    k_msleep(10);
    
    printk("RESET: Releasing reset (HIGH)...\n");
    gpio_pin_set_dt(&dw3000_reset, 0);  /* Release reset (Active LOW = logic 1) */
    k_msleep(10);  /* Wait for chip to boot */
    printk("RESET: Complete\n");
#else
    printk("WARNING: RESET pin not defined in devicetree!\n");
    k_msleep(20);  /* Give chip time anyway */
#endif

#if HAS_IRQ_GPIO
    /* Step 2: Configure IRQ pin as input (P0.11) */
    if (!device_is_ready(dw3000_irq.port)) {
        printk("ERROR: IRQ GPIO not ready!\n");
        return -ENODEV;
    }
    
    ret = gpio_pin_configure_dt(&dw3000_irq, GPIO_INPUT);
    if (ret < 0) {
        printk("ERROR: Failed to configure IRQ pin: %d\n", ret);
        return ret;
    }
    printk("IRQ: Configured as input (P0.11)\n");
#else
    printk("WARNING: IRQ pin not defined in devicetree!\n");
#endif
    
    printk("=== Power-On Sequence Complete ===\n");
    printk("Chip should now be ready for SPI communication\n\n");
    
    return 0;
}

/**
 * Read DW3000 Device ID register - Using Decawave SDK pattern
 * @return Device ID value or 0 on error
 */
static uint32_t dw3000_read_dev_id(void)
{
    printk("\n=== DW3000 Device ID Read Attempt (SDK Pattern) ===\n");
    
    // Debug: Check SPI configuration
    printk("SPI Config Check:\n");
    printk("  Bus device: %s\n", spi_dev->name);
    printk("  Frequency: %u Hz\n", dw3000_spi_cfg.frequency);
    printk("  Operation: 0x%08X\n", dw3000_spi_cfg.operation);
#if HAS_CS_GPIO
    printk("  CS GPIO: Port=%p, Pin=%d (MANUAL CONTROL)\n", 
           dw3000_cs.port, dw3000_cs.pin);
#else
    printk("  ERROR: No CS GPIO defined!\n");
    return 0;
#endif
    
    // DW3110 SPI Format: [Header byte][Data bytes]
    // DEV_ID is at register 0x00, length 4 bytes
    static uint8_t tx_buf[5];
    static uint8_t rx_buf[5];
    
    // Try multiple times
    for (int attempt = 0; attempt < 3; attempt++) {
        memset(tx_buf, 0, 5);
        memset(rx_buf, 0, 5);
        
        tx_buf[0] = 0x80;  // Read (0x80) from register 0x00
        
        printk("\nAttempt %d: Reading Device ID...\n", attempt + 1);
        
        const struct spi_buf tx = {.buf = tx_buf, .len = 5};
        const struct spi_buf rx = {.buf = rx_buf, .len = 5};
        const struct spi_buf_set tx_set = {&tx, 1};
        const struct spi_buf_set rx_set = {&rx, 1};
        
        // Manual CS control - CORRECTED for ACTIVE_LOW
#if HAS_CS_GPIO
        gpio_pin_set_dt(&dw3000_cs, 1);  // CS ACTIVE (LOW physically) = Select chip
        k_busy_wait(1);  // Small delay
#endif
        
        int ret = spi_transceive(spi_dev, &dw3000_spi_cfg, &tx_set, &rx_set);
        
#if HAS_CS_GPIO        
        k_busy_wait(1);  // Small delay
        gpio_pin_set_dt(&dw3000_cs, 0);  // CS INACTIVE (HIGH physically) = Deselect chip
#endif
        
        printk("  SPI ret=%d, RX=[0x%02X 0x%02X 0x%02X 0x%02X 0x%02X]\n", 
               ret, rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3], rx_buf[4]);
        
        if (ret != 0) {
            printk("  SPI transceive failed: %d\n", ret);
            k_msleep(10);
            continue;
        }
        
        // Check if we got valid data (any non-zero response)
        if (rx_buf[0] != 0 || rx_buf[1] != 0 || rx_buf[2] != 0 || rx_buf[3] != 0 || rx_buf[4] != 0) {
            // Try multiple interpretations of the Device ID
            // Option 1: Little-endian starting at byte 1 (skip header)
            uint32_t dev_id_v1 = (rx_buf[4] << 24) | (rx_buf[3] << 16) | (rx_buf[2] << 8) | rx_buf[1];
            // Option 2: Little-endian starting at byte 0
            uint32_t dev_id_v2 = (rx_buf[3] << 24) | (rx_buf[2] << 16) | (rx_buf[1] << 8) | rx_buf[0];
            // Option 3: Big-endian starting at byte 1
            uint32_t dev_id_v3 = (rx_buf[1] << 24) | (rx_buf[2] << 16) | (rx_buf[3] << 8) | rx_buf[4];
            
            printk("  SUCCESS! Device ID interpretations:\n");
            printk("    v1 (LE, byte 1-4): 0x%08X\n", dev_id_v1);
            printk("    v2 (LE, byte 0-3): 0x%08X\n", dev_id_v2);
            printk("    v3 (BE, byte 1-4): 0x%08X\n", dev_id_v3);
            
            // Expected DW3110 ID is 0xDECA0130
            if (dev_id_v1 == 0xDECA0130 || dev_id_v2 == 0xDECA0130 || dev_id_v3 == 0xDECA0130) {
                printk("  ✓ VERIFIED: DW3110 Device ID matches!\n");
                return 0xDECA0130;
            }
            
            // Return the most likely candidate (v1 is standard for Decawave)
            return dev_id_v1;
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
 * Initialize UWB driver and DW3110 transceiver
 * @return 0 on success, negative error code on failure
 */
int uwb_driver_init(void)
{
    int ret;
    
    LOG_INF("=== DW3000 UWB Driver Init (Decawave SDK Pattern) ===");
    
    // Check SPI bus
    if (!device_is_ready(spi_dev)) {
        LOG_ERR("SPI bus not ready");
        return -ENODEV;
    }
    LOG_INF("SPI bus ready");
    
    // Configure CS pin manually (HIGH = deselected)
#if HAS_CS_GPIO
    if (!device_is_ready(dw3000_cs.port)) {
        LOG_ERR("CS GPIO port not ready");
        return -ENODEV;
    }
    
    ret = gpio_pin_configure_dt(&dw3000_cs, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure CS pin: %d", ret);
        return ret;
    }
    gpio_pin_set_dt(&dw3000_cs, 1);  // CS HIGH = deselected
    LOG_INF("CS pin configured (manual control, P0.02)");
#else
    LOG_ERR("No CS GPIO defined - cannot continue");
    return -ENODEV;
#endif
    
    /* Execute proper power-on sequence with all control pins */
    printk("Executing DW3110 power-on sequence...\n");
    ret = dw3000_power_on_sequence();
    if (ret != 0) {
        printk("Power-on sequence failed: %d\n", ret);
        return ret;
    }
    
    /* Small additional delay for SPI to stabilize */
    k_msleep(10);
    
    printk("=== CHECKPOINT A ===\n");
    printk("Reading Device ID after proper initialization...\n");
    
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
