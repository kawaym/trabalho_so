// SO: Ubuntu 24.04.3 LTS

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t can_dine;
    pthread_cond_t can_leave;
    int ready;                // students who finished getFood and not yet seated
    int diners;               // students currently dining
    int waiting_to_dine;      // students blocked trying to be first at an empty table
    bool lonely_leaver_waiting;
} DiningHallState;

static DiningHallState hall = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .can_dine = PTHREAD_COND_INITIALIZER,
    .can_leave = PTHREAD_COND_INITIALIZER,
    .ready = 0,
    .diners = 0,
    .waiting_to_dine = 0,
    .lonely_leaver_waiting = false
};

static void log_state(const char *event, int id) {
    printf("[hall] %s (student %d) -> ready=%d diners=%d\n", event, id, hall.ready, hall.diners);
}

void getFood(int id) {
    pthread_mutex_lock(&hall.lock);
    hall.ready++;
    log_state("got food", id);
    pthread_cond_broadcast(&hall.can_dine);
    pthread_mutex_unlock(&hall.lock);
}

void dine(int id) {
    pthread_mutex_lock(&hall.lock);

    while (hall.ready <= 0) {
        pthread_cond_wait(&hall.can_dine, &hall.lock);
    }

    hall.ready--;
    bool counted = false;

    if (hall.diners == 0) {
        if (hall.waiting_to_dine > 0) {
            hall.waiting_to_dine--;
            hall.diners += 2;
            counted = true;
            pthread_cond_signal(&hall.can_dine);
        } else {
            hall.waiting_to_dine++;
            while (hall.diners == 0) {
                pthread_cond_wait(&hall.can_dine, &hall.lock);
            }
            counted = true;
        }
    }

    if (!counted) {
        hall.diners++;
    }

    log_state("started dining", id);
    pthread_cond_broadcast(&hall.can_leave);
    pthread_mutex_unlock(&hall.lock);
}

static void wake_waiting_diner_if_possible(void) {
    if (hall.waiting_to_dine > 0 && hall.ready > 0) {
        pthread_cond_signal(&hall.can_dine);
    }
}

void leave(int id) {
    pthread_mutex_lock(&hall.lock);

    if (hall.diners > 2) {
        hall.diners--;
        log_state("left immediately", id);
        wake_waiting_diner_if_possible();
        pthread_mutex_unlock(&hall.lock);
        return;
    }

    if (!hall.lonely_leaver_waiting) {
        hall.lonely_leaver_waiting = true;
        do {
            pthread_cond_wait(&hall.can_leave, &hall.lock);
            if (hall.diners > 2) {
                hall.lonely_leaver_waiting = false;
                hall.diners--;
                log_state("left after wait", id);
                wake_waiting_diner_if_possible();
                pthread_mutex_unlock(&hall.lock);
                return;
            }
        } while (hall.lonely_leaver_waiting);
        log_state("left paired", id);
        wake_waiting_diner_if_possible();
        pthread_mutex_unlock(&hall.lock);
        return;
    }

    hall.lonely_leaver_waiting = false;
    hall.diners -= 2;
    log_state("left with partner", id);
    pthread_cond_signal(&hall.can_leave);
    wake_waiting_diner_if_possible();
    pthread_mutex_unlock(&hall.lock);
}

static void *student(void *arg) {
    int id = *(int *)arg;

    getFood(id);
    printf("student %d got food\n", id);
    usleep((rand() % 300 + 100) * 1000);

    dine(id);
    printf("student %d started dining\n", id);
    usleep((rand() % 400 + 100) * 1000);

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
        usleep(50000);
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
