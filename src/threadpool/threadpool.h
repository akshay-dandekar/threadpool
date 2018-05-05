#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <pthread.h>
#include "linked_list/linked_list_s.h"

enum threadpool_task_state {
	THREADPOOL_TASK_STATE_NEW
	, THREADPOOL_TASK_STATE_QUEUED
	, THREADPOOL_TASK_STATE_RUNNING
	, THREADPOOL_TASK_STATE_INTERRUPTED
	, THREADPOOL_TASK_STATE_COMPLETED
};

struct threadpool_task;

/*
 *	Threadpool context for managing threadpool
 */
struct threadpool {
	struct linked_list_s threads_available;
	struct linked_list_s threads_busy;

	struct linked_list_s tasks;

	pthread_t th_manager;
	int is_interrupted;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

struct threadpool_thread {
	struct threadpool *ctx;
	pthread_t tid;
	pthread_mutex_t mutex;
	pthread_cond_t cond;

	struct threadpool_task *assigned_task;
	int is_interrupted;
};

struct threadpool_task {
	void * (*run)(void *);
	void * data;
	void * ret;

	int state;
	struct threadpool_thread *th;

	// TODO Implement a async completetion callback
};

/*
 *	This function creates a threadpool with num_threads created.
 *	Once threadpool is created, tasks can be sumitted using submit method
 *
 */
int threadpool_create(struct threadpool **ctx, int num_threads);

int threadpool_destroy(struct threadpool *ctx);

int threadpool_task_create(struct threadpool_task **task
		, void * (*run)(void *)
		, void *data);

int threadpool_task_destroy(struct threadpool_task *task);

int threadpool_submit(struct threadpool *ctx, struct threadpool_task *task);

int threadpool_task_is_completed(struct threadpool_task *task);

int threadpool_task_wait_complete(struct threadpool_task *task);

#endif /* _THREADPOOL_H_ */
