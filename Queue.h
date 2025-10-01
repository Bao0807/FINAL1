
#pragma once
#include "Order.h"

const int MAX = 100;

struct Queue {
    Order orders[MAX];
    int front = 0;
    int rear = -1;
    int count = 0;
};

bool isEmpty(Queue &q);
bool isFull(Queue &q);
bool enqueue(Queue &q, const Order &o);
bool dequeue(Queue &q, Order &o);

// Implementations
bool isEmpty(Queue &q) { return q.count == 0; }
bool isFull(Queue &q)  { return q.count == MAX; }

bool enqueue(Queue &q, const Order &o) {
    if (isFull(q)) return false;
    q.rear = (q.rear + 1) % MAX;
    q.orders[q.rear] = o;
    q.count++;
    return true;
}

bool dequeue(Queue &q, Order &o) {
    if (isEmpty(q)) return false;
    o = q.orders[q.front];
    q.front = (q.front + 1) % MAX;
    q.count--;
    return true;
}
