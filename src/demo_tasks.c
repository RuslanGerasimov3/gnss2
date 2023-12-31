/**
 * @file demo_tasks.c
 * @author Alex Hoffman
 * @date 23 January 2023
 * @brief Demo tasks created to illustrate the emulator's functionalities

 *
 * @verbatim
 ----------------------------------------------------------------------
 Copyright (C) Alexander Hoffman, 2023
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ----------------------------------------------------------------------
 @endverbatim
 */

#include "gfx_event.h"
#include "gfx_ball.h"
#include "gfx_sound.h"
#include "gfx_utils.h"
#include "gfx_print.h"

#include "main.h"
#include "demo_tasks.h"
#include "async_message_queues.h"
#include "async_sockets.h"
#include "buttons.h"
#include "state_machine.h"
#include "draw.h"

#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)

TaskHandle_t DemoTask1 = NULL;
TaskHandle_t DemoTask2 = NULL;
TaskHandle_t DemoSendTask = NULL;

struct locked_ball {
    ball_t *ball;
    SemaphoreHandle_t lock;
} my_ball = { 0 };

void vStateOneEnter(void)
{
    vTaskResume(DemoTask1);
}

void vStateOneExit(void)
{
    vTaskSuspend(DemoTask1);
}

void vDemoTask1(void *pvParameters)
{
    vDrawInitResources();

    TickType_t xLastResetTime = xTaskGetTickCount();
    TickType_t xLastFrameTime = xTaskGetTickCount();

    while (1) {
        if (DrawSignal)
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                gfxEventFetchEvents(FETCH_EVENT_BLOCK |
                                    FETCH_EVENT_NO_GL_CHECK);
                vGetButtonInput(); // Update global input

                vDrawClearScreen();
                vDrawStaticItems();
                vDrawMouseBallAndBoundingBox(
                    gfxEventGetMouseLeft());
                vDrawButtonText();
                vDrawSpriteStatic();

                // Reset the downwards animation sequence every 500ms
                if ((xTaskGetTickCount() - xLastResetTime) > 500) {
                    xLastResetTime = xTaskGetTickCount();
                    vDrawSpriteResetDownwardSequence();
                }
                vDrawSpriteAnimations(xLastFrameTime);

                xLastFrameTime = xTaskGetTickCount();

                // Draw FPS in lower right corner
                vDrawFPS();

                // Get input and check for state change
                vCheckStateInput();
            }
    }
}

void vPlayBallSound(void *args)
{
    gfxSoundPlaySample(a3);
}

void vResetBall(void)
{
    if (xSemaphoreTake(my_ball.lock, portMAX_DELAY) == pdTRUE) {
        gfxSetBallSpeed(my_ball.ball, 100, 100, 0, SET_BALL_SPEED_AXES);
        gfxSetBallLocation(my_ball.ball, SCREEN_WIDTH / 2,
                           SCREEN_HEIGHT / 2);
        xSemaphoreGive(my_ball.lock);
    }
}

void vStateTwoInit(void)
{
    my_ball.lock = xSemaphoreCreateMutex();
    if (xSemaphoreTake(my_ball.lock, portMAX_DELAY) == pdTRUE) {
        my_ball.ball =
            gfxCreateBall(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, Black,
                          20, 1000, &vPlayBallSound, NULL, NULL);
        xSemaphoreGive(my_ball.lock);
        vResetBall();
    }
}

void vStateTwoEnter(void)
{
    vResetBall();
    vTaskResume(DemoTask2);
}

void vStateTwoExit(void)
{
    vTaskSuspend(DemoTask2);
}

void vDemoTask2(void *pvParameters)
{
    TickType_t xLastWakeTime, prevWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    prevWakeTime = xLastWakeTime;

    wall_t *left_wall = NULL, *right_wall = NULL, *top_wall = NULL,
            *bottom_wall = NULL;
    vCreateWalls(&left_wall, &right_wall, &top_wall, &bottom_wall);

    while (1) {
        if (DrawSignal)
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                xLastWakeTime = xTaskGetTickCount();

                vGetButtonInput(); // Update global button data

                vDrawClearScreen();
                vDrawStaticItems();
                vDrawWalls(left_wall, right_wall, top_wall,
                           bottom_wall);

                if (xSemaphoreTake(my_ball.lock,
                                   portMAX_DELAY) == pdTRUE) {
                    // Check if ball has made a collision
                    if (gfxCheckBallCollisions(my_ball.ball,
                                               NULL, NULL)) {
                        prints("Collision\n");
                    }

                    // Update the balls position now that possible collisions have
                    // updated its speeds
                    gfxUpdateBallPosition(
                        my_ball.ball,
                        xLastWakeTime - prevWakeTime);

                    vDrawBall(my_ball.ball);

                    xSemaphoreGive(my_ball.lock);
                }

                // Draw FPS in lower right corner
                vDrawFPS();

                // Check for state change
                vCheckStateInput();

                // Keep track of when task last ran so that you know how many ticks
                //(in our case miliseconds) have passed so that the balls position
                // can be updated appropriatley
                prevWakeTime = xLastWakeTime;
            }
    }
}

void vDemoSendTask(void *pvParameters)
{
    static char *test_str_1 = "UDP test 1";

    struct __attribute__((__packed__)) my_item {
        char populated;
        char x;
        char y;
    };

    // Attribute packed will guarentee that the compiler
    // doesnt pad the struct to align members with address
    // doundaries. Explination can be found here
    // https://stackoverflow.com/questions/4306186/structure-padding-and-packing
    struct __attribute__((__packed__)) my_struct {
        int my_int;
        char my_string[10];
        struct common_struct my_common_struct;
        struct my_item my_items[3];
    };

    struct my_struct test_struct = {
        .my_int = 85,
        .my_string = "testing",
        .my_common_struct = {.first_int = 420, .second_int = 100},
        .my_items = { { .populated = 1, .x = 10, .y = 15 },
            { .populated = 1, .x = 50, .y = 100 },
            { .populated = 0 }
        }
    };

    static char *test_str_2 = "TCP test";

    while (1) {
        prints("*****TICK******\n");
        if (mq_one) {
            aIOMessageQueuePut(mq_one_name, "Hello MQ one");
        }
        if (mq_two) {
            aIOMessageQueuePut(mq_two_name, "Hello MQ two");
        }

        if (udp_soc_one)
            aIOSocketPut(UDP, NULL, UDP_TEST_PORT_1, test_str_1,
                         strlen(test_str_1));
        if (udp_soc_two)
            aIOSocketPut(UDP, NULL, UDP_TEST_PORT_2, (char *) &test_struct,
                         sizeof(test_struct));
        if (tcp_soc)
            aIOSocketPut(TCP, NULL, TCP_TEST_PORT, test_str_2,
                         strlen(test_str_2));

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int xCreateDemoTasks(void)
{
    if (xTaskCreate(vDemoTask1, "DemoTask1", mainGENERIC_STACK_SIZE * 2,
                    NULL, mainGENERIC_PRIORITY + 1, &DemoTask1) != pdPASS) {
        PRINT_TASK_ERROR("DemoTask1");
        goto err_task1;
    }
    if (xTaskCreate(vDemoTask2, "DemoTask2", mainGENERIC_STACK_SIZE * 2,
                    NULL, mainGENERIC_PRIORITY + 1, &DemoTask2) != pdPASS) {
        PRINT_TASK_ERROR("DemoTask2");
        goto err_task2;
    }

    if (xTaskCreate(vDemoSendTask, "DemoSendTask",
                    mainGENERIC_STACK_SIZE * 2, NULL,
                    configMAX_PRIORITIES - 1, &DemoSendTask) != pdPASS) {
        PRINT_TASK_ERROR("DemoSendTask");
        goto err_send_task;
    }

    vTaskSuspend(DemoTask1);
    vTaskSuspend(DemoTask2);

    return 0;

err_send_task:
    vTaskDelete(DemoTask2);
err_task2:
    vTaskDelete(DemoTask1);
err_task1:
    return -1;
}

void vDeleteDemoTasks(void)
{
    if (DemoTask1) {
        vTaskDelete(DemoTask1);
    }
    if (DemoTask2) {
        vTaskDelete(DemoTask2);
    }
    if (DemoSendTask) {
        vTaskDelete(DemoSendTask);
    }
}
