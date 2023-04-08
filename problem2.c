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
#define PARAM_ACCESS_SEMAPHORE "/param_access_semaphore001"

char* paths[100]; //for storing the accessing paths of the text files
int num = 0; //the number of text files in the directory

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

	traverseDir(dir_name);

	printf("\n\n\n\n");
	for (int i = 0; i < num; i++) {
		printf("%s \n", paths[i]);
	}

    /////////////////////////////////////////////////
    // You can add some code here to prepare before fork.
    /////////////////////////////////////////////////
    int shmid, status;
	// char *shared_param_p, *shared_param_c;
	int totalWord=0;


	sem_unlink(PARAM_ACCESS_SEMAPHORE);
	// Create the semaphore. sem_init() also creates a semaphore. Learn the difference on your own.
  	sem_t * param_access_semaphore = sem_open(PARAM_ACCESS_SEMAPHORE, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);

	  // Check for error while opening the semaphore
	if (param_access_semaphore != SEM_FAILED) {
		printf("Successfully created new semaphore!\n");
	} else if (errno == EEXIST) { // Semaphore already exists
		printf("Semaphore appears to exist already!\n");
		param_access_semaphore = sem_open(PARAM_ACCESS_SEMAPHORE, 0);
	} else { // An other error occured
		assert(param_access_semaphore != SEM_FAILED);
		exit(-1);
	}
    shmid = shmget(IPC_PRIVATE, 1024*1024, 0666|IPC_CREAT);
    
    
	switch (process_id = fork()) {

	default:
		/*
			Parent Process
		*/
		printf("Parent process: My ID is %jd\n", (intmax_t) getpid());
        
        /////////////////////////////////////////////////
        // Implement your code for parent process here.
        /////////////////////////////////////////////////
		sem_wait(param_access_semaphore);
        char * shared_param_p = (char *)shmat(shmid, NULL, 0);

        int i = 0;
        // while(s[i] != NULL){
            // Open file for reading
            FILE * fp = fopen(paths[i++], "r");

            // size_t bytes_read = 0;
            // bytes_read += fread(shared_param_p, sizeof(char), min(SHM_SIZE,filelen()), fp);
            fread(shared_param_p, sizeof(char), fileLength(fp), fp);
			// size_t num_bytes = filelen() * sizeof(char);

        // }
		sem_post(param_access_semaphore);

		printf("Parent process: Finished.\n");
		break;

	case 0:
		/*
			Child Process
		*/

		printf("Child process: My ID is %jd\n", (intmax_t) getpid());

        /////////////////////////////////////////////////
        // Implement your code for child process here.
        /////////////////////////////////////////////////

		int count = 0;
		char fileName[13] = "p2_result.txt";

		sem_wait(param_access_semaphore);

        char * shared_param_c = (char *)shmat(shmid, NULL, 0);
		count += wordCount(shared_param_c);

		printf("\n count number=%d \n\n",count);
		saveResult(fileName,count);
		sem_post(param_access_semaphore);
		printf("Child process: Finished.\n");
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