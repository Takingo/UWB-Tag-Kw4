#ifndef _DECA_SPI_H_
#define _DECA_SPI_H_

#include <stdint.h>

typedef enum
{
    DW_HAL_NODE_UNLOCKED = 0,
    DW_HAL_NODE_LOCKED = 1
} dw_hal_lockTypeDef;

int32_t writetospi(uint16_t headerLength, const uint8_t *headerBuffer, uint16_t bodyLength, const uint8_t *bodyBuffer);
int32_t readfromspi(uint16_t headerLength, uint8_t *headerBuffer, uint16_t readLength, uint8_t *readBuffer);
int32_t writetospiwithcrc(uint16_t headerLength, const uint8_t *headerBuffer, uint16_t bodyLength, const uint8_t *bodyBuffer, uint8_t crc8);

void port_set_dw_ic_spi_slowrate(void);
void port_set_dw_ic_spi_fastrate(void);

#endif
