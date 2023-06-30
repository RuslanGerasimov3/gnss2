#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

// Simulated NMEA data
const char* simulatedData[] = {
    "$GPGGA,123456.789,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
    "$GPGSA,A,3,05,07,08,12,19,26,28,,,,,,2.4,1.2,2.0*39",
    "$GPZDA,123456.789,04,07,2023,00,00*6A",
    "$GPGGA,223456.789,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*48",
    "$GPGSA,A,3,05,07,08,12,19,26,28,,,,,,2.4,1.2,2.0*3A",
    "$GPZDA,223456.789,04,07,2023,00,00*6B"
};

void gnss_uart_rx(uint8_t data) {
    // Simulate UART reception by printing received data
    putchar(data);
}

/*
// GNSS module simulation task
void gnssModuleTask(void* parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (1) {
        // Simulate UART RX interrupt by transmitting simulated NMEA data
        for (int i = 0; i < sizeof(simulatedData) / sizeof(simulatedData[0]); i++) {
            printf("\n");
            const char* nmeaLine = simulatedData[i];
            size_t lineLength = strlen(nmeaLine);

            for (size_t j = 0; j < lineLength; j++) {
                gnss_uart_rx(nmeaLine[j]); // Simulate UART RX interrupt
            }

            vTaskDelay(pdMS_TO_TICKS(1000)); // Transmit a burst every second
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(1000));
    }
}
*/

// GNSS module simulation task
void gnssModuleTask(void* parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (1) {
        // Transmit burst of NMEA lines every second
        for (int i = 0; i < sizeof(simulatedData) / sizeof(simulatedData[0]); i++) {
            const char* nmeaLine = simulatedData[i];

            // Simulate UART transmission by printing the NMEA line
            printf("%s\n", nmeaLine);

            // Check if it's the last line of the burst
            if (strstr(nmeaLine, "$GPZDA") != NULL) {
                // Wait for 1 second before transmitting the next burst
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(1000));
    }
}

// NMEA parser task
void nmeaParserTask(void* parameter) {
    while (1) {
        // TODO: Read data received from gnss_uart_rx() function

        // TODO: Discard boot info

        // TODO: Verify checksum of NMEA messages

        // TODO: Parse fix mode, position, and time

        // TODO: Handle parsed data or discard NMEA message

        vTaskDelay(pdMS_TO_TICKS(10)); // Adjust the delay according to your requirements
    }
}

// Consumer task
void consumerTask(void* parameter) {
    while (1) {
        // TODO: Wait for notifications from the NMEA parser task

        // TODO: Print notifications in real-time using printf()

        vTaskDelay(pdMS_TO_TICKS(10)); // Adjust the delay according to your requirements
    }
}

int main() {
    // Create tasks
    xTaskCreate(gnssModuleTask, "GNSS Module Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(nmeaParserTask, "NMEA Parser Task", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    xTaskCreate(consumerTask, "Consumer Task", configMINIMAL_STACK_SIZE, NULL, 3, NULL);

    // Start the FreeRTOS scheduler
    vTaskStartScheduler();

    // The scheduler should never return, but if it does, handle the error
    while (1) {
        // Handle error
    }

    return 0;
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vMainQueueSendPassed(void)
{
    /* This is just an example implementation of the "queue send" trace hook. */
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vApplicationIdleHook(void)
{
#ifdef __GCC_POSIX__
    struct timespec xTimeToSleep, xTimeSlept;
    /* Makes the process more agreeable when using the Posix simulator. */
    xTimeToSleep.tv_sec = 1;
    xTimeToSleep.tv_nsec = 0;
    nanosleep(&xTimeToSleep, &xTimeSlept);
#endif
}

