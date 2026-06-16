#pragma once
#include <semaphore.h>
#include <pthread.h>

constexpr int MAX_MEALS = 20;
constexpr int MAX_QUEUE = 10;
constexpr int MAX_EATING_TIME = 5;
constexpr int MIN_EATING_TME = 2;

struct CantineState {
    int currentMeals;

    int qeueSize;
    sem_t qeueLine;
    int ticketDispenser;
    int nowServing;

    sem_t mealsAvailable;   
    sem_t freeSlots;        

    pthread_mutex_t mutex;
};