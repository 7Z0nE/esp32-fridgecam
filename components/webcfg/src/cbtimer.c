#include "cbtimer.h"
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

// PRIVATE
static void cbtimer_task(void* args);

static const char* TAG = "cbtimer";

static void cbtimer_task(void* args)
{
	cbtimer_t* timer = (cbtimer_t*) args;
	ESP_LOGI(TAG, "Callback scheduled for %li seconds", timer->end);
	while(1) {
		time_t current_time = time(NULL);
		time_t time_left = timer->end - current_time; 
		if (time_left <= 0) {
			ESP_LOGI(TAG, "fired, calling callback");
			(*(timer->callback))(timer->args);
			ESP_LOGI(TAG, "cleaning timer");
			cbtimer_clean(timer);
		}
		else {
			TickType_t delay_ticks = 2000 / portTICK_PERIOD_MS; //current_time * 1000 / portTICK_PERIOD_MS;
			ESP_LOGI(TAG, "Timer has %li seconds left, delaying by %i ticks", time_left, delay_ticks);
			vTaskDelay(delay_ticks);
		}
	}
}

cbtimer_t* cbtimer_create(long seconds, void (*callback)(void* args), void* args)
{
	time_t current_time = time(NULL);
	cbtimer_t* timer = malloc(sizeof(cbtimer_t));
	timer->end = current_time + seconds;
	timer->args = args;
	timer->callback = callback;
	BaseType_t err = xTaskCreate(
		cbtimer_task,
		"timer",
		4096,
		timer,
		uxTaskPriorityGet(NULL),
		&(timer->task));
	if (err != pdPASS) {
		ESP_LOGI(TAG, "Failed to create timer task");
	}
	return timer;
}

void cbtimer_clean(cbtimer_t* timer)
{
	vTaskDelete(timer->task);
	free(timer);
}