#include "deca_spi.h"
#include <zephyr/kernel.h>
#include <stdbool.h>

static struct k_mutex dw_mutex;
static bool mutex_init = false;

void deca_mutex_init(void)
{
    if (!mutex_init) {
        k_mutex_init(&dw_mutex);
        mutex_init = true;
    }
}

dw_hal_lockTypeDef deca_mutex_lock(void)
{
    if (k_mutex_lock(&dw_mutex, K_FOREVER) == 0) {
        return DW_HAL_NODE_UNLOCKED;
    }
    return DW_HAL_NODE_LOCKED;
}

void deca_mutex_unlock(void)
{
    k_mutex_unlock(&dw_mutex);
}
