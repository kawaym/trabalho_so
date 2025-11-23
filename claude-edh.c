/**
 * Extended Dining Hall Problem - Seção 7.6.3
 * Little Book of Semaphores
 * 
 * Restrição: Um estudante nunca pode sentar sozinho na mesa
 * - Não pode invocar dine se não há ninguém na mesa E ninguém pronto para comer
 * - Não pode ficar sozinho se todos os outros saírem antes dele terminar
 * 
 * Implementação: Usando APENAS mutex e variáveis de condição (SEM semáforos)
 * 
 * Uso: ./claude-edh [num_estudantes] [tempo_max_comida] [tempo_max_jantar]
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

// Parâmetros configuráveis
int NUM_STUDENTS = 10;
int MAX_GET_FOOD_TIME = 2;  // segundos
int MAX_DINING_TIME = 3;    // segundos

// Contadores de estado
int ready_to_eat = 0;    // Estudantes que pegaram comida mas ainda não sentaram
int eating = 0;          // Estudantes sentados comendo
int ready_to_leave = 0;  // Estudantes que terminaram de comer mas não saíram

// Mutex e variáveis de condição
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t can_sit = PTHREAD_COND_INITIALIZER;
pthread_cond_t can_leave = PTHREAD_COND_INITIALIZER;

// Para geração de números aleatórios thread-safe
unsigned int seed_value;

void print_status(int id, const char* action) {
    printf("[Estudante %2d] %-30s | Mesa: %d comendo | Prontos: %d para comer, %d para sair\n",
           id, action, eating, ready_to_eat, ready_to_leave);
}

// Gera número aleatório thread-safe
int random_time(int max_seconds) {
    return (rand_r(&seed_value) % (max_seconds * 1000)) + 500;  // 500ms a max_seconds ms
}

void getFood(int id) {
    int time_ms = random_time(MAX_GET_FOOD_TIME);
    printf("[Estudante %2d] Pegando comida... (%d ms)\n", id, time_ms);
    usleep(time_ms * 1000);
    
    pthread_mutex_lock(&mutex);
    ready_to_eat++;
    print_status(id, "Pronto para comer");
    pthread_cond_broadcast(&can_sit);  // Acorda estudantes esperando
    pthread_mutex_unlock(&mutex);
}

void dine(int id) {
    pthread_mutex_lock(&mutex);
    
    // Verifica se pode sentar: não pode ficar sozinho
    // Pode sentar se: (1) já tem alguém comendo OU (2) tem outro pronto para comer
    while (eating == 0 && ready_to_eat == 1) {
        // Seria o único na mesa e ninguém mais vem
        printf("[Estudante %2d] Esperando... (não pode sentar sozinho)\n", id);
        pthread_cond_wait(&can_sit, &mutex);  // Libera mutex e espera
    }
    
    ready_to_eat--;
    eating++;
    print_status(id, "Sentou na mesa");
    pthread_cond_broadcast(&can_sit);  // Acorda outros que podem sentar
    pthread_mutex_unlock(&mutex);
    
    // Come por tempo aleatório
    int time_ms = random_time(MAX_DINING_TIME);
    printf("[Estudante %2d] Comendo... (%d ms)\n", id, time_ms);
    usleep(time_ms * 1000);
    
    pthread_mutex_lock(&mutex);
    eating--;
    ready_to_leave++;
    print_status(id, "Terminou de comer");
    pthread_cond_broadcast(&can_sit);    // Mesa pode estar vazia, acorda quem espera
    pthread_cond_broadcast(&can_leave);  // Acorda quem espera para sair
    pthread_mutex_unlock(&mutex);
}

void leave(int id) {
    pthread_mutex_lock(&mutex);
    
    // Não pode sair se seria deixar alguém sozinho
    while (eating == 1 && ready_to_eat == 0) {
        // Tem 1 pessoa comendo e ninguém vindo
        printf("[Estudante %2d] Esperando para sair (não pode deixar alguém sozinho)...\n", id);
        pthread_cond_wait(&can_leave, &mutex);
    }
    
    ready_to_leave--;
    print_status(id, "Saiu");
    pthread_cond_broadcast(&can_sit);  // Se mesa ficou vazia, acorda quem espera
    pthread_mutex_unlock(&mutex);
    
    printf("[Estudante %2d] Deixou o refeitório\n", id);
}

void* student_thread(void* arg) {
    int id = *((int*)arg);
    free(arg);
    
    // Chegada aleatória (0 a 3 segundos)
    int arrival_time = random_time(3);
    usleep(arrival_time * 1000);
    printf("[Estudante %2d] Chegou ao refeitório (após %d ms)\n", id, arrival_time);
    
    getFood(id);
    dine(id);
    leave(id);
    
    return NULL;
}

void print_usage(const char* program) {
    printf("Uso: %s [num_estudantes] [tempo_max_comida] [tempo_max_jantar]\n", program);
    printf("\nParâmetros:\n");
    printf("  num_estudantes     : Número de estudantes (padrão: 10)\n");
    printf("  tempo_max_comida   : Tempo máximo para pegar comida em segundos (padrão: 2)\n");
    printf("  tempo_max_jantar   : Tempo máximo para jantar em segundos (padrão: 3)\n");
    printf("\nExemplo:\n");
    printf("  %s 15 3 5    # 15 estudantes, até 3s para comida, até 5s para jantar\n", program);
    printf("  %s           # Usa valores padrão\n", program);
}

int main(int argc, char* argv[]) {
    // Processa argumentos da linha de comando
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        
        NUM_STUDENTS = atoi(argv[1]);
        if (NUM_STUDENTS <= 0 || NUM_STUDENTS > 1000) {
            fprintf(stderr, "Erro: Número de estudantes deve estar entre 1 e 1000\n");
            return 1;
        }
    }
    
    if (argc > 2) {
        MAX_GET_FOOD_TIME = atoi(argv[2]);
        if (MAX_GET_FOOD_TIME <= 0) {
            fprintf(stderr, "Erro: Tempo de comida deve ser positivo\n");
            return 1;
        }
    }
    
    if (argc > 3) {
        MAX_DINING_TIME = atoi(argv[3]);
        if (MAX_DINING_TIME <= 0) {
            fprintf(stderr, "Erro: Tempo de jantar deve ser positivo\n");
            return 1;
        }
    }
    
    // Inicializa seed aleatória
    seed_value = time(NULL);
    srand(seed_value);
    
    pthread_t* students = malloc(NUM_STUDENTS * sizeof(pthread_t));
    if (!students) {
        fprintf(stderr, "Erro: Falha ao alocar memória\n");
        return 1;
    }
    
    printf("=== Extended Dining Hall Problem ===\n");
    printf("Regra: Ninguém pode ficar sozinho na mesa\n");
    printf("Configuração:\n");
    printf("  - Estudantes: %d\n", NUM_STUDENTS);
    printf("  - Tempo máximo para pegar comida: %d segundos\n", MAX_GET_FOOD_TIME);
    printf("  - Tempo máximo para jantar: %d segundos\n\n", MAX_DINING_TIME);
    
    for (int i = 0; i < NUM_STUDENTS; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&students[i], NULL, student_thread, id);
    }
    
    for (int i = 0; i < NUM_STUDENTS; i++) {
        pthread_join(students[i], NULL);
    }
    
    printf("\n=== Simulação concluída ===\n");
    printf("Todos os %d estudantes almoçaram sem ficar sozinhos!\n", NUM_STUDENTS);
    
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&can_sit);
    pthread_cond_destroy(&can_leave);
    free(students);
    
    return 0;
}
