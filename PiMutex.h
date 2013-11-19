#include <pthread.h>
#include <list>

#ifndef PiMutex_h
#define PiMutex_h

//-----------------------------------------------------------------------------------------
// PiMutex (Priority Inheritance Mutex) class definition and implementation.
// Works as a wrapper around standard pthread_mutex functions.
//-----------------------------------------------------------------------------------------
class PiMutex
{
	//-----------------------------------------------------------------------------------------
	// Thread priority data holder
	//-----------------------------------------------------------------------------------------
	struct ThreadInfo
	{
		float *threadPtr;
		float nativePriority;
	};

	//-----------------------------------------------------------------------------------------
	// Public members
	//-----------------------------------------------------------------------------------------
	public:
		//-----------------------------------------------------------------------------------------
		// Constructor (initializes piMutex)
		//-----------------------------------------------------------------------------------------
		PiMutex()
		{
			printf("Initializing piMutex ...\n");
			pthread_mutex_init(&piMutex, NULL);
			history = new list<ThreadInfo>;
			csPriority = 0;
		}

		//-----------------------------------------------------------------------------------------
		// Destructor
		//-----------------------------------------------------------------------------------------
		virtual ~PiMutex()
		{
			printf("Destroying piMutex ...\n");
			int status = pthread_mutex_destroy(&piMutex);
			if (status != 0)
				printf("Error destroying piMutex");
			delete history;
		}

		//-----------------------------------------------------------------------------------------
		// Locks piMutex for critical section with highest known thread priority.
		// Keeps references to priority donors (history).
		// Returns 0 (success) or error code (failure).
		//-----------------------------------------------------------------------------------------
		int lock(float *currentThreadPriorityPtr)
		{
			int lockStatus = pthread_mutex_trylock(&piMutex);

			// if unlocked
			if (lockStatus == 0)
			{
				printf("\nPiMutex: locking CS");

				// keep reference to the locking thread's priority
				ThreadInfo threadData;
				threadData.threadPtr = currentThreadPriorityPtr;
				threadData.nativePriority = csPriority;
				history->push_front(threadData);

				if (csPriority > *currentThreadPriorityPtr)
				{
					printf("\nPiMutex: inherit CS priority: %f", *currentThreadPriorityPtr);
					*currentThreadPriorityPtr = csPriority;
				}
				else
				{
					printf("\nPiMutex: update CS priority to: %f", *currentThreadPriorityPtr);
					csPriority = *currentThreadPriorityPtr;
				}

			}
			// if already locked by lower priority thread
			else if (lockStatus == 16 && csPriority < *currentThreadPriorityPtr)
			{
				printf("\nPiMutex: CS already locked, inherit priority: %f", *currentThreadPriorityPtr);

				// update CS and locking thread's priority to that of the attempting thread
				csPriority = *currentThreadPriorityPtr;
				*(history->back().threadPtr) = *currentThreadPriorityPtr;

				// keep reference to the suspended thread's priority
				ThreadInfo threadData;
				threadData.threadPtr = currentThreadPriorityPtr;
				threadData.nativePriority = *currentThreadPriorityPtr;
				history->push_front(threadData);

				printf("\nPiMutex: suspend higher priority thread");
				*currentThreadPriorityPtr = 0;	// set its priority to 0
			}
			else
				printf("\nPiMutex: ignoring locking attempts from lower priority threads");

			return lockStatus;
		}

		//-----------------------------------------------------------------------------------------
		// Unlocks piMutex and restores thread priorities to their original values.
		// This also resumes suspended priorities (with priority set to 0).
		//-----------------------------------------------------------------------------------------
		int unlock(float *currentThreadPriority)
		{
			int unlockStatus = pthread_mutex_unlock(&piMutex);

			if (unlockStatus == 0)
			{
				printf("\nPiMutex: unlocked, recovering priorities, resuming suspended threads");
				while(!history->empty())
				{
					// recover native priorities (also resumes suspended threads)
					*(history->front().threadPtr) = history->front().nativePriority;
					history->pop_front();

					// reset CS priority
					csPriority = 0;
				}
			}

			return unlockStatus;
		}

	//-----------------------------------------------------------------------------------------
	// Private members
	//-----------------------------------------------------------------------------------------
	private:
		pthread_mutex_t piMutex;
		list<ThreadInfo> *history;
		float csPriority;
		float *lockingThreadPriorityPtr;
};

#endif
