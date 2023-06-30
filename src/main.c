#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define QUEUE_LENGTH 10
#define MAX_NMEA_LINE_LENGTH 128

// Simulated NMEA data
const char* simulatedData[] = {
    "$GPGGA,123456.789,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
    "$GPGSA,A,3,05,07,08,12,19,26,28,,,,,,2.4,1.2,2.0*39",
    "$GPZDA,123456.789,04,07,2023,00,00*6A",
    "$GPGGA,223456.789,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*48",
    "$GPGSA,A,3,05,07,08,12,19,26,28,,,,,,2.4,1.2,2.0*3A",
    "$GPZDA,223456.789,04,07,2023,00,00*6B"
};

QueueHandle_t nmeaQueue;

// GNSS module simulation task
void gnssModuleTask(void* parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (1) {
        // Transmit burst of NMEA lines every second
        for (int i = 0; i < sizeof(simulatedData) / sizeof(simulatedData[0]); i++) {
            const char* nmeaLine = simulatedData[i];
            size_t lineLength = strlen(nmeaLine);

            // Send the NMEA line as a message through the message queue
            for (size_t j = 0; j < lineLength; j++) {
                char data = nmeaLine[j];
                xQueueSend(nmeaQueue, &data, portMAX_DELAY);
            }

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
    char nmeaLine[MAX_NMEA_LINE_LENGTH];
    size_t lineIndex = 0;

    while (1) {
        // Receive the NMEA line from the message queue
        char data;
        if (xQueueReceive(nmeaQueue, &data, portMAX_DELAY) == pdTRUE) {
            // Store the received character in the line buffer
            nmeaLine[lineIndex++] = data;

            // Check if it's the end of the line or the buffer is full
            if (data == '\n' || lineIndex >= MAX_NMEA_LINE_LENGTH) {
                // Process the complete NMEA line
                // TODO: Implement NMEA line processing and parsing logic here

                // Reset the line buffer index
                lineIndex = 0;
            }
        }
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
    // Create the message queue
    nmeaQueue = xQueueCreate(QUEUE_LENGTH, sizeof(char));

    // Create the GNSS module task
    xTaskCreate(gnssModuleTask, "GNSS Module Task", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    // Create the NMEA parser task
    xTaskCreate(nmeaParserTask, "NMEA Parser Task", configMINIMAL_STACK_SIZE, NULL, 2, NULL);

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

