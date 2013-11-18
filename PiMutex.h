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

			if (lockStatus == 0)	// if unlocked
			{
				printf("\nPiMutex: locking critical section");
				maxPriorityThreadPtr = currentThreadPriority;
				if (csPriority > *currentThreadPriority)
				{
					printf("\nPiMutex: Increasing thread's priority to %f", csPriority);
					*currentThreadPriority = csPriority;
				}
				else
				{
					printf("\nPiMutex: Increasing critical section priority to %f", *currentThreadPriority);
					csPriority = *currentThreadPriority;
				}
				return lockStatus;
			}
			else if (lockStatus == 16 && csPriority < *currentThreadPriority)	// if locked
			{
				printf("\nPiMutex: critical section already locked, updating CS and threads priorities");

				*maxPriorityThreadPtr = *currentThreadPriority;
				printf("\nPiMutex: new lockingThreadPtr value: %f", *maxPriorityThreadPtr);

				*currentThreadPriority = csPriority;
				printf("\nPiMutex: old lockingThreadPtr value: %f", *currentThreadPriority);

				csPriority = *maxPriorityThreadPtr;
				printf("\nPiMutex: updated critical section priority: %f", csPriority);

				maxPriorityThreadPtr = currentThreadPriority;
				return lockStatus;
			}

			printf("\nPiMutex: locking error");
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
