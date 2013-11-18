#include <pthread.h>

#ifndef PiMutex_h
#define PiMutex_h

//-----------------------------------------------------------------------------------------
// PiMutex (Priority Inheritance Mutex) class definition and implementation.
// Works as a wrapper around standard pthread_mutex functions.
//-----------------------------------------------------------------------------------------
class PiMutex
{
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
		}

		//-----------------------------------------------------------------------------------------
		// Locks piMutex. Returns 0 (success) or error code (failure).
		//-----------------------------------------------------------------------------------------
		int lock()
		{
			int lockStatus = pthread_mutex_lock(&piMutex);
			return lockStatus;
		}

		//-----------------------------------------------------------------------------------------
		// Locks piMutex for critical section with highest known thread priority.
		// Returns 0 (success) or error code (failure).
		//-----------------------------------------------------------------------------------------
		int lock(float *currentThreadPriority)
		{
			int lockStatus = pthread_mutex_trylock(&piMutex);

			// if unlocked
			if (lockStatus == 0)
			{
				printf("\nPiMutex: locking CS");
				maxPriorityThreadPtr = currentThreadPriority;

				if (csPriority > *currentThreadPriority)
				{
					printf("\nPiMutex: set thread's priority to %f", csPriority);
					*currentThreadPriority = csPriority;
				}
				else
				{
					printf("\nPiMutex: inherit priority %f", *currentThreadPriority);
					csPriority = *currentThreadPriority;
				}
			}
			// if already locked by lower priority thread
			else if (lockStatus == 16 && csPriority < *currentThreadPriority)
			{
				printf("\nPiMutex: CS already locked, inherit and swap priorities");

				*maxPriorityThreadPtr = *currentThreadPriority;
				*currentThreadPriority = csPriority;
				csPriority = *maxPriorityThreadPtr;
				maxPriorityThreadPtr = currentThreadPriority;

			}
			else
				printf("\nPiMutex: lock error");

			return lockStatus;
		}

		//-----------------------------------------------------------------------------------------
		// Unlocks piMutex. Returns 0 (success) or error code (failure).
		//-----------------------------------------------------------------------------------------
		int unlock()
		{
			int unlockStatus = pthread_mutex_unlock(&piMutex);
			return unlockStatus;
		}

		//-----------------------------------------------------------------------------------------
		// Unlocks piMutex and restores thread's priority to its original value, returns 0 (success) or error code (failure).
		// Restores original thread's priority
		//-----------------------------------------------------------------------------------------
		int unlock(float *currentThreadPriority, float originalThreadPriority)
		{
			int unlockStatus = pthread_mutex_unlock(&piMutex);

			*currentThreadPriority = originalThreadPriority;

			if (unlockStatus == 0)
			{
				printf("\nPiMutex: recovering thread's priority");
				*maxPriorityThreadPtr = csPriority;
			}

			return unlockStatus;
		}

	//-----------------------------------------------------------------------------------------
	// Private members
	//-----------------------------------------------------------------------------------------
	private:
		pthread_mutex_t piMutex;
		float csPriority;
		float *maxPriorityThreadPtr;
};

#endif
