#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

struct wordData {
    char *word;
    char *fileName;
};

struct wordData *wordDataArray = NULL;
int counter = 0, arraySize = 8;
pthread_mutex_t mutex;

int inArray(char *value, int size);

int isTxt(char *fileName);

void *readFromFile(char *file);

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr,
                "\nERROR: Invalid Argument Number. \n\nUSAGE: ./a.out -d <directoryName> -n <numberOfThreads>\n\n");
        exit(-1);
    } else if (strcmp(argv[1], "-d")) {
        fprintf(stderr, "\nERROR: Invalid input. \n\nUSAGE: ./a.out -d <directoryName> -n <numberOfThreads>\n\n");
        exit(-1);
    } else if (strcmp(argv[3], "-n")) {
        fprintf(stderr, "\nERROR: Invalid input. \n\nUSAGE: ./a.out -d <directoryName> -n <numberOfThreads>\n\n");
        exit(-1);
    } else {
        char *isNumber = argv[4];
        strtol(isNumber, &isNumber, 10);
        if (*isNumber != '\0') {
            fprintf(stderr,
                    "\nERROR: Thread number must be an integer. \n\nUSAGE: ./a.out -d <directoryName> -n <numberOfThreads>\n\n");
            exit(-1);
        }
    }

    DIR *directory = NULL;
    char *directoryName = argv[2];
    directory = opendir(directoryName);

    if (directory == NULL) {
        char currentDir[256];
        if (getcwd(currentDir, sizeof(currentDir)) != NULL) {
            strcat(currentDir, "/");
            strcat(currentDir, argv[2]);
        }
        directory = opendir(currentDir);
        if (directory == NULL) {
            fprintf(stderr,
                    "\nERROR: \"%s\" directory does not exist. \n\nUSAGE: ./a.out -d <directoryName> -n <numberOfThreads>\n\n",
                    argv[2]);
            exit(-1);
        }
    }

    int i, j, threadStatus, threadNumber = atoi(argv[4]), fileNumber = 0;
    pthread_t threads[threadNumber];
    struct dirent *ent;
    pthread_mutex_init(&mutex, NULL);
    wordDataArray = malloc(sizeof(struct wordData) * arraySize);
    printf("\n\nMAIN THREAD: Allocated initial array of 8 pointers.\n\n");
    for (i = 0; (ent = readdir(directory)) != NULL;) {
        if (isTxt(ent->d_name)) {
            threadStatus = pthread_create(&threads[i], NULL, readFromFile, (void *) &ent->d_name);
            if (threadStatus) {
                fprintf(stderr, "\nERROR: return code from pthread_create() is %d.\n", threadStatus);
                exit(-1);
            }
            i++;
            fileNumber++;
        } else {
            continue;
        }

        if (i > threadNumber - 1) {
            for (j = 0; j < i; j++) {
                threadStatus = pthread_join(threads[j], NULL);
                if (threadStatus) {
                    fprintf(stderr, "\nERROR: return code from pthread_join() is %d.\n", threadStatus);
                    exit(-1);
                }
            }
            i = 0;
        }
    }

    if (closedir(directory) < 0) {
        exit(-1);
    }

    for (j = 0; j < i; j++) {
        threadStatus = pthread_join(threads[j], NULL);
        if (threadStatus) {
            fprintf(stderr, "\nERROR: return code from pthread_join() is %d.\n", threadStatus);
            exit(-1);
        }
    }
    printf("MAIN THREAD: All done (successfully read %d words with %d threads from %d files).\n\n", counter,
           threadNumber, fileNumber);
    pthread_mutex_destroy(&mutex);
    free(wordDataArray);
    pthread_exit(NULL);
}


void *readFromFile(char *file) {
    printf("MAIN THREAD: Assigned \"%s\" to worker thread %d.\n\n", file, (int) pthread_self());
    FILE *fp = fopen(file, "r");
    char newWord[32];
    int a;

    while (fscanf(fp, "%s", newWord) != EOF) {
        pthread_mutex_lock(&mutex);
        a = inArray(newWord, counter);
        if (!a) {
            if (counter == arraySize) {
                arraySize *= 2;
                wordDataArray = realloc(wordDataArray, arraySize * sizeof(struct wordData));
                printf("THREAD %d: Re-allocated array of %d pointers.\n\n", (int) pthread_self(), arraySize);
            }
            asprintf(&wordDataArray[counter].word, "%s", newWord);
            asprintf(&wordDataArray[counter].fileName, "%s", file);
            printf("THREAD %d: Added \"%s\" at index %d.\n\n", (int) pthread_self(), &newWord, counter);
            counter++;
        } else {
            printf("THREAD %d: The word \"%s\" has already located at index %d.\n\n", (int) pthread_self(), &newWord,
                   a);
        }
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

int inArray(char *value, int size) {
    int i;
    for (i = 0; i < size; i++) {
        if (!strcmp(wordDataArray[i].word, value)) {
            return i;
        }
    }
    return 0;
}

int isTxt(char *fileName) {
    size_t len = strlen(fileName);
    if (len > 4 && strcmp(fileName + len - 4, ".txt") == 0) {
        return 1;
    }
    return 0;
}