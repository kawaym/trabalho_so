#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t can_pair;
    pthread_cond_t can_leave;
    int diners;               // number of students currently dining
    int waiting_to_pair;      // students waiting for a partner before entering
    bool lonely_leaver_waiting; // whether a student is waiting to avoid leaving someone alone
} DiningHallState;

static DiningHallState hall = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .can_pair = PTHREAD_COND_INITIALIZER,
    .can_leave = PTHREAD_COND_INITIALIZER,
    .diners = 0,
    .waiting_to_pair = 0,
    .lonely_leaver_waiting = false
};

static void log_state(const char *event, int id) {
    printf("[hall] %s (student %d) -> diners=%d\n", event, id, hall.diners);
}

static void wake_waiting_pair_if_possible(void) {
    if (hall.waiting_to_pair > 0 && hall.diners > 0) {
        // Someone is waiting outside and it is now safe to let them in.
        pthread_cond_signal(&hall.can_pair);
    }
}

void dine(int id) {
    pthread_mutex_lock(&hall.lock);

    if (hall.diners == 0) {
        if (hall.waiting_to_pair > 0) {
            hall.waiting_to_pair--;
            hall.diners += 2;
            pthread_cond_signal(&hall.can_pair);
        } else {
            hall.waiting_to_pair++;
            do {
                pthread_cond_wait(&hall.can_pair, &hall.lock);
            } while (hall.diners == 0);
            // Partner has already accounted for both diners.
        }
    } else {
        hall.diners++;
    }

    // More diners means it may be safe for a waiting leaver to depart now.
    log_state("dine entered", id);
    pthread_cond_broadcast(&hall.can_leave);
    pthread_mutex_unlock(&hall.lock);
}

void leave(int id) {
    pthread_mutex_lock(&hall.lock);

    if (hall.diners > 2) {
        hall.diners--;
        log_state("leave immediate", id);
        wake_waiting_pair_if_possible();
        pthread_mutex_unlock(&hall.lock);
        return;
    }

    // From here we know hall.diners is exactly 2.
    if (!hall.lonely_leaver_waiting) {
        hall.lonely_leaver_waiting = true;
        do {
            pthread_cond_wait(&hall.can_leave, &hall.lock);
            if (hall.diners > 2) {
                hall.lonely_leaver_waiting = false;
                hall.diners--;
                log_state("leave resumed", id);
                wake_waiting_pair_if_possible();
                pthread_mutex_unlock(&hall.lock);
                return;
            }
        } while (hall.lonely_leaver_waiting);
        log_state("leave paired", id);
        wake_waiting_pair_if_possible();
        pthread_mutex_unlock(&hall.lock);
        return;
    }

    // Pair with the waiting leaver so neither is left alone.
    hall.lonely_leaver_waiting = false;
    hall.diners -= 2;
    log_state("leave partner", id);
    pthread_cond_signal(&hall.can_leave);
    wake_waiting_pair_if_possible();
    pthread_mutex_unlock(&hall.lock);
}

static void *student(void *arg) {
    int id = *(int *)arg;

    dine(id);
    printf("student %d is dining\n", id);
    usleep((rand() % 400 + 100) * 1000); // simulate dining time
    leave(id);
    printf("student %d left\n", id);

    return NULL;
}

int main(int argc, char *argv[]) {
    int student_count = (argc > 1) ? atoi(argv[1]) : 8;
    if (student_count < 2) {
        fprintf(stderr, "need at least two students to keep nobody alone\n");
        return EXIT_FAILURE;
    }

    srand((unsigned int)getpid());

    pthread_t *threads = calloc((size_t)student_count, sizeof(pthread_t));
    int *ids = calloc((size_t)student_count, sizeof(int));
    if (!threads || !ids) {
        perror("calloc");
        free(threads);
        free(ids);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < student_count; ++i) {
        ids[i] = i + 1;
        if (pthread_create(&threads[i], NULL, student, &ids[i]) != 0) {
            perror("pthread_create");
            student_count = i;
            break;
        }
        usleep(50000); // stagger arrivals slightly
    }

    for (int i = 0; i < student_count; ++i) {
        if (threads[i] != 0) {
            pthread_join(threads[i], NULL);
        }
    }

    free(threads);
    free(ids);

    return EXIT_SUCCESS;
}
