#ifndef INCLUDE_CBTIMER
#define INCLUDE_CBTIMER
#include "time.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task.h"
#include "esp_system.h"

typedef struct {
	void (*callback)(void* args);
	void* args;
	time_t end;
	TaskHandle_t task;
} cbtimer_t;

cbtimer_t* cbtimer_create(long seconds, void (*callback)(void* args), void* args);
void cbtimer_clean(cbtimer_t* timer);

#endif