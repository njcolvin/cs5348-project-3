#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include "common.h"
#include "common_threads.h"

void error(char *message) {
    fprintf(stderr, "%s", message);
    exit(EXIT_FAILURE);
}

const int num_arguments = 5;
pthread_t *s_threads, *t_threads, *c_thread;
sem_t chair_free, tutor_ready, student_ready, num_tutored_free;
int students, tutors, chairs, help, num_free_chairs, num_tutored;

struct Student {
    int id, helped_times;
};

struct Node {
    struct Student value;
    struct Node *next;
};

struct FIFO {
    struct Node *front;
    struct Node *back;
};

void enqueue(struct FIFO fifo, struct Student _value) {
    struct Node *new_node = (struct Node *) malloc(sizeof(struct Node));
    new_node->value = _value;

    if (fifo.front == NULL) {
        fifo.front = new_node;
        fifo.back = fifo.front;
    } else {
        fifo.back->next = new_node;
        fifo.back = fifo.back->next;
    }
}

struct Student dequeue(struct FIFO fifo) {
    if (fifo.front == NULL)
        error("Error. Nothing to dequeue.\n");

    struct Student value = fifo.front->value;
    fifo.front = fifo.front->next;

    return value;
}

// generate a random number between 0 and 1 inclusive
double RandomFraction() {
    return (double) rand() / (double) RAND_MAX;
}

void *StudentThread(void *arg) {
    int id, tutored_times;

    id = *(int*)arg;
    tutored_times = 0;
    // student programs for a random interval between 0 and 2ms
    // then checks if there is a free chair in the waiting room
    while (tutored_times < help) {
        // sleep for 2ms * random fraction
        usleep(2000 * RandomFraction());

        // get num_chair semaphore
        if (sem_wait(&chair_free) != 0)
            error("Error. Failed to wait for chair_free.\n");

        // check num_free_chairs value
        if (num_free_chairs <= 0) {
            printf("S: Student %d found no empty chair. Will try again later.\n", id);

            if (sem_post(&chair_free) != 0)
                error("Error. Failed to post chair_free.\n");
        } else {
            num_free_chairs--;        
            printf("S: Student %d takes a seat. Empty chairs = %d.\n", id, num_free_chairs);

            if (sem_post(&chair_free) != 0)
                error("Error. Failed to post chair_free.\n");
            
            // student is now in the waiting area

            // let the coordinator know I am waiting
            if (sem_post(&student_ready) != 0)
                error("Error. Failed to post student_ready.\n");

            // wait for the tutor to call me
            if (sem_wait(&tutor_ready) != 0)
                error("Error. Failed to wait for tutor_ready.\n");

            // I was called so increment num_free_chairs
            // get num_chair semaphore
            if (sem_wait(&chair_free) != 0)
                error("Error. Failed to wait for chair_free.\n");
            printf("student %d acquired chair_free again. num_free_chairs = %d\n", id, num_free_chairs);
            num_free_chairs++;
            printf("student %d incremented num_free_chairs. num_free_chairs = %d\n", id, num_free_chairs);
            if (sem_post(&chair_free) != 0)
                error("Error. Failed to post chair_free.\n");

            // TODO: get tutored
            int tutor_id = 0;
            printf("S: Student %d received help from Tutor %d.\n", id, tutor_id);

            tutored_times++;
        }
    }

    return 0;
}

void *TutorThread(void *arg) {
    int id;
    id = *(int*)arg;

    while (num_tutored < help * students) {
        printf("Tutor %d waiting for a student\n", id);

        // wait for a student to call me
        if (sem_wait(&student_ready) != 0)
            error("Error. Failed to wait for student_ready.\n");
        
        // let the student know I am ready
        if (sem_post(&tutor_ready) != 0)
            error("Error. Failed to post tutor_ready.\n");

        printf("Tutor %d is here.\n", id);

        // TODO: tutor
        // TODO: start over
        // TODO: exit after all students are helped (I might not be the tutor who helped last student)
        if (sem_wait(&num_tutored_free) != 0)
            error("Error. Failed to wait for num_tutored_free.\n");
        num_tutored++;
        printf("Tutor %d incremented help count. Help count = %d\n", id, num_tutored);
        if (sem_post(&num_tutored_free) != 0)
            error("Error. Failed to post num_tutored_free.\n");
    }
    
    return 0;
}

void *CoordinatorThread(void *arg) {
    printf("Hello World! Coordinator Thread.\n");



    // TODO: everything

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
    num_tutored = 0;

    if (students < 0 || tutors < 0 || chairs < 0 || help < 0)
        error("Error. Arguments must be nonnegative.\n");

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

    if (sem_init(&num_tutored_free, 0, 1) != 0)
        error("Error. Failed to init num_tutored_free semaphore\n");

    // start threads
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
    Pthread_create(c_thread, NULL, CoordinatorThread, NULL);

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
}