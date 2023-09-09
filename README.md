# Concurrent Threads

This project is an adaptation of the [readers-writers problem](https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem) framed as the Seeking Tutor Problem.
In the problem, there are students, tutors, a waiting area with some chairs, and a coordinator.
A student seeks to be tutored a certain number of times, and waits in the waiting area according to their priority.
The coordinator queues students in the waiting area and assigns them to tutors as they become free.
Every student, tutor, and the coordinator run on separate threads.

To run this program, compile `gcc csmc.c –o csmc -Wall -Werror -pthreads -std=gnu99` and call it with arguments:
* `csmc #students #tutors #chairs #help`
* `csmc 10 3 4 5`
