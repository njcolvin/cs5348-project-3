#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include "common.h"
#include "common_threads.h"

const int num_arguments = 5;
pthread_t *s_threads, *t_threads;

void *PrintHello(void *threadid) {
    int tid;
    tid = *(int*)threadid;
    printf("Hello World! Student Thread ID, %d\n", tid);
    return 0;
}

void *PrintHelloTutor(void *threadid) {
    int tid;
    tid = *(int*)threadid;
    printf("Hello World! Tutor Thread ID, %d\n", tid);
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

    for (int i = 0; i < students; i++) {
        printf("creating student thread %d\n", i);
        void *ptr = malloc(sizeof(int));
        *(int*)ptr = i;
        Pthread_create(&s_threads[i], NULL, PrintHello, ptr);
    }

    for (int i = 0; i < tutors; i++) {
        printf("creating tutor thread %d\n", i);
        void *ptr = malloc(sizeof(int));
        *(int*)ptr = i;
        Pthread_create(&t_threads[i], NULL, PrintHelloTutor, ptr);
    }

    for (int i = 0; i < students; i++) {
        printf("joining student thread %d\n", i);
        Pthread_join(s_threads[i], NULL);
    }

    for (int i = 0; i < tutors; i++) {
        printf("joining tutor thread %d\n", i);
        Pthread_join(t_threads[i], NULL);
    }

    free(s_threads);
    free(t_threads);

}