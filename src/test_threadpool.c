#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#include "threadpool/threadpool.h"

void print_usage(char *prg_name) {
	printf("[USAGE] %s\r\n", prg_name);
}

void * task_method(void *ptr) {
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	static int task_count = 0;
	int current_taskid;

	int sleep_time = *((int *) ptr);
	pthread_t my_tid;
	time_t cur_time;
	char str[100];

	my_tid = pthread_self();

	/* Get taskid */
	pthread_mutex_lock(&mutex);
	current_taskid = ++task_count;
	pthread_mutex_unlock(&mutex);

	/* Get current time and print to a string */
	cur_time = time(NULL);
	snprintf(str, sizeof(str), "%s", ctime(&cur_time));
	printf("[%s, INFO] (%.*s) (%d, %ld) Sleeping for %d seconds\r\n"
			, __FUNCTION__, (int) (strlen(str) - 1), str, current_taskid
			, my_tid, sleep_time);

	/* Sleep */
	sleep(sleep_time);
	free(ptr);

	/* Get current time and print to a string */
	cur_time = time(NULL);
	snprintf(str, sizeof(str), "%s", ctime(&cur_time));
	printf("[%s, INFO] (%.*s) (%d, %ld) Task completed after %d seconds\r\n"
			, __FUNCTION__, (int) (strlen(str) - 1), str, current_taskid
			, my_tid, sleep_time);

	return NULL;
}

int main(int argc, char *argv[]) {
	int ret, count, i;
	int *sleep_time;
	struct threadpool *tp_ctx;
	struct threadpool_task *tasks[6];

	if(argc != 1) {
		fprintf(stderr, "Invalid number of arguments\r\n");
		return 1;
	}

	printf("Thread pool demo\r\n");

	/* Create a new threadpool */
	printf("Creating a threadpool\r\n");
	ret = threadpool_create(&tp_ctx, 15);
	if(ret != 0) {
		fprintf(stderr, "[%s, ERROR] Unable to create new threadpool\r\n"
				, __FUNCTION__);
		return 1;
	}

	printf("Sleeping for 10 sec\r\n");
	sleep(1);

	for(i = 0; i < 5; i++) {

		/* Create a task */
		printf("Creating a task number %d\r\n", (i + 1));
		sleep_time = (int *) malloc(sizeof(int));
		if(sleep_time == NULL) {
			fprintf(stderr, "Unable to create the task data\r\n");
		
			/* Destroy created threadpool */
			threadpool_destroy(tp_ctx);
			return 2;
		}

		*sleep_time = 15 + (2 * i);
		threadpool_task_create(&tasks[i], &task_method, sleep_time);

		/* Submit 10 tasks */
		printf("Submitting task\r\n");
		threadpool_submit(tp_ctx, tasks[i]);
	}

	count = 0;
	while(1) {
		sleep(6);
		count++;
		if(count > 2) {
			break;
		}
	}

	/* Destroy created threadpool */
	printf("[%s, INFO] Destroy threadpool\r\n", __FUNCTION__);
	threadpool_destroy(tp_ctx);

	/* Clear allocated tasks */
	for(i = 0; i < 5; i++) {
		threadpool_task_destroy(tasks[i]);
	}

	return 0;

}
