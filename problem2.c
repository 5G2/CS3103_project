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

#include "helpers.h"

char* paths[100]; //for storing the accessing paths of the text files
int num = 0; //the number of text files in the directory
#define SHM_SIZE 1048576 // 1MB

#define PARAM_ACCESS_SEMAPHORE1 "/param_access_semaphore1" //parent semaphore
#define PARAM_ACCESS_SEMAPHORE2 "/param_access_semaphore2" //child semaphore

/**
 * @brief This function recursively traverse the source directory.
 * 
 * @param dir_name : The source directory name.
 */
void traverseDir(char *dir_name);

int main(int argc, char **argv) {
	int process_id; // Process identifier 
	
    // The source directory. 
    // It can contain the absolute path or relative path to the directory.
	char *dir_name = argv[1];

	if (argc < 2) {
		printf("Main process: Please enter a source directory name.\nUsage: ./main <dir_name>\n");
		exit(-1);
	}

	// Checks if the semaphore exists, if it exists we unlink him from the process.
	sem_unlink(PARAM_ACCESS_SEMAPHORE1);
	sem_unlink(PARAM_ACCESS_SEMAPHORE2);
	
	// Create the semaphore. sem_init() also creates a semaphore. Learn the difference on your own.
	sem_t *param_access_semaphore1 = sem_open(PARAM_ACCESS_SEMAPHORE1, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 1);
	sem_t *param_access_semaphore2 = sem_open(PARAM_ACCESS_SEMAPHORE2, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 0);

	// Check for error while opening the semaphore
	if (param_access_semaphore1 != SEM_FAILED){
		printf("Successfully created new semaphore!\n");
	}	
	else if (errno == EEXIST) {   // Semaphore already exists
		printf("Semaphore appears to exist already!\n");
		param_access_semaphore1 = sem_open(PARAM_ACCESS_SEMAPHORE1, 0);
	}
	else {  // An other error occured
		assert(param_access_semaphore1 != SEM_FAILED);
		exit(-1);
	}

	if (param_access_semaphore2 != SEM_FAILED){
		printf("Successfully created new semaphore!\n");
	}	
	else if (errno == EEXIST) {   // Semaphore already exists
		printf("Semaphore appears to exist already!\n");
		param_access_semaphore1 = sem_open(PARAM_ACCESS_SEMAPHORE2, 0);
	}
	else {  // An other error occured
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
	int totalWord=0;

    shmid = shmget(IPC_PRIVATE, SHM_SIZE, 0666|IPC_CREAT);
    
	switch (process_id = fork()) {
		
        default:
		/*
			Parent Process
		*/
		printf("Parent process: My ID is %jd\n", (intmax_t) getpid());
        
        /////////////////////////////////////////////////
        // Implement your code for parent process here.
        /////////////////////////////////////////////////
		
        shared_param_p = (char *)shmat(shmid, NULL, 0);
        for (size_t i = 0; i < num; i++) {

            // Wait for permission to access shared memory
            sem_wait(param_access_semaphore1);
            // Open the input file
            FILE* fp = fopen(paths[i], "r");
            // Get the size of the file, why minus 1 detail refer to helper.c
            size_t fileSize = fileLength(fp);
            // Initialize variables for tracking the last two spaces
            size_t lastspace = -1;
            // Initialize pointer for writing to shared memory
            int temppointer = 0;
            // Loop over the file contents
            for (long i = 0; i < fileSize; i++) {
                // Read the next character from the file
                char c = (char)fgetc(fp);
                // Check if the character is a space or a newline
                if (c == ' ' || c == '\n') {
                // Update the position of the last spaces
                lastspace = temppointer;
                }
                // Write the character to shared memory
                shared_param_p[temppointer] = c;
                // If the shared memory segment is full, check if the last character written was neither a space nor a newline
                if (temppointer == SHM_SIZE-1) {
					printf("exceed the size");
                    if (c != ' ' || c != '\n') {
                        // Replace the second last space with a dummy char
                        shared_param_p[lastspace] = 'A';
						// Add a null character in the end of shared memory
						shared_param_p[temppointer] = '\0';
                    }
                    // Reset the pointer and signal that the shared memory segment is ready for reading
                    temppointer = 0;
                    sem_post(param_access_semaphore2);
                    sem_wait(param_access_semaphore1);
                }
            }
			// Add a null character in the end of shared memory
			shared_param_p[(fileSize%SHM_SIZE)-1] = '\0';
			printf("last round");
            // Close the input file
            fclose(fp);
            // Signal that the current file is done being processed
            sem_post(param_access_semaphore2);

        }

		for (int i = 0; i < num; i++) {
			free(paths[i]);
		}

		// FILE* fp = fopen(paths[0], "r");
		// size_t bytes_read = 0;
		// bytes_read += fread(shared_param_p, sizeof(char), fileLength(fp), fp);
		// printf("file %s:   %s bytes\n", paths[0], shared_param_p);
		
		wait(NULL);
     	printf("bug free.\n");
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

		printf("Child process: My ID is %jd\n", (intmax_t) getpid());

        /////////////////////////////////////////////////
        // Implement your code for child process here.
        /////////////////////////////////////////////////
        char * shared_param_c = (char *)shmat(shmid, NULL, 0);

		for (int i = 0; i < num; i++) {
			sem_wait(param_access_semaphore2);
			//long word = wordCount(shared_param_c);
			//printf("file %s has %ld words", paths[i], word);
			totalWord += wordCount(shared_param_c);
			sem_post(param_access_semaphore1);
		}

		saveResult("p2_result.txt", totalWord);
		
		// totalWord += wordCount(shared_param_c);
		// printf("word count is: %d", totalWord);
		// saveResult("p2_result.txt", totalWord);

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
void traverseDir(char *dir_name){
   
    // Implement your code here to find out
    // all textfiles in the source directory.

	DIR *dir = opendir (dir_name); 
	struct dirent *entity;
	if (dir == NULL) {
		return;
	} 

	entity = readdir(dir); //read all the objects in the directory
	while ((entity = readdir(dir)) != NULL) { //read each object 1 by 1

		if (strcmp(entity->d_name, ".") != 0 && strcmp(entity->d_name, "..") != 0) {
			//recording the current path.
			char local_Path[100] = { 0 };
			strcat(local_Path, dir_name);
			strcat(local_Path, "/");
			strcat(local_Path, entity->d_name);

			if (validateTextFile(entity->d_name) == 0) {
				traverseDir(local_Path);
			}else {
				printf("%s\n", local_Path);
				int length = sizeof(local_Path);
				paths[num] = (char*) malloc(length);
				strcpy(paths[num++], local_Path);
			}
		}
	}
}