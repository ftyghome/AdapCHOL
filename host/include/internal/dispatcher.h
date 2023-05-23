#pragma once

#include <cstdint>
#include <queue>

class Dispatcher {
private:
    int n;
    int *outDegree;
    const int64_t *parent;
    int *pending;
    int *pendingBegin, *pendingEnd;
    bool *hasChildInTaskQueue;
    std::deque<int> taskQueue;

    void fillDegree();

    void queueInit();

    void pushPendingQueue(int64_t par, int desc);



    void pushToTaskQueue(int node);

    int getPendingCount(int node);

    int getOnePending(int node);

public:
    Dispatcher(int n_, const int64_t *parent_);

    int dispatch(int *res, int length);
};