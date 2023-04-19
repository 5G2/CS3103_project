#include <stdio.h>

#include <stdlib.h>

#include <stdint.h>

#include <sys/ipc.h>

#include <sys/wait.h>

#include <unistd.h>

#include <errno.h>

#include <assert.h>

#include <sys/shm.h> // This is necessary for using shared memory constructs

#include <semaphore.h> // This is necessary for using semaphore

#include <fcntl.h> // This is necessary for using semaphore

#include <pthread.h> // This is necessary for Pthread

#include <string.h>

#include "helpers.h"

#define PARAM_ACCESS_SEMAPHORE "/PARAM_ACCESS_SEMAPHORE11"

long int global_param = 0;
sem_t sem_array[8];
int arr[8];
int indexes[8] = {
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7};
long times;

/**
 * This function should be implemented by yourself. It must be invoked
 * in the child process after the input parameter has been obtained.
 * @parms: The input parameter from the terminal.
 */
void multi_threads_run(long int input_param);

int main(int argc, char **argv)
{
	int shmid, status;
	long int local_param = 0;
	long int *shared_param_p, *shared_param_c;

	if (argc < 2)
	{
		printf("Please enter an eight-digit decimal number as the input parameter.\nUsage: ./main <input_param>\n");
		exit(-1);
	}

	/*
		  Creating semaphores. Mutex semaphore is used to acheive mutual
		  exclusion while processes access (and read or modify) the global
		  variable, local variable, and the shared memory.
	  */

	// Checks if the semaphore exists, if it exists we unlink him from the process.
	sem_unlink(PARAM_ACCESS_SEMAPHORE);

	// Create the semaphore. sem_init() also creates a semaphore. Learn the difference on your own.
	sem_t *param_access_semaphore = sem_open(PARAM_ACCESS_SEMAPHORE, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);

	// Check for error while opening the semaphore
	if (param_access_semaphore != SEM_FAILED)
	{
		printf("Successfully created new semaphore!\n");
	}
	else if (errno == EEXIST)
	{ // Semaphore already exists
		printf("Semaphore appears to exist already!\n");
		param_access_semaphore = sem_open(PARAM_ACCESS_SEMAPHORE, 0);
	}
	else
	{ // An other error occured
		assert(param_access_semaphore != SEM_FAILED);
		exit(-1);
	}

	/*
		  Creating shared memory.
		  The operating system keeps track of the set of shared memory
		  segments. In order to acquire shared memory, we must first
		  request the shared memory from the OS using the shmget()
		  system call. The second parameter specifies the number of
		  bytes of memory requested. shmget() returns a shared memory
		  identifier (SHMID) which is an integer. Refer to the online
		  man pages for details on the other two parameters of shmget()
	  */
	shmid = shmget(IPC_PRIVATE, sizeof(long int), 0666 | IPC_CREAT); // We request an array of one long integer

	/*
		After forking, the parent and child must "attach" the shared
		memory to its local data segment. This is done by the shmat()
		system call. shmat() takes the SHMID of the shared memory
		segment as input parameter and returns the address at which
		the segment has been attached. Thus shmat() returns a char
		pointer.
	*/

	if (fork() == 0)
	{ // Child Process

		printf("Child Process: Child PID is %jd\n", (intmax_t)getpid());

		/*  shmat() returns a long int pointer which is typecast here
			to long int and the address is stored in the long int pointer shared_param_c. */
		shared_param_c = (long int *)shmat(shmid, 0, 0);

		while (1) // Loop to check if the variables have been updated.
		{
			// Get the semaphore
			sem_wait(param_access_semaphore);
			printf("Child Process: Got the variable access semaphore.\n");

			if ((global_param != 0) || (local_param != 0) || (shared_param_c[0] != 0))
			{
				printf("Child Process: Read the global variable with value of %ld.\n", global_param);
				printf("Child Process: Read the local variable with value of %ld.\n", local_param);
				printf("Child Process: Read the shared variable with value of %ld.\n", shared_param_c[0]);

				// Release the semaphore
				sem_post(param_access_semaphore);
				printf("Child Process: Released the variable access semaphore.\n");

				break;
			}

			// Release the semaphore
			sem_post(param_access_semaphore);
			printf("Child Process: Released the variable access semaphore.\n");
		}

		/**
		 * After you have fixed the issue in Problem 1-Q1,
		 * uncomment the following multi_threads_run function
		 * for Problem 1-Q2. Please note that you should also
		 * add an input parameter for invoking this function,
		 * which can be obtained from one of the three variables,
		 * i.e., global_param, local_param, shared_param_c[0].
		 */
		times = strtol(argv[2], NULL, 10);
		multi_threads_run(shared_param_c[0]);

		/* each process should "detach" itself from the
		   shared memory after it is used */
		shmdt(shared_param_c);

		exit(0);
	}
	else
	{ // Parent Process

		printf("Parent Process: Parent PID is %jd\n", (intmax_t)getpid());

		/*  shmat() returns a long int pointer which is typecast here
			to long int and the address is stored in the long int pointer shared_param_p.
			Thus the memory location shared_param_p[0] of the paent
			is the same as the memory locations shared_param_c[0] of
			the child, since the memory is shared.
		*/
		shared_param_p = (long int *)shmat(shmid, 0, 0);

		// Get the semaphore first
		sem_wait(param_access_semaphore);
		printf("Parent Process: Got the variable access semaphore.\n");
		global_param = strtol(argv[1], NULL, 10);
		local_param = strtol(argv[1], NULL, 10);
		shared_param_p[0] = strtol(argv[1], NULL, 10);

		// Release the semaphore
		sem_post(param_access_semaphore);
		printf("Parent Process: Released the variable access semaphore.\n");

		wait(&status);

		/* each process should "detach" itself from the
		   shared memory after it is used */

		shmdt(shared_param_p);

		/* Child has exited, so parent process should delete
		   the cretaed shared memory. Unlike attach and detach,
		   which is to be done for each process separately,
		   deleting the shared memory has to be done by only
		   one process after making sure that noone else
		   will be using it
		 */

		shmctl(shmid, IPC_RMID, 0);

		// Close and delete semaphore.
		sem_close(param_access_semaphore);
		sem_unlink(PARAM_ACCESS_SEMAPHORE);

		exit(0);
	}

	exit(0);
}

/**
 * This function should be implemented by yourself. It must be invoked
 * in the child process after the input parameter has been obtained.
 * @parms: The input parameter from terminal.
 */

void *child(void *arg)
{
	int id = *(int *)arg;
	for (long i = 0; i < times; i++)
	{
		sem_wait(&sem_array[id]);
		// perform addition on id-th digit
		arr[id] = (arr[id] + 1) % 10;
		sem_post(&sem_array[id]);
		sem_wait(&sem_array[(id + 1) % 8]);
		// perform addition on id+1mod8-th digit
		arr[(id + 1) % 8] = (arr[(id + 1) % 8] + 1) % 10;
		sem_post(&sem_array[(id + 1) % 8]);
	}
	pthread_exit(NULL);
}

<<<<<<< HEAD
void multi_threads_run(long int input_param)
{
	printf("argv 2 in multi %ld times\n", times);
	// Declare variables
	pthread_t thread[8];
	// convert input_param to array to allow the thread to access one digit at a time
	for (int i = 7; i >= 0; i--)
	{
		arr[i] = input_param % 10;
		input_param /= 10;
	}
=======
void multi_threads_run(long int input_param) {
  printf("argv 2 in multi %ld\n", times);
  //Declare variables
  pthread_t thread[8];
  //convert input_param to array to allow the thread to access one digit at a time
  for (int i = 7; i >= 0; i--) {
    arr[i] = input_param % 10;
    input_param /= 10;
  }
>>>>>>> f7afd30f85d7c769011a8509a3067b605d4d5a69

	// initialize the array of threads and semaphore
	for (int i = 0; i < 8; i++)
	{
		if (sem_init(&sem_array[i], 0, 1) < 0)
		{
			perror("Failed to initialize semaphore");
			exit(EXIT_FAILURE);
		}
	}

<<<<<<< HEAD
	// create 8 thread to do the calculation
	for (int i = 0; i < 8; i++)
	{
		pthread_create(&thread[i], NULL, child, indexes + i);
	}
	// waiting all threads finished
	for (int i = 0; i < 8; i++)
	{
		pthread_join(thread[i], NULL);
	}
=======
  //create 8 thread to do the calculation
  for (int i = 0; i < 8; i++) {
    pthread_create( & thread[i], NULL, child, indexes + i);
  }
  //waiting all threads finished
  for (int i = 0; i < 8; i++) {
    pthread_join(thread[i], NULL);
  }

  //for debugging use, to check to if result is correct
  printf("Array after all threads finish:\n");
  for (int i = 0; i < 8; i++) {
    printf("%d ", arr[i]);
  }
  printf("\n");
>>>>>>> f7afd30f85d7c769011a8509a3067b605d4d5a69

	// for debugging use, to check to if result is correct
	printf("Array after all threads finish:\n");
	for (int i = 0; i < 8; i++)
	{
		printf("%d ", arr[i]);
	}
	printf("\n");

	// change the arr to integer
	int result = 0;
	int multiplier = 1;
	for (int i = 7; i >= 0; i--)
	{
		result += arr[i] * multiplier;
		multiplier *= 10;
	}

	// call saveResult() to save the data
	saveResult("p1_result.txt", result);
}