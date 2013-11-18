#include <stdio.h>
#include <iostream.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sync.h>
#include <sys/siginfo.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <sys/syspage.h>

#include "PulseTimer.h"
#include "PiMutex.h"
//=============================================================================

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

#define threadCount 10 /* Maximum number of threads*/

#define RELEASE_TIME_P1 4
#define RELEASE_TIME_P2 2
#define RELEASE_TIME_P3 0

#define PRIORITY_P1	0.7
#define PRIORITY_P2	0.6
#define PRIORITY_P3	0.5

float priority[threadCount] = {0};	// priority of threads
int active_p = 0;					// detemine the active thread that should be run

void ThreadManager();

//-----------------------------------------------------------------------------------------
// Instantiates a "Priority Inheritance" mutex.
//-----------------------------------------------------------------------------------------
PiMutex piMutex;

//-----------------------------------------------------------------------------------------
// Thread 1 (highest priority)
//-----------------------------------------------------------------------------------------
void * P1(void* arg)
{
	int cnt = 0;
	while(1)
	{
		pthread_mutex_lock(&mutex);

		// wait for the message from ThreadManager and check if current thread is active
		printf("\nP1: suspended, priority: %f", priority[1]);
		while (active_p != 1)
			pthread_cond_wait(&cond, &mutex);

		printf("\nP1: executing, cnt: %d", cnt);
		active_p = 0;

		if (cnt == 1)
		{
			// Try to acquire mutex after running for 1 unit
			printf("\nP1: try CS lock");
			if (piMutex.lock(&priority[1]) == 16);
				printf("\nP1: blocked, CS already locked");
		}
		else if (cnt == 3)
		{
			// Release mutex after running for 3 units
			printf("\nP1: unlocking semaphore");
			piMutex.unlock(&priority[1], PRIORITY_P1);
			printf("\nP1: semaphore unlocked");
		}
		else if (cnt == 4)
		{
			// Terminate after 4 units
			printf("\nP1: thread execution completed");

			// remove 1st process from the ThreadManager's queue
			priority[1] = 0;

			pthread_mutex_unlock(&mutex);
			break;
		}
		printf("\nP1: executed, cnt: %d", cnt);

		pthread_mutex_unlock(&mutex);
		cnt++;
	}

	return NULL;
}

//-----------------------------------------------------------------------------------------
// Thread 2 (medium priority)
//-----------------------------------------------------------------------------------------
void * P2(void* arg)
{
	int cnt = 0;
	while(1)
	{
		pthread_mutex_lock(&mutex);

		// wait for the message from ThreadManager and check if current thread is active
		printf("\nP2: suspended, priority: %f", priority[2]);
		while (active_p != 2)
			pthread_cond_wait(&cond, &mutex);

		printf("\nP2: executing, cnt: %d", cnt);
		active_p = 0;

		if (cnt == 6)
		{
			printf("\nP2: thread execution completed");

			// remove 1st process from the ThreadManager's queue
			priority[2] = 0;

			pthread_mutex_unlock(&mutex);
			break;
		}
		printf("\nP2: executed, cnt: %d", cnt);

		pthread_mutex_unlock(&mutex);
		cnt++;
	}

	return NULL;
}

//-----------------------------------------------------------------------------------------
// Thread 3 (lowest priority)
//-----------------------------------------------------------------------------------------
void * P3(void* arg)
{
	int cnt = 0;
	while(1)
	{
		printf("\nP3: lock mutex");
		pthread_mutex_lock(&mutex);

		// wait for the message from ThreadManager and check if current thread is active
		printf("\nP3: suspended, priority: %f", priority[3]);
		while (active_p != 3)
			pthread_cond_wait(&cond, &mutex);

		printf("\nP3: executing, cnt: %d", cnt);
		active_p = 0;

		if (cnt == 1)
		{
			printf("\nP3: locking CS");
			piMutex.lock(&priority[3]);
			printf("\nP3: CS locked");
		}
		else if (cnt == 3)
		{
			printf("\nP3: unlocking CS");
			piMutex.unlock(&priority[3], PRIORITY_P3);
			printf("\nP3: CS unlocked");
		}
		else if (cnt == 5)
		{
			printf("\nP3: thread execution completed");

			// remove 1st process from the ThreadManager's queue
			priority[3] = 0;

			pthread_mutex_unlock(&mutex);
			break;
		}
		printf("\nP3: executed, counter: %d", cnt);

		pthread_mutex_unlock(&mutex);

		cnt++;
	}

	return NULL;
}


//-----------------------------------------------------------------------------------------
// ThreadManager - determines which thread should run based on its current priority.
//-----------------------------------------------------------------------------------------
void threadManager()
{
	// find thread with the highest priority and flag it as active
	float p =- 1;
	for (int i = 1; i < threadCount; i++)
	{
		if (priority[i] > p)
		{
			active_p = i;
			p = priority[i];
		}
	}
	printf("\nThread manager: activate thread %d", active_p);

	printf("\nThread manager: notify threads");
	pthread_cond_broadcast(&cond);
}

//-----------------------------------------------------------------------------------------
// Main execution function
//-----------------------------------------------------------------------------------------
int main(void)
{
	// initialize threads
	pthread_t P1_ID, P2_ID, P3_ID;

	// create and start periodic timer to generate pulses every second.
	PulseTimer* timer = new PulseTimer(1);
	timer->start();

	int cnt = 0;
	while(1)
	{
		//--------------------------------------------
		pthread_mutex_lock(&mutex);

		// release P1 t = 4
		if(cnt == RELEASE_TIME_P1)
		{
			priority[1] = PRIORITY_P1;	// 0.7
			printf("\nP1 released");
			pthread_create(&P1_ID, NULL, P1, NULL);
		}
		// release P2 at t = 2
		if(cnt == RELEASE_TIME_P2)
		{
			priority[2] = PRIORITY_P2;	// 0.6
			printf("\nP2 released");
			pthread_create(&P2_ID, NULL, P2, NULL);
		}
		// release P3 at t = 0
		if(cnt == RELEASE_TIME_P3)
		{
			priority[3] = PRIORITY_P3;	// 0.5
			printf("\nP3 released");
			pthread_create(&P3_ID, NULL, P3, NULL);
		}
		 // terminate the program at t = 30
		if (cnt == 30)
		{
			printf("\n\n30 seconds are over, terminate program");
			break;
		}

		threadManager();

		pthread_mutex_unlock(&mutex);

		// wait for the timer pulse to fire
		timer->wait();
		printf("\n\n timer tick: %d\n", cnt);

		cnt++;
	}

	// stop and destroy the timer
	timer->stop();
	delete timer;
}
