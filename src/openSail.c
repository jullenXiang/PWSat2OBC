#include <FreeRTOS.h>
#include <task.h>
#include <em_i2c.h>
#include <em_cmu.h>
#include <em_gpio.h>

#include <swo/swo.h>

#include "io_map.h"
#include "obc_time.h"
#include "devices/eps.h"

#define SAILOPENTIME 2500

static void OpenSail(void)
{
    EpsOpenSail();
}

static void CheckOpenSail(void* _)
{
    (void)_;

    while (1)
    {
        uint32_t time = CurrentTime();
        if (time > SAILOPENTIME)
        {
            SwoPuts("time to open sail.");

            OpenSail();

            while (1)
            {
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void OpenSailInit(void)
{
    xTaskCreate(CheckOpenSail, "openSail", 1024, NULL, 4, NULL);
}
