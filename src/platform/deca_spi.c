#include "deca_spi.h"
#include "port.h"
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(uwb_port);

extern struct spi_dt_spec spi_spec;

int32_t writetospi(uint16_t headerLength, const uint8_t *headerBuffer, uint16_t bodyLength, const uint8_t *bodyBuffer)
{
    struct spi_buf buf[2];
    struct spi_buf_set tx = {
        .buffers = buf,
        .count = 2
    };

    buf[0].buf = (void *)headerBuffer;
    buf[0].len = headerLength;
    buf[1].buf = (void *)bodyBuffer;
    buf[1].len = bodyLength;

    if (bodyLength == 0) {
        tx.count = 1;
    }

    return spi_write_dt(&spi_spec, &tx);
}

int32_t readfromspi(uint16_t headerLength, uint8_t *headerBuffer, uint16_t readLength, uint8_t *readBuffer)
{
    struct spi_buf tx_buf;
    struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1
    };
    struct spi_buf rx_buf[2];
    struct spi_buf_set rx = {
        .buffers = rx_buf,
        .count = 2
    };

    tx_buf.buf = headerBuffer;
    tx_buf.len = headerLength;

    /* Skip the header in RX */
    rx_buf[0].buf = NULL;
    rx_buf[0].len = headerLength;
    rx_buf[1].buf = readBuffer;
    rx_buf[1].len = readLength;

    int ret = spi_transceive_dt(&spi_spec, &tx, &rx);
    
    if (readLength >= 4) {
        LOG_INF("SPI Read: Len=%d, Data=0x%02X 0x%02X 0x%02X 0x%02X", 
                readLength, readBuffer[0], readBuffer[1], readBuffer[2], readBuffer[3]);
    }

    return ret;
}

int32_t writetospiwithcrc(uint16_t headerLength, const uint8_t *headerBuffer, uint16_t bodyLength, const uint8_t *bodyBuffer, uint8_t crc8)
{
    struct spi_buf buf[3];
    struct spi_buf_set tx = {
        .buffers = buf,
        .count = 3
    };

    buf[0].buf = (void *)headerBuffer;
    buf[0].len = headerLength;
    buf[1].buf = (void *)bodyBuffer;
    buf[1].len = bodyLength;
    buf[2].buf = &crc8;
    buf[2].len = 1;

    return spi_write_dt(&spi_spec, &tx);
}
