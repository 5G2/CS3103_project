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

#define PARAM_ACCESS_SEMAPHORE1 "/param_access_semaphore1" // parent semaphore
#define PARAM_ACCESS_SEMAPHORE2 "/param_access_semaphore2" // child semaphore

typedef struct thread_data
{
	char *segment_ptrs;
	int threadid;
	int result;

} thread_data;

/**
 * @brief This function recursively traverse the source directory.
 *
 * @param dir_name : The source directory name.
 */
void traverseDir(char *dir_name);

void *threadFunc(void *arg)
{
	thread_data *data = (thread_data *)arg;

	char *thread_array = (char *)malloc(segment_length);
	memcpy(thread_array, data->segment_ptrs, segment_length);
	size_t thread_array_length = strlen(thread_array);
	printf("Length of thread_array: %zu\n", thread_array_length);
	if (thread_array_length == 262144)
	{
		char c = thread_array[262143];
		if (c != '\0' || c != ' ' || c != '\n')
		{
			data->result = -1;
		}
	}

	if (thread_array[0] != '\0')
	{
		data->result += wordCount(thread_array);
	}
	printf("this is process %d   \nword count %d\n", data->threadid, data->result);

	free(thread_array);

	return NULL;
}

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
	sem_t *param_access_semaphore1 = sem_open(PARAM_ACCESS_SEMAPHORE1, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
	sem_t *param_access_semaphore2 = sem_open(PARAM_ACCESS_SEMAPHORE2, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);

	// Check for error while opening the semaphore
	if (param_access_semaphore1 != SEM_FAILED)
	{
		printf("Successfully created new semaphore1!\n");
	}
	else if (errno == EEXIST)
	{ // Semaphore already exists
		printf("Semaphore appears to exist already!\n");
		param_access_semaphore1 = sem_open(PARAM_ACCESS_SEMAPHORE1, 0);
	}
	else
	{ // An other error occured
		assert(param_access_semaphore1 != SEM_FAILED);
		exit(-1);
	}

	if (param_access_semaphore2 != SEM_FAILED)
	{
		printf("Successfully created new semaphore2!\n");
	}
	else if (errno == EEXIST)
	{ // Semaphore already exists
		printf("Semaphore appears to exist already!\n");
		param_access_semaphore2 = sem_open(PARAM_ACCESS_SEMAPHORE2, 0);
	}
	else
	{ // An other error occured
		assert(param_access_semaphore2 != SEM_FAILED);
		exit(-1);
	}

	traverseDir(dir_name);

	// printf("\n\n\n\n");
	// for (int i = 0; i < num; i++) {
	// 	printf("%s \n", paths[i]);
	// }

	/////////////////////////////////////////////////
	// You can add some code here to prepare before fork.
	/////////////////////////////////////////////////
	int shmid;
	char *shared_param_p, *shared_param_c;

	shmid = shmget(IPC_PRIVATE, SHM_SIZE, 0666 | IPC_CREAT);

	// Calculate looptime
	for (int i = 0; i < num; i++)
	{
		FILE *fp = fopen(paths[i], "r");
		looptime += (fileLength(fp) > SHM_SIZE ? fileLength(fp) : SHM_SIZE);
		fclose(fp);
	}
	looptime = (long)(((double)looptime / SHM_SIZE) + 0.999999);

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
		//memset(shared_param_p, '\0', SHM_SIZE);
		for (size_t i = 0; i < num; i++)
		{

			// Wait for permission to access shared memory
			sem_wait(param_access_semaphore1);
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
					sem_post(param_access_semaphore2);
					sem_wait(param_access_semaphore1);
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
			sem_post(param_access_semaphore2);
		}

		for (int i = 0; i < num; i++)
		{
			free(paths[i]);
		}

		// FILE* fp = fopen(paths[0], "r");
		// size_t bytes_read = 0;
		// bytes_read += fread(shared_param_p, sizeof(char), fileLength(fp), fp);
		// printf("file %s:   %s bytes\n", paths[0], shared_param_p);

		wait(NULL);
		printf("Parent process: Finished.\n");

		shmdt(shared_param_p);
		shmctl(shmid, IPC_RMID, 0);

		sem_close(param_access_semaphore1);
		sem_unlink(PARAM_ACCESS_SEMAPHORE1);
		sem_close(param_access_semaphore2);
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
		char *shared_param_c = (char *)shmat(shmid, NULL, 0);
		int totalWord = 0;
		thread_data thread_data_array[4];

		for (int i = 0; i < 4; i++)
		{
			thread_data_array[i].segment_ptrs = shared_param_c + i * segment_length;
			thread_data_array[i].threadid = i + 1;
			thread_data_array[i].result = 0;
		}

		pthread_t threads[4];
		for (long i = 0; i < looptime; i++)
		{
			printf("loop for file %lu\n", i);
			sem_wait(param_access_semaphore2);

			// printf("this is orignal pointer%s\n", shared_param_c);
			// for (int i = 0; i < 4; i++)
			// {
			// 	printf("this is %d pointer%s\n", i, thread_data_array[i].segment_ptrs);
			// }

			// for (int i = 0; i < 4; i++)
			// {
			// 	char c = *(thread_data_array[i].segment_ptrs + segment_length);
			// 	printf("\nThe ASCII Value of %c is %d", c, c);
			// 	if (c != '\0' || c != ' ' || c != '\n')
			// 	{
			// 		thread_data_array[i].result = -1;
			// 	}
			// 	else
			// 	{
			// 		thread_data_array[i].result = 1;
			// 	}
			// 	printf("this is the result init vlaue of %d is %d\n", i, thread_data_array[i].result);
			// }

			for (int i = 0; i < 4; i++)
			{
				pthread_create(&threads[i], NULL, threadFunc, &thread_data_array[i]);
			}

			for (int i = 0; i < 4; i++)
			{
				pthread_join(threads[i], NULL);
			}
			for (int i = 0; i < 4; i++)
			{
				totalWord += thread_data_array[i].result;
				thread_data_array[i].result = 0;
			}

			memset(shared_param_c, '\0', SHM_SIZE);
			sem_post(param_access_semaphore1);
		}

		saveResult("p2_result.txt", totalWord);

		printf("word count is: %d\n", totalWord);
		printf("Child process: Finished.\n");
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
 */
void traverseDir(char *dir_name)
{

	// Implement your code here to find out
	// all textfiles in the source directory.

	DIR *dir = opendir(dir_name);
	struct dirent *entity;
	if (dir == NULL)
	{
		return;
	}

	entity = readdir(dir); // read all the objects in the directory
	while ((entity = readdir(dir)) != NULL)
	{ // read each object 1 by 1

		if (strcmp(entity->d_name, ".") != 0 && strcmp(entity->d_name, "..") != 0)
		{
			// recording the current path.
			char local_Path[100] = {0};
			strcat(local_Path, dir_name);
			strcat(local_Path, "/");
			strcat(local_Path, entity->d_name);

			if (validateTextFile(entity->d_name) == 0)
			{
				traverseDir(local_Path);
			}
			else
			{
				printf("%s\n", local_Path);
				int length = sizeof(local_Path);
				paths[num] = (char *)malloc(length);
				strcpy(paths[num++], local_Path);
			}
		}
	}
}
