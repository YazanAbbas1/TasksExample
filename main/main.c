
#include <stdbool.h>
#include <unistd.h>
#include"sdkconfig.h"
#include"freertos/FreeRTOS.h"
#include"freertos/task.h"
#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/semphr.h"

#define STACK_SIZE 1024*2
#define PRIO_LVL_2  2
#define ARR_SIZE    10

static SemaphoreHandle_t MyMutex,MySecMux, ThirdMux;
int * arr;
int * arrCPY;
BaseType_t ReadTask, ProcessTask, SendTask;
TaskHandle_t ReadHandle = NULL, ProcessHandle= NULL,SendHandle= NULL;

// Green : read
// Blue : process
// Yellow : send

/* Sending and processing should wait till the data are received.
*  When the data are recived then the 
*/ 

static void ArrProcess(int * src, int * dst)
{
    for(int i = 0 ; i < ARR_SIZE ; i ++)
    {                                      //i= 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        dst[i] = src[i] + (ARR_SIZE % (i+1)); // 0 , 1, 0, 1, 4, 3, 2, 1, 0,
    }                               // arr =    0 , 2, 2, 4, 8, 8, 8, 8, 9
}

static void printArr(int * arr, int size)
{
    
    for(int i = 0 ;  i < size ; i++)
        printf("%d,  ",arr[i]);
    printf("\n");
    
}
static void ReadHandler(void * pvParameter)
{
    while(true)
    {
        
        if(xSemaphoreTake(MyMutex,0)==pdTRUE)
        {
            printf("Read Task : \n");
            vTaskDelay(100);

            for(int i =1 ; i< ARR_SIZE ; i++)
                arr[i] = arr[i-1] + 1; 
        
            printArr(arr,ARR_SIZE);
            xSemaphoreGive(MyMutex);
            xTaskNotify(ProcessHandle, 0, eNoAction);
        }
    }
    vTaskDelete(NULL);
}
static void ProcessHandler(void * pvParameter)
{
    
    while(true)
    {
        xTaskNotifyWait(0,0,NULL,portMAX_DELAY);
        if(  xSemaphoreTake(MyMutex,0)==pdTRUE)
       {
           // xTaskNotifyWait(0,0,NULL,portMAX_DELAY);
            printf("Processing Task: \n");
            vTaskDelay(100);

            for(int i =0; i< ARR_SIZE ; i++)
                arr[i] = arr[i] * 2;

            printArr(arr,ARR_SIZE);
            
            xSemaphoreGive(MyMutex);
            xTaskNotify(SendHandle, 0, eNoAction);
       }
    }
   vTaskDelete(NULL);
    
}
static void SendHandler(void * pvParameter)
{
    
    while(true)
    {
        xTaskNotifyWait(0,0,NULL, portMAX_DELAY);
        if(xSemaphoreTake(MyMutex,0)==pdTRUE)
        {
            
            printf("Send Task: \n");
            vTaskDelay(100);
            printArr(arr,ARR_SIZE);
            xSemaphoreGive(MyMutex);
        }
    }
    vTaskDelete(NULL);
}
/*
* the only problem with this approach is that if there will be tasks that have the same priority
* starvation will occure.
*/
void app_main(void)
{
   
    arr = (int *)calloc(ARR_SIZE , sizeof(int));
    arrCPY = (int *)calloc(ARR_SIZE , sizeof(int));
    MyMutex = xSemaphoreCreateMutex();
    MySecMux = xSemaphoreCreateMutex();
    ThirdMux = xSemaphoreCreateMutex();

    ReadTask = xTaskCreate(ReadHandler, "GreenTask",STACK_SIZE, NULL, PRIO_LVL_2,&ReadHandle );
    configASSERT(ReadTask == pdPASS);
    ProcessTask = xTaskCreate(ProcessHandler, "BlueTask",STACK_SIZE,NULL,3,&ProcessHandle );
    configASSERT(ProcessTask == pdPASS);
    SendTask=xTaskCreate(SendHandler,"YellowTask",STACK_SIZE,NULL,4,&SendHandle);
    configASSERT(SendTask == pdPASS);
    vTaskStartScheduler();
  
}


// Trying to start from this skeleton to schedule the three different led tasks.
