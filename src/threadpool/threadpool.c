#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "threadpool.h"

#if DEBUG > 6
static void print_thread_list(struct linked_list_s *list) {
	int i, list_size, size;
	struct threadpool_thread *th;

	list_size = linked_list_s_size(list);
	for(i = 0; i < list_size; i++) {
		linked_list_s_get_data(list, i, (void **) &th, &size);

		printf("Thread ID: %ld\r\n", th->tid);
	}
}
#endif

static int thread_switch_to_available(struct threadpool_thread *th) {
	int ret;

	/* Aquire mutex for thread */
	pthread_mutex_lock(&th->mutex);

	/* Check if the task is completed */
	if(th->assigned_task->state != THREADPOOL_TASK_STATE_COMPLETED) {
		/* Release mutex for thread */
		pthread_mutex_unlock(&th->mutex);

#ifdef DEBUG
		fprintf(stderr, "[%s, ERROR] Task still not completed for thread"
				" %ld\r\n", __FUNCTION__, th->tid);
#endif

		return -1;
	}

	/* Clear the assigned task from thread */
	th->assigned_task = NULL;

	/* Release mutex for thread */
	pthread_mutex_unlock(&th->mutex);

#if DEBUG > 3
	printf("[%s, INFO] Transfering thread %ld to available list\r\n"
			, __FUNCTION__, th->tid);
#endif

	/* Transfer thread to available threads list */
	pthread_mutex_lock(&th->ctx->mutex);

	/* Find and delete current thread's node in busy threads list */
	ret = linked_list_s_find_node(&th->ctx->threads_busy, (void *) th
			, sizeof(struct threadpool_thread));
	if(ret == -1) {
#ifdef DEBUG
		fprintf(stderr, "[%s, ERROR] Unable to find specified thread in busy "
				"threads list\r\n", __FUNCTION__);
#endif
		pthread_mutex_unlock(&th->ctx->mutex);
		return -1;
	}
	linked_list_s_del(&th->ctx->threads_busy, ret);

	/* Add thread to the available threads list */
	linked_list_s_add(&th->ctx->threads_available, -1, (void *) th
			, sizeof(struct threadpool_thread));

#if DEBUG > 6
	printf("[%s, INFO] List of available threads\r\n", __FUNCTION__);
	print_thread_list(&th->ctx->threads_available);
#endif

	pthread_mutex_unlock(&th->ctx->mutex);

#if DEBUG > 6
	printf("[%s, INFO] Transfered %ld\r\n", __FUNCTION__, th->tid);
#endif

	/* Signal manager thread to inform about available thread */
	pthread_cond_signal(&th->ctx->cond);

	return 0;
}

static int thread_assign_tasks(struct threadpool *ctx) {
	int size;
	struct threadpool_task *task;
	struct threadpool_thread *th;

#if DEBUG > 6
	printf("[%s, INFO] Assigning tasks\r\n", __FUNCTION__);
#endif

	while(1) {
		pthread_mutex_lock(&ctx->mutex);
		
		if(linked_list_s_is_empty(&ctx->threads_available) == 1) {
			pthread_mutex_unlock(&ctx->mutex);
#if DEBUG > 6
			printf("[%s, INFO] No avaialble threads\r\n", __FUNCTION__);
#endif
			return 0;
		}

		if(linked_list_s_is_empty(&ctx->tasks) == 1) {
			pthread_mutex_unlock(&ctx->mutex);
#if DEBUG > 6
			printf("[%s, INFO] No task available\r\n", __FUNCTION__);
#endif
			return 0;
		}

#if DEBUG > 6
		printf("[%s, INFO] Moving thread from avaiable to busy\r\n"
				, __FUNCTION__);
#endif

		/* Get task from tasks list and delete it from tasks list */
		linked_list_s_get_data(&ctx->tasks, 0, (void **) &task, &size);
		linked_list_s_del(&ctx->tasks, 0);

		/* Get an available thread */
		linked_list_s_get_data(&ctx->threads_available, 0, (void **) &th
				, &size);
		linked_list_s_del(&ctx->threads_available, 0);

		/* Put thread into busy thread list */
		linked_list_s_add(&ctx->threads_busy, -1, (void *) th
				, sizeof(struct threadpool_thread));

#if DEBUG > 6
		/* Print busy threads list */
		printf("[%s, INFO] List of busy threads\r\n", __FUNCTION__);
		print_thread_list(&th->ctx->threads_busy);
#endif

		pthread_mutex_unlock(&ctx->mutex);

#if DEBUG > 6
		printf("[%s, INFO] Assigning task to %ld thread\r\n"
				, __FUNCTION__, th->tid);
#endif

		/* Assign a task to it */
		pthread_mutex_lock(&th->mutex);
		th->assigned_task = task;
		pthread_mutex_unlock(&th->mutex);

#if DEBUG > 6
		printf("[%s, INFO] Assigned task to %ld thread\r\n"
				, __FUNCTION__, th->tid);
#endif

		/* Signal the thread */
		pthread_cond_signal(&th->cond);

	}
#if DEBUG > 6
	printf("[%s, INFO] Done assigning tasks\r\n", __FUNCTION__);
#endif

	return 0;
}

void * sch_exec(void *ptr) {
	struct threadpool *ctx = (struct threadpool *) ptr;

#if DEBUG > 6
	printf("[%s, INFO] Started manager thread for threadpool\r\n"
			, __FUNCTION__);
#endif

	while(1) {
		/* Check if the threadpool was interrupted */
		pthread_mutex_lock(&ctx->mutex);
		if(ctx->is_interrupted != 0) {
			pthread_mutex_unlock(&ctx->mutex);
			break;
		}

		/* Wait for new task to be submitted or 
		 * for a thread to complete its task */
		pthread_cond_wait(&ctx->cond, &ctx->mutex);

#if DEBUG > 6
		printf("[%s, INFO] Manager thread woken\r\n", __FUNCTION__);
#endif

		pthread_mutex_unlock(&ctx->mutex);

		thread_assign_tasks(ctx);
	}

	return NULL;
}

/*
 *	Function which will be executed by every thread in the threadpool
 */
void * thread_exec(void *th) {
	struct threadpool_thread *cur_th;
	struct threadpool_task *task;

	/* Get thread context */
	cur_th = (struct threadpool_thread *) th;

#if DEBUG > 6
	printf("[%s, INFO] (%ld) Started for the first time\r\n"
			, __FUNCTION__, cur_th->tid);
#endif

	while(1) {
		/* Check if thread was interrupted */
		pthread_mutex_lock(&cur_th->mutex);
		if(cur_th->is_interrupted != 0) {
#if DEBUG > 6
			printf("[%s, INFO] (%ld) Thread interrupted\r\n"
				, __FUNCTION__, cur_th->tid);
#endif
			pthread_mutex_unlock(&cur_th->mutex);
			return NULL;
		}

		/* Wait for task to be assigned or thread to be interrupted */
#if DEBUG > 6
		printf("[%s, INFO] (%ld) Waiting for task or interrupt\r\n"
				, __FUNCTION__, cur_th->tid);
#endif
		pthread_cond_wait(&cur_th->cond, &cur_th->mutex);
	
#if DEBUG > 6
		printf("[%s, INFO] (%ld) Thread has task or "
				"thread is interrupted\r\n"
				, __FUNCTION__, cur_th->tid);
#endif

		/*
		 *	Get details of task.
		 *	If no task assigned, thread might has been interrupted
		 */
		task = cur_th->assigned_task;
		if(task == NULL) {
			pthread_mutex_unlock(&cur_th->mutex);
			continue;
		}

		/*	
		 *	Check if run is available
		 */
		if(task->run == NULL) {
			task->ret = NULL;
			pthread_mutex_unlock(&cur_th->mutex);
			continue;
		}

		/* 
		 * Run given task, change its state to running
		 * Once task is done save retuned data
		 * change the state of task to complete
		 */
		task->state = THREADPOOL_TASK_STATE_RUNNING;

#if DEBUG > 6
		printf("[%s, INFO] (%ld) Running new task\r\n"
				, __FUNCTION__, cur_th->tid);
#endif
		pthread_mutex_unlock(&cur_th->mutex);

		/*
		 * Run task
		 *
		 * Without any threadpool mutexes locked
		 *
		 */
		task->ret = task->run(task->data);

		/* Once task is complete update the status for the task */
		pthread_mutex_lock(&cur_th->mutex);
		task->state = THREADPOOL_TASK_STATE_COMPLETED;
#if DEBUG > 6
		printf("[%s, INFO] (%ld) Assigned task complete\r\n"
				, __FUNCTION__, cur_th->tid);
#endif
		pthread_mutex_unlock(&cur_th->mutex);

		/* Put the thread in available queue */
		thread_switch_to_available(cur_th);

		/* Signal threadpool manager */
		pthread_cond_signal(&cur_th->ctx->cond);
	}
}

/*
 *	This function creates a threadpool with num_threads created.
 *	Once threadpool is created, tasks can be sumitted using submit method
 *
 */
int threadpool_create(struct threadpool **ctx, int num_threads) {
	struct threadpool *new_pool;
	struct threadpool_thread *thread;
	int i, ret;

	/* Allocate memory for new threadpool */
	new_pool = (struct threadpool *) malloc(sizeof(struct threadpool));
	if(new_pool == NULL) {
#ifdef DEBUG
		fprintf(stderr, "[%s, ERROR] No memory to create new threadpool\r\n"
				, __FUNCTION__);
#endif
		errno = ENOMEM;
		return -1;
	}


	/* Create a linked list for storing available threadpool_threads */
	ret = linked_list_s_init(&new_pool->threads_available);
	if(ret != 0) {
#ifdef DEBUG
		fprintf(stderr, "[%s, ERROR] Unable to init internal thread list\r\n"
				, __FUNCTION__);
#endif
		free(new_pool);
		return -1;
	}

	/* Create a linked list for storing busy threadpool_threads */
	ret = linked_list_s_init(&new_pool->threads_busy);
	if(ret != 0) {
#ifdef DEBUG
		fprintf(stderr, "[%s, ERROR] Unable to init internal thread list\r\n"
				, __FUNCTION__);
#endif
		free(new_pool);
		return -1;
	}

	/* Init list for tasks */
	linked_list_s_init(&new_pool->tasks);
	if(ret != 0) {
#ifdef DEBUG
		fprintf(stderr, "[%s, ERROR] Unable to init tasks list\r\n"
				, __FUNCTION__);
#endif
		free(new_pool);
		return -1;
	}


	/* Init mutexe and condition variable for manager threads */
	pthread_mutex_init(&new_pool->mutex, NULL);
	pthread_cond_init(&new_pool->cond, NULL);

	/* Start manager thread for new threadpool */
	ret = pthread_create(&new_pool->th_manager, NULL, &sch_exec
			, (void *) new_pool);
	if(ret != 0) {
#ifdef DEBUG
		fprintf(stderr, "[%s, ERROR] Unable to start a manager thread\r\n"
				, __FUNCTION__);
#endif

		pthread_mutex_destroy(&new_pool->mutex);
		pthread_cond_destroy(&new_pool->cond);
		free(new_pool);
		return -1;
	}

	/* Create worker threads */
	for(i = 0; i < num_threads; i++) {
		/* Allocate memory for threadpoll_thread struct */
		thread = (struct threadpool_thread *) 
			malloc(sizeof(struct threadpool_thread));
		if(thread == NULL) {
#ifdef DEBUG
			fprintf(stderr, "[%s, ERROR] No memory to create a new "
					"thread object", __FUNCTION__);
#endif

			threadpool_destroy(new_pool);
			return -1;
		}

		/* Set threadpool this thread belongs to */
		thread->ctx = new_pool;
		
		/* initialize mutex and condition for thread */
		pthread_mutex_init(&thread->mutex, NULL);
		pthread_cond_init(&thread->cond, NULL);

		/* Init assigned_task to NULL */
		thread->assigned_task = NULL;

		/* Set is_interrupted to 0 */
		thread->is_interrupted = 0;

		/* Create threads */
		ret = pthread_create(&thread->tid, NULL, &thread_exec, thread);
		if(ret != 0) {
#ifdef DEBUG
			fprintf(stderr, "[%s, ERROR] Unable to start thread\r\n"
					, __FUNCTION__);
#endif
			threadpool_destroy(new_pool);
			return -1;
		}

		/* Adding to available pool */
		linked_list_s_add(&new_pool->threads_available, -1, (void *) thread
				, sizeof(struct threadpool_thread));
	}

	/* Store threadpool in ctx */
	*ctx = new_pool;

	return 0;
}

int threadpool_destroy(struct threadpool *ctx) {
	int size;
	struct threadpool_thread *th;
	struct threadpool_task *task;

	/* Delete queued tasks */
	pthread_mutex_lock(&ctx->mutex);
	
#if DEBUG > 6
	printf("[%s, INFO] Clear all pending tasks\r\n", __FUNCTION__);
#endif

	while(1) {
		/* Check if all tasks are deleted from threadpool */
		if(linked_list_s_is_empty(&ctx->tasks) == 1) {
			break;
		}

		/* Get a threadpool_task */
		linked_list_s_get_data(&ctx->tasks, 0, (void **) &task, &size);

		/* Delete threadpool_task from task list */
		linked_list_s_del(&ctx->tasks, 0);
	}
	pthread_mutex_unlock(&ctx->mutex);

	/* Delete all the busy threads */
#if DEBUG > 6
	printf("[%s, INFO] Waiting for all busy threads\r\n", __FUNCTION__);
#endif

	while(1) {
		pthread_mutex_lock(&ctx->mutex);
		/* Check if all threads are deleted from threadpool */
		if(linked_list_s_is_empty(&ctx->threads_busy) == 1) {
			pthread_mutex_unlock(&ctx->mutex);
			break;
		}
		pthread_mutex_unlock(&ctx->mutex);
	}

	/* Delete all available threads */
#if DEBUG > 6
	printf("[%s, INFO] Cleaning all available threads\r\n", __FUNCTION__);
#endif

	while(1) {
		pthread_mutex_lock(&ctx->mutex);
		/* Check if all threads are deleted from threadpool */
		if(linked_list_s_is_empty(&ctx->threads_available) == 1) {
			pthread_mutex_unlock(&ctx->mutex);
			break;
		}

		/* Get a threadpool_thread */
		linked_list_s_get_data(&ctx->threads_available, 0, (void **) &th
				, &size);
		pthread_mutex_unlock(&ctx->mutex);

#if DEBUG > 6
		printf("[%s, INFO] Cleaning thread %ld\r\n", __FUNCTION__, th->tid);
#endif

		/* Set interrupt flag */
		pthread_mutex_lock(&th->mutex);
#if DEBUG > 6
		printf("[%s, INFO] Thread %ld interrupted\r\n", __FUNCTION__, th->tid);
#endif
		th->is_interrupted = 1;
		pthread_mutex_unlock(&th->mutex);

		/* Signal thread */
		pthread_cond_signal(&th->cond);

		/* Wait till thread completes the task */
#if DEBUG > 6
		printf("[%s, INFO] Waiting for Thread %ld to complete its task\r\n"
				, __FUNCTION__, th->tid);
#endif
		pthread_join(th->tid, NULL);

		/* Destroy mutexes and conditions */
		pthread_mutex_destroy(&th->mutex);
		pthread_cond_destroy(&th->cond);

		/* Free memory for threadpool_thread */
		free(th);

		/* Delete threadpool_thread from list */
		pthread_mutex_lock(&ctx->mutex);
		linked_list_s_del(&ctx->threads_available, 0);
		pthread_mutex_unlock(&ctx->mutex);

#if DEBUG > 6
		printf("[%s, INFO] Cleaned\r\n", __FUNCTION__);
#endif
	}

#if DEBUG > 6
	printf("[%s, INFO] Stopping manager thread %ld\r\n", __FUNCTION__
			, ctx->th_manager);
#endif

	/* Stop manager thread */
	pthread_mutex_lock(&ctx->mutex);
	ctx->is_interrupted = 1;
	pthread_mutex_unlock(&ctx->mutex);

	/* Send signal to manager thread */
	pthread_cond_signal(&ctx->cond);

	/* Wait for manager thread to complete its tasks */
	pthread_join(ctx->th_manager, NULL);

	pthread_mutex_destroy(&ctx->mutex);
	pthread_cond_destroy(&ctx->cond);

	/* Free memory for threadpool */
	free(ctx);

#if DEBUG > 6
	printf("[%s, INFO] Threadpool destroied\r\n", __FUNCTION__);
#endif

	return 0;
}

int threadpool_task_create(struct threadpool_task **task
		, void * (*run)(void *)
		, void *data) {

	struct threadpool_task *new_task;

	/* Allocate memory for new task structure */
	new_task = (struct threadpool_task *) 
		malloc(sizeof(struct threadpool_task));
	if(new_task == NULL) {
#ifdef DEBUG
		fprintf(stderr, "[%s, ERROR] Unable to create a new task\r\n"
				, __FUNCTION__);
#endif
		errno = ENOMEM;
		return -1;
	}

	/* Init structure members to default values */
	new_task->run = run;
	new_task->data = data;
	new_task->ret = NULL;

	new_task->state = THREADPOOL_TASK_STATE_NEW;

	/* Store the new task pointer in the task */
	*task = new_task;

	return 0;
}

int threadpool_task_destroy(struct threadpool_task *task) {
	free(task);
	return 0;
}

int threadpool_submit(struct threadpool *ctx, struct threadpool_task *task) {
	/* Add task in task queue */
	pthread_mutex_lock(&ctx->mutex);
	linked_list_s_add(&ctx->tasks, -1, task, sizeof(struct threadpool_task));
	pthread_mutex_unlock(&ctx->mutex);

	/* Signal main thread to assign new task */
	pthread_cond_signal(&ctx->cond);
	return 0;
}

int threadpool_task_is_completed(struct threadpool_task *task) {
	int ret;
	pthread_mutex_lock(&task->th->mutex);
	ret = (task->state == THREADPOOL_TASK_STATE_COMPLETED);
	pthread_mutex_unlock(&task->th->mutex);

	return ret;
}

int threadpool_task_wait_complete(struct threadpool_task *task) {

	return -1;
}
