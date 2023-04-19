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

char *paths[100];		 // for storing the accessing paths of the text files
int num = 0;			 // the number of text files in the directory
long looptime = 0;		 // the number of child process read data form shared memory
#define SHM_SIZE 1048576 // 1MB

<<<<<<< HEAD
#define PARAM_ACCESS_SEMAPHORE1 "/parent_semaphore" // parent semaphore
#define PARAM_ACCESS_SEMAPHORE2 "/child_semaphorea" // child semaphore


=======
#define PARAM_ACCESS_SEMAPHORE1 "/param_access_semaphore11" // parent semaphore
#define PARAM_ACCESS_SEMAPHORE2 "/param_access_semaphore22" // child semaphore

/**
 * @brief This function recursively traverse the source directory.
 *
 * @param dir_name : The source directory name.
 */
>>>>>>> f7afd30f85d7c769011a8509a3067b605d4d5a69
void traverseDir(char *dir_name);

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
<<<<<<< HEAD
	sem_t *parent_semaphore = sem_open(PARAM_ACCESS_SEMAPHORE1, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
	sem_t *child_semaphore = sem_open(PARAM_ACCESS_SEMAPHORE2, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);

	// Check for error while opening the semaphore
	if (parent_semaphore != SEM_FAILED)
	{
		printf("Successfully created new semaphore!\n");
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
		printf("Successfully created new semaphore!\n");
	}
	else if (errno == EEXIST)
	{ // Semaphore already exists
		printf("Semaphore appears to exist already!\n");
		parent_semaphore = sem_open(PARAM_ACCESS_SEMAPHORE2, 0);
	}
	else
	{ // An other error occured
		assert(child_semaphore != SEM_FAILED);
=======
	sem_t *param_access_semaphore1 = sem_open(PARAM_ACCESS_SEMAPHORE1, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
	sem_t *param_access_semaphore2 = sem_open(PARAM_ACCESS_SEMAPHORE2, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);

	// Check for error while opening the semaphore
	if (param_access_semaphore1 != SEM_FAILED)
	{
		printf("Successfully created new semaphore!\n");
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
		printf("Successfully created new semaphore!\n");
	}
	else if (errno == EEXIST)
	{ // Semaphore already exists
		printf("Semaphore appears to exist already!\n");
		param_access_semaphore1 = sem_open(PARAM_ACCESS_SEMAPHORE2, 0);
	}
	else
	{ // An other error occured
		assert(param_access_semaphore2 != SEM_FAILED);
>>>>>>> f7afd30f85d7c769011a8509a3067b605d4d5a69
		exit(-1);
	}

	// Traverse the directory and store the paths of the text files
	traverseDir(dir_name);

<<<<<<< HEAD
=======
	// printf("\n\n\n\n");
	// for (int i = 0; i < num; i++) {
	// 	printf("%s \n", paths[i]);
	// }

>>>>>>> f7afd30f85d7c769011a8509a3067b605d4d5a69
	/////////////////////////////////////////////////
	// You can add some code here to prepare before fork.
	/////////////////////////////////////////////////
	int shmid;
	char *shared_param_p, *shared_param_c;
	int totalWord = 0;

	shmid = shmget(IPC_PRIVATE, SHM_SIZE, 0666 | IPC_CREAT);

<<<<<<< HEAD
	// Calculate looptime, the calucation of looptime is divide total length of all files by 1MB and take the ceiling of the result
=======
	// Calculate looptime
>>>>>>> f7afd30f85d7c769011a8509a3067b605d4d5a69
	for (int i = 0; i < num; i++)
	{
		FILE *fp = fopen(paths[i], "r");
		looptime += (fileLength(fp) > SHM_SIZE ? fileLength(fp) : SHM_SIZE);
		fclose(fp);
	}
<<<<<<< HEAD
	looptime = (long)(((double)looptime / SHM_SIZE) + 0.9999999);
=======
	looptime = (long)(((double)looptime / SHM_SIZE) + 0.999999);
	printf("The loop time is %ld", looptime);
>>>>>>> f7afd30f85d7c769011a8509a3067b605d4d5a69

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
<<<<<<< HEAD

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
=======

		shared_param_p = (char *)shmat(shmid, NULL, 0);
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
			printf("last round");
			// Close the input file
			fclose(fp);
			// Signal that the current file is done being processed
			sem_post(param_access_semaphore2);
		}

>>>>>>> f7afd30f85d7c769011a8509a3067b605d4d5a69
		for (int i = 0; i < num; i++)
		{
			free(paths[i]);
		}

<<<<<<< HEAD
		// Wait for all child processes to complete
		wait(NULL);
=======
		wait(NULL);//wait until a state change in the child process
		printf("bug free.\n");
>>>>>>> f7afd30f85d7c769011a8509a3067b605d4d5a69
		printf("Parent process: Finished.\n");

		shmdt(shared_param_p);
		shmctl(shmid, IPC_RMID, 0);

<<<<<<< HEAD
		// Clean up resources and exit the parent process
		sem_close(parent_semaphore);
		sem_unlink(PARAM_ACCESS_SEMAPHORE1);
		sem_close(child_semaphore);
=======

		//finished the critical section, and release the semaphore
		sem_close(param_access_semaphore1);
		sem_unlink(PARAM_ACCESS_SEMAPHORE1);
		sem_close(param_access_semaphore2);
>>>>>>> f7afd30f85d7c769011a8509a3067b605d4d5a69
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

		for (long i = 0; i < looptime; i++)
		{
<<<<<<< HEAD
			sem_wait(child_semaphore);
			totalWord += wordCount(shared_param_c);
			sem_post(parent_semaphore);
		}

		saveResult("p2_result.txt", totalWord);

=======
			sem_wait(param_access_semaphore2);
			// long word = wordCount(shared_param_c);
			// printf("file %s has %ld words", paths[i], word);
			totalWord += wordCount(shared_param_c);
			// memset(shared_param_c, '\0', SHM_SIZE);
			sem_post(param_access_semaphore1);
		}

		//for debugging use, to check if the final value is correct
>>>>>>> f7afd30f85d7c769011a8509a3067b605d4d5a69
		printf("word count is: %d\n", totalWord);

		//save result to p2_result.txt
		saveResult("p2_result.txt", totalWord);


		printf("Child process: Finished.\n");

		shmdt(shared_param_c);
		exit(0);
		//end child process

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
<<<<<<< HEAD
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
=======

 //we use recursion to find all the directory and file under the request file directory
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
				traverseDir(local_Path);//do the recursion, going to the lower file directory
			}
			else
			{
				printf("%s\n", local_Path);
				int length = sizeof(local_Path);
				paths[num] = (char *)malloc(length);
				strcpy(paths[num++], local_Path);
				// to store the current path inside array "paths"
			}
		}
	}
>>>>>>> f7afd30f85d7c769011a8509a3067b605d4d5a69
}
