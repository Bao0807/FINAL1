#pragma once
#include "Order.h"

const int MAX = 1;

struct Queue {
    Order orders[MAX];
    int right = -1;
};


bool isEmpty(Queue &q){
    return q.right < 0;
}

bool isFull(Queue &q){
    return q.right == MAX - 1;
}

bool enqueue(Queue &q, const Order &o) {
    if (isFull(q)) return false;
    q.right++;
    q.orders[q.right] = o;
    return true;
}

bool dequeue(Queue &q, Order &o) {
    if (isEmpty(q)) return false;
    o = q.orders[0];
    for (int i = 0; i < q.right; i++) {
        q.orders[i] = q.orders[i + 1];
    }
    q.right--;
    return true;
}
