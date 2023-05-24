#pragma once
#include "common.h"
#include <cstdint>
#include <queue>

class Dispatcher {
private:
    int n;
    int *outDegree;
    const int *parent;
    int *pending;
    int *pendingBegin, *pendingEnd;
    bool *hasChildInTaskQueue;
    std::deque<int> taskQueue;

    void fillDegree();

    void queueInit();

    void pushPendingQueue(int par, int desc);



    void pushToTaskQueue(int node);

    int getPendingCount(int node);

    int getOnePending(int node);

public:
    Dispatcher(int n_, const int *parent_);

    int dispatch(int *res, int length);
};