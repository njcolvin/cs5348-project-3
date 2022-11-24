#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include "common.h"
#include "common_threads.h"
#include <sys/queue.h>

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

void error(char *message) {
    fprintf(stderr, "%s", message);
    exit(EXIT_FAILURE);
}

const int num_arguments = 5;
pthread_t *s_threads, *t_threads, *c_thread;
// lock for num_tutored
sem_t num_tutored_free;
// lock for num_free_chairs
sem_t chair_free;
// customer_is_waiting
sem_t tutor_ready;
// barber_is_waiting
sem_t student_ready;
// waiting room lock
sem_t waiting_room_free;
// priority rooms lock
sem_t priority_rooms_free;
// coordinator_ready
sem_t coordinator_ready;
// student tutors lock
sem_t student_tutors_free;
// busy_tutors lock
sem_t busy_tutors_free;
// total_requests lock
sem_t total_requests_free;
int students, tutors, chairs, help, num_free_chairs, num_tutored, *student_tutors, busy_tutors, total_requests;
struct FIFO *waiting_room, *priority_rooms;


void enqueue(struct FIFO *fifo, struct Student _value) {
    struct Node *new_node = (struct Node *) malloc(sizeof(struct Node));
    new_node->value = _value;

    if (fifo->front == NULL) {
        fifo->front = new_node;
        fifo->back = fifo->front;
    } else {
        fifo->back->next = new_node;
        fifo->back = fifo->back->next;
    }
}

struct Student dequeue(struct FIFO *fifo) {
    if (fifo->front == NULL)
        error("Error. Nothing to dequeue.\n");

    struct Student value = fifo->front->value;
    fifo->front = fifo->front->next;

    return value;
}

// generate a random number between 0 and 1 inclusive
double RandomFraction() {
    return (double) rand() / (double) RAND_MAX;
}

void *StudentThread(void *arg) {
    int id;
    struct Student this_student;

    id = *(int*)arg;
    this_student = (struct Student) {id, 0};

    // student programs for a random interval between 0 and 2ms
    // then checks if there is a free chair in the waiting room
    while (this_student.helped_times < help) {
        // printf("IN STUDENT THREAD\n");
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
            total_requests++;        
            printf("S: Student %d takes a seat. Empty chairs = %d.\n", id, num_free_chairs);

            if (sem_post(&chair_free) != 0)
                error("Error. Failed to post chair_free.\n");
            
            // student is now in the waiting area
            // enqueue ourselves before signaling to the coordinator
            if (sem_wait(&waiting_room_free) != 0)
                error("Error. Failed to wait for waiting_room_free.\n");
            
            enqueue(waiting_room, this_student);
            
            if (sem_post(&waiting_room_free) != 0)
                error("Error. Failed to post waiting_room_free.\n");
            
            // let the coordinator know I am waiting
            if (sem_post(&student_ready) != 0)
                error("Error. Failed to post student_ready.\n");

            // wait for the tutor to call me
            while (student_tutors[this_student.id] == -1);

            // TODO: get tutored
            printf("S: Student %d received help from Tutor %d.\n", id, student_tutors[this_student.id]);

            // I was called so increment num_free_chairs
            // get num_chair semaphore
            if (sem_wait(&chair_free) != 0)
                error("Error. Failed to wait for chair_free.\n");
            // printf("student %d acquired chair_free again. num_free_chairs = %d\n", id, num_free_chairs);
            num_free_chairs++;
            // printf("student %d incremented num_free_chairs. num_free_chairs = %d\n", id, num_free_chairs);
            if (sem_post(&chair_free) != 0)
                error("Error. Failed to post chair_free.\n");



            this_student.helped_times++;

            if (sem_wait(&student_tutors_free) != 0)
                error("Error. Failed to wait for student_tutors_free.\n");

            student_tutors[this_student.id] = -1;

            if (sem_post(&student_tutors_free) != 0)
                error("Error. Failed to post student_tutors_free.\n");
        }
    }
    //printf("DONE STUDENT THREAD %d\n",id);
    return 0;
}

void *TutorThread(void *arg) {
    int id;
    id = *(int*)arg;

    int tutor_done = 0;

    while (!tutor_done && num_tutored < students * help) {
        //printf("IN TUTOR THREAD %d\n",id);
        // printf("Tutor %d waiting for a student\n", id);

        if (sem_wait(&coordinator_ready) != 0)
            error("Error. Failed to wait for coordinator_ready.\n");
            
        //printf("WAITING FOR priority_rooms_free IN TUTOR %d\n",id);
        if (sem_wait(&priority_rooms_free) != 0)
            error("Error. Failed to wait for priority_rooms_free.\n");
        
        struct Student current_student = (struct Student) {-1, -1};

        for (int i = 0; i < help; i++) {
            if (priority_rooms[i].front != NULL) {
                current_student = dequeue(&priority_rooms[i]);
                break;
            }
        }

        if (sem_post(&priority_rooms_free) != 0)
            error("Error. Failed to post priority_rooms_free.\n");

        if (current_student.id == -1)
            continue;


        //printf("WAITING FOR busy_tutors_free IN TUTOR %d\n",id);
        if (sem_wait(&busy_tutors_free) != 0)
            error("Error. Failed to wait for busy_tutors_free.\n");
        
        busy_tutors++;

        if (sem_post(&busy_tutors_free) != 0)
            error("Error. Failed to post busy_tutors_free.\n");

        // printf("Tutor %d is here.\n", id);
        usleep(200);

        //printf("WAITING FOR busy_tutors_free 2.0 IN TUTOR %d\n",id);
        if (sem_wait(&busy_tutors_free) != 0)
            error("Error. Failed to wait for busy_tutors_free.\n");
        
        busy_tutors--;
        num_tutored++;

        if (num_tutored + busy_tutors == help * students)
            tutor_done = 1;

        if (sem_post(&busy_tutors_free) != 0)
            error("Error. Failed to post busy_tutors_free.\n");

        // TODO: tutor
        // TODO: start over
        // TODO: exit after all students are helped (I might not be the tutor who helped last student)
        // if (sem_wait(&num_tutored_free) != 0)
        //     error("Error. Failed to wait for num_tutored_free.\n");
        // num_tutored++;
        // // printf("Tutor %d incremented help count. Help count = %d\n", id, num_tutored);
        // if (sem_post(&num_tutored_free) != 0)
        //     error("Error. Failed to post num_tutored_free.\n");

        printf("T: Student %d tutored by Tutor %d. Students tutored now = %d. Total sessions tutored = %d\n", current_student.id, id, busy_tutors, num_tutored);

        //printf("WAITING FOR student_tutors_free IN TUTOR %d\n",id);
        // let the student know I am ready
        if (sem_wait(&student_tutors_free) != 0)
            error("Error. Failed to wait for student_tutors_free.\n");

        student_tutors[current_student.id] = id;

        if (sem_post(&student_tutors_free) != 0)
            error("Error. Failed to post student_tutors_free.\n");
        
        if (tutor_done) {
            for (int i = 0; i < tutors - 1; i++) {
                if (sem_post(&coordinator_ready) != 0)
                    error("Error. Failed to post coordinator_ready.\n");
            }
        }
    }

    //printf("DONE TUTOR THREAD %d\n",id);
    
    return 0;
}

void *CoordinatorThread(void *arg) {

    int coordinator_done = 0;
    int waiting_students_now = 0;

    while (!coordinator_done) {
        // wait for a student to call me
        //printf("coordinator waiting for student_ready\n");
        if (sem_wait(&student_ready) != 0)
            error("Error. Failed to wait for student_ready.\n");

        if (sem_wait(&waiting_room_free) != 0)
            error("Error. Failed to wait for waiting_room_free.\n");
        
        struct Student current_student = dequeue(waiting_room);
        
        if (sem_post(&waiting_room_free) != 0)
            error("Error. Failed to post waiting_room_free.\n");

        if (sem_wait(&priority_rooms_free) != 0)
            error("Error. Failed to wait for priority_rooms_free.\n");
        
        enqueue(&priority_rooms[current_student.helped_times], current_student);
        
        if (sem_post(&priority_rooms_free) != 0)
            error("Error. Failed to post priority_rooms_free.\n");

        // int waiting_students_now = chairs - num_free_chairs;
        // int total_requests = num_tutored + busy_tutors + waiting_students_now;
        if (sem_wait(&chair_free) != 0)
            error("Error. Failed to wait for chair_free.\n");

        waiting_students_now = chairs - num_free_chairs;
        printf("C: Student %d with priority %d added to the queue. Waiting students now = %d. Total requests = %d\n",
                current_student.id, current_student.helped_times, waiting_students_now, total_requests);

        if (total_requests == help * students)
            coordinator_done = 1;

        if (sem_post(&chair_free) != 0)
            error("Error. Failed to post chair_free.\n");

        if (sem_post(&coordinator_ready) != 0)
            error("Error. Failed to post coordinator_ready.\n");

        
    }
    //printf("DONE COORDINATOR THREAD \n");
    if (waiting_students_now > 0) {
        struct Student current_student;

        for (int i = 0; i < waiting_students_now; i++) {

            if (sem_wait(&waiting_room_free) != 0)
                error("Error. Failed to wait for waiting_room_free.\n");

            if (waiting_room->front != NULL) {
                current_student = dequeue(waiting_room);

                if (sem_post(&waiting_room_free) != 0)
                    error("Error. Failed to post waiting_room_free.\n");

                if (sem_wait(&priority_rooms_free) != 0)
                    error("Error. Failed to wait for priority_rooms_free.\n");
                
                enqueue(&priority_rooms[current_student.helped_times], current_student);
                
                if (sem_post(&priority_rooms_free) != 0)
                    error("Error. Failed to post priority_rooms_free.\n");

                // int waiting_students_now = chairs - num_free_chairs;
                // int total_requests = num_tutored + busy_tutors + waiting_students_now;
                if (sem_wait(&chair_free) != 0)
                    error("Error. Failed to wait for chair_free.\n");

                waiting_students_now = chairs - num_free_chairs;
                printf("C: Student %d with priority %d added to the queue. Waiting students now = %d. Total requests = %d\n",
                        current_student.id, current_student.helped_times, waiting_students_now, total_requests);

                if (total_requests == help * students)
                    coordinator_done = 1;

                if (sem_post(&chair_free) != 0)
                    error("Error. Failed to post chair_free.\n");

            }

            if (sem_post(&waiting_room_free) != 0)
                error("Error. Failed to post waiting_room_free.\n");

            if (sem_post(&coordinator_ready) != 0)
                error("Error. Failed to post coordinator_ready.\n");
        }
    }
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
    busy_tutors = 0;
    total_requests = 0;

    if (students < 0 || tutors < 0 || chairs < 0 || help < 0)
        error("Error. Arguments must be nonnegative.\n");


    // init arrays, queues

    student_tutors = (int *) malloc(students * sizeof(int));
    waiting_room = (struct FIFO *) malloc(sizeof(struct FIFO));
    priority_rooms = (struct FIFO *) malloc(help * sizeof(struct FIFO));

    // init threads, locks, and semaphores

    s_threads = (pthread_t*) malloc(students * sizeof(pthread_t));
    t_threads = (pthread_t*) malloc(tutors * sizeof(pthread_t));
    c_thread = (pthread_t*) malloc(sizeof(pthread_t));

    if (sem_init(&chair_free, 0, 1) != 0)
        error("Error. Failed to init chair_free semaphore\n");

    if (sem_init(&num_tutored_free, 0, 1) != 0)
        error("Error. Failed to init num_tutored_free semaphore\n");

    if (sem_init(&tutor_ready, 0, 0) != 0)
        error("Error. Failed to init tutor_ready semaphore\n");

    if (sem_init(&student_ready, 0, 0) != 0)
        error("Error. Failed to init student_ready semaphore\n");

    if (sem_init(&waiting_room_free, 0, 1) != 0)
        error("Error. Failed to init waiting_room_free semaphore\n");

    if (sem_init(&priority_rooms_free, 0, 1) != 0)
        error("Error. Failed to init priority_rooms_free semaphore\n");

    if (sem_init(&student_tutors_free, 0, 1) != 0)
        error("Error. Failed to init student_tutors_free semaphore\n");

    if (sem_init(&busy_tutors_free, 0, 1) != 0)
        error("Error. Failed to init busy_tutors_free semaphore\n");

    if (sem_init(&total_requests_free, 0, 1) != 0)
        error("Error. Failed to init total_requests_free semaphore\n");

    // start threads
    int i;

    for (i = 0; i < students; i++)
        student_tutors[i] = -1;

    for (i = 0; i < students; i++) {
        //printf("creating student thread %d\n", i);
        void *ptr = malloc(sizeof(int));
        *(int*)ptr = i;
        Pthread_create(&s_threads[i], NULL, StudentThread, ptr);
    }

    for (i = 0; i < tutors; i++) {
        //printf("creating tutor thread %d\n", i);
        void *ptr = malloc(sizeof(int));
        *(int*)ptr = i;
        Pthread_create(&t_threads[i], NULL, TutorThread, ptr);
    }

    //printf("creating coordinator thread 0\n");
    Pthread_create(c_thread, NULL, CoordinatorThread, NULL);

    // join threads

    for (i = 0; i < students; i++) {
        //printf("joining student thread %d\n", i);
        Pthread_join(s_threads[i], NULL);
    }

    for (i = 0; i < tutors; i++) {
        //printf("joining tutor thread %d\n", i);
        Pthread_join(t_threads[i], NULL);
    }

    //printf("joining coordinator thread 0\n");
    Pthread_join(*c_thread, NULL);

    // free memory

    free(s_threads);
    free(t_threads);
    free(c_thread);
    free(student_tutors);
    free(waiting_room);
    free(priority_rooms);
}