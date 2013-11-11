#include <pthread.h>

#ifndef mutex_h
#define mutex_h

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
			currentPriority = -1;
			inherited = false;
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
		// Locks piMutex, returns 0 (success) or error code (failure).
		//-----------------------------------------------------------------------------------------
		int lock(int priority)
		{
			// update current priority
			if (currentPriority < priority)
				currentPriority = priority;

			return pthread_mutex_lock(&piMutex);
		}

		//-----------------------------------------------------------------------------------------
		// Unlocks piMutex, returns 0 (success) or error code (failure).
		//-----------------------------------------------------------------------------------------
		int unlock(int priority)
		{
			return pthread_mutex_unlock(&piMutex);
		}

		//-----------------------------------------------------------------------------------------
		// Returns inheritance status of the priority.
		//-----------------------------------------------------------------------------------------
		bool isInherited()
		{
			return inherited;
		}

	//-----------------------------------------------------------------------------------------
	// Private members
	//-----------------------------------------------------------------------------------------
	private:
		pthread_mutex_t  piMutex;
		int currentPriority;
		bool inherited;
};

#endif
