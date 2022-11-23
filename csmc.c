#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include "common.h"
#include "common_threads.h"

const int num_arguments = 5;
pthread_t *s_threads, *t_threads, *c_thread;
sem_t chair_free, tutor_ready, student_ready;
int students, tutors, chairs, help, num_free_chairs;

// generate a random number between 0 and 1 inclusive
double RandomFraction() {
    return (double) rand() / (double) RAND_MAX;
}

void error(char *message) {
    fprintf(stderr, "%s", message);
    exit(EXIT_FAILURE);
}

void *StudentThread(void *threadid) {
    int tid;
    tid = *(int*)threadid;
    int helped_times = 0;

    // student programs for a random interval between 0 and 2ms
    // then checks if there is a free chair in the waiting room
    while (helped_times < help) {
        // sleep for 2ms * random fraction
        usleep(2000 * RandomFraction());

        // get num_chair semaphore
        if (sem_wait(&chair_free) != 0)
            error("Error. Failed to wait for chair_free.\n");

        // check num_free_chairs value
        if (num_free_chairs <= 0) {
            printf("S: Student %d found no empty chair. Will try again later.\n", tid);

            if (sem_post(&chair_free) != 0)
                error("Error. Failed to post chair_free.\n");
        } else {
            num_free_chairs--;        
            printf("S: Student %d takes a seat. Empty chairs = %d.\n", tid, num_free_chairs);

            if (sem_post(&chair_free) != 0)
                error("Error. Failed to post chair_free.\n");
            
            // student is now in the waiting area

            // let the coordinator know I am waiting
            if (sem_post(&student_ready) != 0)
                error("Error. Failed to post student_ready.\n");

            // wait for the tutor to call me
            if (sem_wait(&tutor_ready) != 0)
                error("Error. Failed to wait for tutor_ready.\n");

            // increment num_free_chairs
            // get num_chair semaphore
            if (sem_wait(&chair_free) != 0)
                error("Error. Failed to wait for chair_free.\n");
            printf("student %d acquired chair_free again. num_free_chairs = %d\n", tid, num_free_chairs);
            num_free_chairs++;
            printf("student %d incremented num_free_chairs. num_free_chairs = %d\n", tid, num_free_chairs);
            if (sem_post(&chair_free) != 0)
                error("Error. Failed to post chair_free.\n");

            // TODO: get tutored

            helped_times++;

        }
    }

    return 0;
}

void *TutorThread(void *threadid) {
    int tid;
    tid = *(int*)threadid;
    printf("Hello World! Tutor Thread ID, %d\n", tid);

    // let the student know I am ready
    if (sem_post(&tutor_ready) != 0)
        error("Error. Failed to post tutor_ready.\n");

    // TODO: tutor
    // TODO: start over
    
    return 0;
}

void *CoordinatorThread(void *threadid) {
    int tid;
    tid = *(int*)threadid;
    printf("Hello World! Coordinator Thread ID, %d\n", tid);

    // wait for a student to call me
    if (sem_wait(&student_ready) != 0)
        error("Error. Failed to wait for student_ready.\n");

    

    return 0;
}

int main(int argc, char *argv[])
{

    // parse arguments

    if (argc != num_arguments)
        error("Error. Usage: csmc #students #tutors #chairs #help.\n");

    students = atoi(argv[1]);
    tutors = atoi(argv[2]);
    chairs = atoi(argv[3]);
    help = atoi(argv[4]);
    num_free_chairs = chairs;

    if (students < 0 || tutors < 0 || chairs < 0 || help < 0)
        error("Error. Arguments must be nonnegative.\n");

    printf("%d students %d tutors %d chairs %d help\n", students, tutors, chairs, help);

    // init threads, locks, and semaphores

    s_threads = (pthread_t*) malloc(students * sizeof(pthread_t));
    t_threads = (pthread_t*) malloc(tutors * sizeof(pthread_t));
    c_thread = (pthread_t*) malloc(sizeof(pthread_t));

    if (sem_init(&chair_free, 0, 1) != 0)
        error("Error. Failed to init chair_free semaphore\n");

    if (sem_init(&tutor_ready, 0, 0) != 0)
        error("Error. Failed to init tutor_ready semaphore\n");

    if (sem_init(&student_ready, 0, 0) != 0)
        error("Error. Failed to init student_ready semaphore\n");

    int i;

    // start threads

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

    // join threads

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

    // free memory

    free(s_threads);
    free(t_threads);
    free(c_thread);
    free(ptr);

}