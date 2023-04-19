#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/shm.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <math.h>

#include "helpers.h"

char *paths[100];  // for storing the accessing paths of the text files
int num = 0;	   // the number of text files in the directory
long looptime = 0; // the number of child process read data form shared memory

#define SHM_SIZE 1048576	  // 1MB
#define segment_length 262144 // 256kB

#define PARAM_ACCESS_SEMAPHORE1 "/parent_semaphorea" // parent semaphore
#define PARAM_ACCESS_SEMAPHORE2 "/child_semaphorea"	 // child semaphore

// This struct is used to store data that will be passed to each thread.
// It contains a pointer to a segment of shared memory, the thread ID, and the result of the thread's computation.
typedef struct thread_data
{
	char *segment_ptrs; // Pointer to a segment of shared memory
	int threadid;		// The ID of the thread
	int result;			// The result of the thread's computation
} thread_data;

/**
 * @brief This function recursively traverse the source directory.
 *
 * @param dir_name : The source directory name.
 */
void traverseDir(char *dir_name);

/**
 * This function is the entry point for each thread that counts the number of words in a segment of shared memory.
 * It first creates a local copy of the shared memory segment pointed to by the thread_data struct.
 * It then counts the number of words in the segment and stores the result in the thread_data struct.
 * Finally, the function frees the memory allocated for the local copy of the shared memory segment and returns NULL.
 *
 * @param arg : A pointer to a thread_data struct containing a pointer to a segment of shared memory and the thread ID.
 * @return NULL
 */
void *threadFunc(void *arg);

int main(int argc, char **argv)
{
	int process_id; // Process identifier

	// The source directory.
	// It can contain the absolute path or relative path to the directory.
	char *dir_name = argv[1];

	if (argc < 2)
	{
		printf("Main process: Please enter a source directory name.\nUsage: ./main <dir_name>\n");
		exit(-1);
	}

	// Checks if the semaphore exists, if it exists we unlink him from the process.
	sem_unlink(PARAM_ACCESS_SEMAPHORE1);
	sem_unlink(PARAM_ACCESS_SEMAPHORE2);

	// Create the semaphore. sem_init() also creates a semaphore. Learn the difference on your own.
	sem_t *parent_semaphore = sem_open(PARAM_ACCESS_SEMAPHORE1, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
	sem_t *child_semaphore = sem_open(PARAM_ACCESS_SEMAPHORE2, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);

	// Check for error while opening the semaphore
	if (parent_semaphore != SEM_FAILED)
	{
		printf("Successfully created new semaphore1!\n");
	}
	else if (errno == EEXIST)
	{ // Semaphore already exists
		printf("Semaphore appears to exist already!\n");
		parent_semaphore = sem_open(PARAM_ACCESS_SEMAPHORE1, 0);
	}
	else
	{ // An other error occured
		assert(parent_semaphore != SEM_FAILED);
		exit(-1);
	}

	if (child_semaphore != SEM_FAILED)
	{
		printf("Successfully created new semaphore2!\n");
	}
	else if (errno == EEXIST)
	{ // Semaphore already exists
		printf("Semaphore appears to exist already!\n");
		child_semaphore = sem_open(PARAM_ACCESS_SEMAPHORE2, 0);
	}
	else
	{ // An other error occured
		assert(child_semaphore != SEM_FAILED);
		exit(-1);
	}

	// Traverse the directory and store the paths of the text files
	traverseDir(dir_name);

	/////////////////////////////////////////////////
	// You can add some code here to prepare before fork.
	/////////////////////////////////////////////////
	int shmid;
	char *shared_param_p, *shared_param_c;
	int totalWord = 0;

	shmid = shmget(IPC_PRIVATE, SHM_SIZE, 0666 | IPC_CREAT);

	// Calculate looptime, the calucation of looptime is divide total length of all files by 1MB and take the ceiling of the result
	for (int i = 0; i < num; i++)
	{
		FILE *fp = fopen(paths[i], "r");
		looptime += (fileLength(fp) > SHM_SIZE ? fileLength(fp) : SHM_SIZE);
		fclose(fp);
	}
	looptime = (long)(((double)looptime / SHM_SIZE) + 0.9999999);

	switch (process_id = fork())
	{

	default:
		/*
			Parent Process
		*/
		printf("Parent process: My ID is %jd\n", (intmax_t)getpid());

		/////////////////////////////////////////////////
		// Implement your code for parent process here.
		/////////////////////////////////////////////////

		shared_param_p = (char *)shmat(shmid, NULL, 0);
		for (size_t i = 0; i < num; i++)
		{

			// Wait for permission to access shared memory
			sem_wait(parent_semaphore);
			// Open the input file
			FILE *fp = fopen(paths[i], "r");
			// Get the size of the file, why minus 1 detail refer to helper.c
			size_t fileSize = fileLength(fp);
			// Initialize variables for tracking the last two spaces
			size_t lastspace = -1;
			// Initialize pointer for writing to shared memory
			int temppointer = 0;
			// Loop over the file contents
			for (long i = 0; i < fileSize; i++)
			{
				// Read the next character from the file
				char c = (char)fgetc(fp);
				// Write the character to shared memory
				shared_param_p[temppointer] = c;
				// printf("%c\n", c);
				//  Check if the character is a space or a newline
				if (c == ' ' || c == '\n')
				{
					// Update the position of the last two spaces
					lastspace = temppointer;
				}
				// If the shared memory segment is full, check if the last character written was neither a space nor a newline
				if (temppointer == SHM_SIZE - 1)
				{
					if (c != ' ' || c != '\n')
					{
						// Replace the last space with a dummy char
						shared_param_p[lastspace] = 'A';
					}
					else
					{
						shared_param_p[temppointer - 1] = ' ';
					}
					// Reset the pointer and signal that the shared memory segment is ready for reading
					// Add a null character in the last of shared memory
					shared_param_p[temppointer] = '\0';
					temppointer = 0;
					sem_post(child_semaphore);
					sem_wait(parent_semaphore);
				}
				else
				{
					temppointer++;
				}
			}
			// Add a null character in the last of shared memory
			shared_param_p[(fileSize % SHM_SIZE) - 1] = '\0';
			// Close the input file
			fclose(fp);
			// Signal that the current file is done being processed
			sem_post(child_semaphore);
		}

		// Free the memory allocated for the file paths
		for (int i = 0; i < num; i++)
		{
			free(paths[i]);
		}

		// Wait for all child processes to complete
		wait(NULL);
		printf("Parent process: Finished.\n");

		shmdt(shared_param_p);
		shmctl(shmid, IPC_RMID, 0);

		// Clean up resources and exit the parent process
		sem_close(parent_semaphore);
		sem_unlink(PARAM_ACCESS_SEMAPHORE1);
		sem_close(child_semaphore);
		sem_unlink(PARAM_ACCESS_SEMAPHORE2);
		exit(0);
		break;

	case 0:
		/*
			Child Process
		*/

		printf("Child process: My ID is %jd\n", (intmax_t)getpid());

		/////////////////////////////////////////////////
		// Implement your code for child process here.
		/////////////////////////////////////////////////
		// initalized result
		int totalWord = 0;
		// Attach the shared memory segment to the child process
		char *shared_param_c = (char *)shmat(shmid, NULL, 0);

		// Initialize the thread_data array and set the segment pointers, thread IDs, and initial results
		thread_data thread_data_array[4];
		for (int i = 0; i < 4; i++)
		{
			thread_data_array[i].segment_ptrs = shared_param_c + i * segment_length;
			thread_data_array[i].threadid = i + 1;
			thread_data_array[i].result = 0;
		}

		// Create 4 threads to count the words in the shared memory segment
		pthread_t threads[4];
		for (long i = 0; i < looptime; i++)
		{
			// Wait for the parent process to signal that the shared memory segment is ready to be accessed
			sem_wait(child_semaphore);

			// Create the threads and pass each thread_data struct as an argument
			for (int i = 0; i < 4; i++)
			{
				pthread_create(&threads[i], NULL, threadFunc, &thread_data_array[i]);
			}

			// Wait for all threads to finish
			for (int i = 0; i < 4; i++)
			{
				pthread_join(threads[i], NULL);
			}

			// Add up the word counts from each thread and reset the results for the next iteration
			for (int i = 0; i < 4; i++)
			{
				totalWord += thread_data_array[i].result;
				thread_data_array[i].result = 0;
			}

			// Clear the shared memory segment and signal the parent process that it can be accessed again
			memset(shared_param_c, '\0', SHM_SIZE);
			sem_post(parent_semaphore);
		}

		// Save the result to a file and print the word count
		saveResult("p3_result.txt", totalWord);
		printf("Word count is: %d\n", totalWord);

		// Detach the shared memory segment from the child process and exit
		shmdt(shared_param_c);
		exit(0);

	case -1:
		/*
		Error occurred.
		*/
		printf("Fork failed!\n");
		exit(-1);
	}

	exit(0);
}

/**
 * @brief This function recursively traverse the source directory.
 *
 * @param dir_name : The source directory name.
 *
 * This function recursively traverses the source directory and
 * stores the accessing paths of the text files in the global array `paths`.
 * It also counts the number of text files in the directory and stores it in the global variable `num`.
 */
void traverseDir(char *dir_name)
{
	// Open the directory
	DIR *dir = opendir(dir_name);
	struct dirent *entity;
	if (dir == NULL)
	{
		return;
	}

	// Read all the objects in the directory
	entity = readdir(dir);
	while ((entity = readdir(dir)) != NULL)
	{
		// Skip the "." and ".." directories
		if (strcmp(entity->d_name, ".") != 0 && strcmp(entity->d_name, "..") != 0)
		{
			// Get the full path of the object
			char local_Path[100] = {0};
			strcat(local_Path, dir_name);
			strcat(local_Path, "/");
			strcat(local_Path, entity->d_name);

			// If the object is a directory, recursively traverse it
			if (entity->d_type == DT_DIR)
			{
				traverseDir(local_Path);
			}
			// If the object is a text file, add its path to the array
			else if (entity->d_type == DT_REG && strstr(entity->d_name, ".txt") != NULL)
			{
				printf("%s\n", local_Path);
				int length = strlen(local_Path) + 1;
				paths[num] = (char *)malloc(length);
				strcpy(paths[num++], local_Path);
			}
		}
	}
	closedir(dir);
}

/**
 * This function is the entry point for each thread. It takes a pointer to a thread_data struct as an argument.
 * The function first creates a local copy of the shared memory segment pointed to by the thread_data struct.
 * It then counts the number of words in the segment and stores the result in the thread_data struct.
 * Finally, the function frees the memory allocated for the local copy of the shared memory segment and returns NULL.
 *
 * @param arg A pointer to a thread_data struct containing a pointer to a segment of shared memory and the thread ID.
 * @return NULL
 */
void *threadFunc(void *arg)
{
	// Cast the argument to a pointer to a thread_data struct
	thread_data *data = (thread_data *)arg;

	// Allocate memory for a local copy of the shared memory segment
	char *thread_array = (char *)malloc(segment_length);
	memcpy(thread_array, data->segment_ptrs, segment_length);

	// Count the number of words in the local copy of the shared memory segment
	size_t thread_array_length = strlen(thread_array);
	if (thread_array_length == segment_length)
	{
		// Check if the last character of the segment is a null terminator, a space, or a newline character
		char c = thread_array[segment_length - 1];
		if (c != '\0' || c != ' ' || c != '\n')
		{
			// Set the result to -1 if the last character is not a null terminator, a space, or a newline character
			data->result = -1;
		}
	}

	// Count the number of words in the local copy of the shared memory segment and add the result to the thread_data struct
	if (thread_array[0] != '\0')
	{
		data->result += wordCount(thread_array);
	}

	// Print the thread ID and the result of the computation
	printf("Thread ID: %d   Word count: %d\n", data->threadid, data->result);

	// Free the memory allocated for the local copy of the shared memory segment
	free(thread_array);

	return NULL;
}
