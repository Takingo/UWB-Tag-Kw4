#include "deca_device_api.h"
#include <zephyr/kernel.h>

void deca_sleep(unsigned int time_ms)
{
    k_msleep(time_ms);
}

void deca_usleep(unsigned long time_us)
{
    k_usleep(time_us);
}
