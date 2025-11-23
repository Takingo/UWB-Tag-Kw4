#include "port.h"
#include "deca_spi.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uwb_port, CONFIG_LOG_DEFAULT_LEVEL);

const struct spi_dt_spec dw3000_spi = SPI_DT_SPEC_GET(DW3000_NODE, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0);
const struct gpio_dt_spec dw3000_irq = GPIO_DT_SPEC_GET(DW3000_NODE, irq_gpios);
const struct gpio_dt_spec dw3000_reset = GPIO_DT_SPEC_GET(DW3000_NODE, reset_gpios);

/* Mutable SPI spec for changing frequency */
struct spi_dt_spec spi_spec;

void port_init(void)
{
    LOG_INF("Port Init: Entry");
    // k_msleep(100);

    /* Initialize Mutex */
    // LOG_INF("Port Init: Calling deca_mutex_init...");
    // deca_mutex_init();
    // LOG_INF("Port Init: Mutex initialized");
    // k_msleep(10);

    /* Manually copy fields */
    LOG_INF("Port Init: Copying SPI bus...");
    if (dw3000_spi.bus == NULL) {
        LOG_ERR("FATAL: SPI Bus Pointer is NULL!");
        return;
    }
    spi_spec.bus = dw3000_spi.bus;
    
    LOG_INF("Port Init: Copying SPI config...");
    spi_spec.config = dw3000_spi.config;
    
    LOG_INF("Port Init: SPI Spec manually copied");
    k_msleep(10);

    LOG_INF("Port Init: Checking Reset GPIO...");
    /* Configure Reset Pin */
    if (!gpio_is_ready_dt(&dw3000_reset)) {
        LOG_ERR("Reset GPIO not ready");
        // Don't return, try to continue to see if SPI works
    } else {
        LOG_INF("Port Init: Reset GPIO ready");
        int ret = gpio_pin_configure_dt(&dw3000_reset, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            LOG_ERR("Failed to configure Reset GPIO");
        }
        LOG_INF("Port Init: Reset GPIO configured");
    }

    /* Set Reset High (Inactive for Active Low?) */
    /* Usually Reset is Active Low, so setting it to 0 asserts reset, 1 releases it. */
    /* But GPIO_ACTIVE_LOW flag in DTS means logical 1 sets pin low. */
    /* Let's assume we want to release reset initially. */
    // gpio_pin_set_dt(&dw3000_reset, 0); 

    LOG_INF("Port Init: Checking IRQ GPIO...");
    /* Configure IRQ Pin */
    if (!gpio_is_ready_dt(&dw3000_irq)) {
        LOG_ERR("IRQ GPIO not ready");
        // return;
    } else {
        int ret = gpio_pin_configure_dt(&dw3000_irq, GPIO_INPUT);
        if (ret < 0) {
            LOG_ERR("Failed to configure IRQ GPIO");
        }
        LOG_INF("Port Init: IRQ GPIO configured");
    }

    /* SPI is initialized by Zephyr */
    LOG_INF("Port Init: Checking SPI Bus...");
    if (!spi_is_ready_dt(&dw3000_spi)) {
        LOG_ERR("SPI bus not ready");
    } else {
        LOG_INF("SPI bus ready");
    }
    
    LOG_INF("Port Init: Completed");
}

void reset_DW3000(void)
{
    LOG_INF("Resetting DW3000 (Active Low)...");
    
    /* Try releasing reset first (in case it was held) */
    gpio_pin_set_dt(&dw3000_reset, 0);
    k_msleep(10);
    
    /* Assert Reset (Active Low means 1 = reset) */
    gpio_pin_set_dt(&dw3000_reset, 1);
    k_msleep(50);  // Hold in reset longer
    
    /* Release Reset */
    gpio_pin_set_dt(&dw3000_reset, 0);
    k_msleep(100);  // Wait longer for boot
    
    LOG_INF("Reset sequence: 10ms release, 50ms assert, 100ms boot wait");
}

void port_set_dw_ic_spi_slowrate(void)
{
    spi_spec.config.frequency = 2000000; /* 2 MHz */
}

void port_set_dw_ic_spi_fastrate(void)
{
    spi_spec.config.frequency = 8000000; /* 8 MHz */
}
