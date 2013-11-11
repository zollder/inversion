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
#define eps .005
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
// Instantiates an array of "Priority Inheritance" mutexes (one for each thread).
//-----------------------------------------------------------------------------------------
PiMutex piMutex[threadCount]; // mutexes are instantiated here

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
		while(active_p != 1)
			pthread_cond_wait(&cond, &mutex);

		cout << "P1 started";
		if(cnt == 1)
		{
			// Try to acquire mutex after running for 1 unit
			cout << ".....Attempting to Lock Semaphore ..";
			piMutex[0].lock(1);
		}
		else if(cnt == 3)
		{
			// Release mutex after running for 3 units
			cout << ".....Unlocking Semaphore ..";
			piMutex[0].unlock(1);
		}
		else if (cnt == 4)
		{
			// Terminate after 4 units
			cout << "P1 completed.........";

			// remove 1st process from the ThreadManager's queue
			priority[1] = 0;

			pthread_mutex_unlock(&mutex);
			break;
		}
		cnt++;

		pthread_mutex_unlock(&mutex);
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
		while (active_p != 2)
			pthread_cond_wait(&cond, &mutex);

		cout << "P2 started ...";
		if (cnt == 6)
		{
			cout << "P2 completed ...";

			// remove 1st process from the ThreadManager's queue
			priority[2] = 0;

			pthread_mutex_unlock(&mutex);
			break;
		}
		cnt++;

		pthread_mutex_unlock(&mutex);
	}

	return NULL;
}

//-----------------------------------------------------------------------------------------
// Thread 3 (lowest priority)
//-----------------------------------------------------------------------------------------
void * P3(void* arg)
{
	int cnt=0;
	while(1)
	{
		pthread_mutex_lock(&mutex);

		// wait for the message from ThreadManager and check if current thread is active
		while(active_p != 3);
			pthread_cond_wait(&cond,&mutex);

		cout << "P3 started ...";
		if(cnt == 1)
		{
			cout << "Attempting to Lock Semaphore ..";
			piMutex[0].lock(3);
		}
		else if(cnt==3)
		{
			cout << ".....Unlocking Semaphore ..";
			piMutex[0].unlock(3);
		}
		else if (cnt == 5)
		{
			cout << "P3 completed ...";

			// remove 1st process from the ThreadManager's queue
			priority[3] = 0;

			pthread_mutex_unlock(&mutex);
			break;
		}
		cnt++;

		pthread_mutex_unlock(&mutex);
	}

	return NULL;
}


//-----------------------------------------------------------------------------------------
// ThreadManager - determines which thread should run based on its current priority.
//-----------------------------------------------------------------------------------------
void ThreadManager()
{
	float p;
	int i;

	pthread_mutex_lock(&mutex);

	// find thread with the highest priority and flag it as active
	p =- 1;
	for(i = 1; i < threadCount; i++)
	{
		if(priority[i] > p)
		{
			active_p = i;
			p = priority[i];
		}
	}

	pthread_mutex_unlock(&mutex);
	pthread_cond_broadcast(&cond); // notify threads
}

//-----------------------------------------------------------------------------------------
// Main execution function
//-----------------------------------------------------------------------------------------
int main(void)
{
	// initilize threads
	pthread_t P1_ID, P2_ID, P3_ID;

	// create and start periodic timer to generate pulses every second.
	PulseTimer* timer = new PulseTimer(1, 0);
	timer->start();

	int cnt=0;
	while(1)
	{
		//--------------------------------------------
		pthread_mutex_lock(&mutex);
		 // release P1 t= 4
		if(cnt == RELEASE_TIME_P1)
		{
			priority[1] = PRIORITY_P1;
			pthread_create(&P1_ID, NULL, P1, NULL);
		}
		 // release P2 at t=2
		if(cnt == RELEASE_TIME_P2)
		{
			priority[2] = PRIORITY_P2;
			pthread_create(&P2_ID, NULL, P2, NULL);
		}
		 // release P3 at t=0
		if(cnt == RELEASE_TIME_P3)
		{
			priority[3] = PRIORITY_P3;
			pthread_create(&P3_ID, NULL, P3, NULL);
		}
		 // terminate the program at t=30
		if (cnt == 30)
			break;

		pthread_mutex_unlock(&mutex);

		// wait for the timer pulse to fire
		timer->wait();

		// message
		cout << endl << "tick=" << cnt << /*", active_p="<<active_p<<*/ "->";

		// determine active thread and run it
		ThreadManager();

		cnt++;
	}

	// stop and destroy the timer
	timer->stop();
	delete timer;
}

