#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include "common.h"
#include "common_threads.h"

const int num_arguments = 5;
pthread_t *s_threads, *t_threads, *c_thread;

// generate a random number between 0 and 1 inclusive
double RandomFraction() {
    return (double) rand() / (double) RAND_MAX;
}

void *StudentThread(void *threadid) {
    int tid;
    tid = *(int*)threadid;
    double rand_frac = RandomFraction();

    // sleep for 2ms * random fraction
    printf("student %d sleeping for %lf...\n", tid, 2000 * rand_frac);
    usleep(2000 * rand_frac);
    printf("student %d awake from sleeping...\n", tid);

    return 0;
}

void *TutorThread(void *threadid) {
    int tid;
    tid = *(int*)threadid;
    printf("Hello World! Tutor Thread ID, %d\n", tid);
    return 0;
}

void *CoordinatorThread(void *threadid) {
    int tid;
    tid = *(int*)threadid;
    printf("Hello World! Coordinator Thread ID, %d\n", tid);
    return 0;
}

int main(int argc, char *argv[])
{
    int students, tutors, chairs, help;

    if (argc != num_arguments) {
        fprintf(stderr, "%s", "usage: csmc #students #tutors #chairs #help\n");
        exit(EXIT_FAILURE);
    }

    students = atoi(argv[1]);
    tutors = atoi(argv[2]);
    chairs = atoi(argv[3]);
    help = atoi(argv[4]);

    printf("%d students %d tutors %d chairs %d help\n", students, tutors, chairs, help);

    s_threads = (pthread_t*) malloc(students * sizeof(pthread_t));
    t_threads = (pthread_t*) malloc(tutors * sizeof(pthread_t));
    c_thread = (pthread_t*) malloc(sizeof(pthread_t));

    int i;

    for (i = 0; i < students; i++) {
        printf("creating student thread %d\n", i);
        void *ptr = malloc(sizeof(int));
        *(int*)ptr = i;
        Pthread_create(&s_threads[i], NULL, StudentThread, ptr);
    }

    for (i = 0; i < tutors; i++) {
        printf("creating tutor thread %d\n", i);
        void *ptr = malloc(sizeof(int));
        *(int*)ptr = i;
        Pthread_create(&t_threads[i], NULL, TutorThread, ptr);
    }

    printf("creating coordinator thread 0\n");
    void *ptr = malloc(sizeof(int));
    *(int*)ptr = 0;
    Pthread_create(c_thread, NULL, CoordinatorThread, ptr);

    for (i = 0; i < students; i++) {
        printf("joining student thread %d\n", i);
        Pthread_join(s_threads[i], NULL);
    }

    for (i = 0; i < tutors; i++) {
        printf("joining tutor thread %d\n", i);
        Pthread_join(t_threads[i], NULL);
    }

    printf("joining coordinator thread 0\n");
    Pthread_join(*c_thread, NULL);


    free(s_threads);
    free(t_threads);
    free(c_thread);

}