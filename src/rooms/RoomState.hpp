#pragma once
#include <pthread.h>
#include "RoomType.hpp"
#include <semaphore.h>

constexpr int MAX_ROOM_CAPACITY = 100;
constexpr int CELL_CAPACITY = 2;

struct RoomState {
    RoomType type;
    int capacity;
    int occupants[MAX_ROOM_CAPACITY];
    int occupantCount;

    sem_t cleanBedding;   // 1 = czysta pościel dostępna
    sem_t dirtyBedding;

    pthread_mutex_t mutex;
};