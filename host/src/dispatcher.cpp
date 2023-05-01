#include "dispatcher.h"
#include "utils.h"
#include <cstring>
#include <stdexcept>
#include <iostream>

Dispatcher::Dispatcher(int n_, const int64_t *parent_) {
    int64_t timecost = 0;
    timecost += timedRun([&] {
        n = n_;
        outDegree = new int[n];
        memset(outDegree, 0, sizeof(int) * n);
        parent = parent_;
        pending = new int[n];
        pendingBegin = new int[n];
        pendingEnd = new int[n];
        hasChildInTaskQueue = new bool[n];
        fillDegree();
        queueInit();
    });
    std::cerr << "dispatcher init time: " << timecost << std::endl;

}

void Dispatcher::fillDegree() {
    for (int i = 0; i < n; i++) {
        outDegree[parent[i]]++;
    }
    pendingBegin[0] = pendingEnd[0] = 0;
    for (int i = 1; i < n; i++) {
        pendingBegin[i] = pendingEnd[i] = pendingBegin[i - 1] + outDegree[i - 1];
    }
}

void Dispatcher::queueInit() {
    memset(hasChildInTaskQueue, 0, sizeof(bool) * n);
    for (int i = 0; i < n; i++) {
        if (outDegree[i] == 0) {
            if (parent[i] == -1 || !hasChildInTaskQueue[parent[i]]) {
                pushToTaskQueue(i);
            } else {
                pushPendingQueue(parent[i], i);
            }
        }
    }
}

void Dispatcher::pushPendingQueue(int64_t par, int desc) {
    pending[pendingEnd[par]++] = desc;
}

int Dispatcher::dispatch(int *res, int length) {
    int count = 0;
    for (; count < length; count++) {
        if (taskQueue.empty()) break;
        int elem = taskQueue.front();
        res[count] = elem;
        taskQueue.pop_front();
    }
    for (int k = 0; k < count; k++) {
        int elem = res[k];

        int par = (int) parent[elem];

        if (par == -1) continue;
        outDegree[par]--;
        if (outDegree[par] == 0) {
            int ppar = (int) parent[par];
            if (ppar == -1 || !hasChildInTaskQueue[ppar]) {
                pushToTaskQueue(par);
            } else {
                pushPendingQueue(ppar, par);
            }
        } else {
            if (getPendingCount(par) == 0) {
                hasChildInTaskQueue[par] = false;
            } else {
                pushToTaskQueue(getOnePending(par));
            }
        }

    }
    return count;
}

void Dispatcher::pushToTaskQueue(int node) {
    taskQueue.push_front(node);
    if (parent[node] != -1)
        hasChildInTaskQueue[parent[node]] = true;
}

int Dispatcher::getPendingCount(int node) {
    return pendingEnd[node] - pendingBegin[node];
}

int Dispatcher::getOnePending(int node) {
    if (getPendingCount(node) <= 0)
        throw std::runtime_error("get empty pending!");
    return pending[pendingBegin[node]++];
}
