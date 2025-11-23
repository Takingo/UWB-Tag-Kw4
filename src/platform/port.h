#ifndef PORT_H_
#define PORT_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

/* DW3000 Device Tree Node */
#define DW3000_NODE DT_NODELABEL(dw3210)

/* SPI Configuration */
extern const struct spi_dt_spec dw3000_spi;

/* GPIO Configuration */
extern const struct gpio_dt_spec dw3000_irq;
extern const struct gpio_dt_spec dw3000_reset;

/* Function prototypes */
void port_init(void);
void reset_DW3000(void);
void deca_mutex_init(void);
void port_set_dw_ic_spi_slowrate(void);
void port_set_dw_ic_spi_fastrate(void);

#endif /* PORT_H_ */
