#include <thread>
#include <pthread.h>
#include <sched.h>
#include <readerwriterqueue.h>
#include <akq.c>

#define N 10000000
struct work {
    int id;
};

void do_it()
{
    sched_yield();
}

void rwq_do_work(moodycamel::ReaderWriterQueue<work>& q)
{
    work w;
    while (q.try_dequeue(w)) {
        do_it();
        if (w.id == N) return;
    }
}

void brw_do_work(moodycamel::BlockingReaderWriterQueue<work>& q)
{
    work w;
    while (1) {
        q.wait_dequeue(w);
        do_it();
        if (w.id == N) return;
    }
}

template<typename Q>
void do_enqueue(Q& q)
{
    for (int i = 1; i <= N; ++i) {
        work w;
        w.id = i;
        q.enqueue(w);
    }
}

void rwq()
{
    moodycamel::ReaderWriterQueue<work> q;
    std::thread t(rwq_do_work, std::ref(q));
    do_enqueue(q);
    t.join();
}

void brw()
{
    moodycamel::BlockingReaderWriterQueue<work> q;
    std::thread t(brw_do_work, std::ref(q));
    do_enqueue(q);
    t.join();
}

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
void akq_do_work(void *arg)
{
    work *w = static_cast<work *>(arg);
    do_it();
    if (w->id == N) {
        pthread_mutex_lock(&lock);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }
}

void AKQ()
{
    akq *q = akq_new(sizeof(work), akq_do_work, nullptr, nullptr);
    work *w;
    for (int i = 1; i < N - 1; ++i) {
        w = static_cast<work *>(akq_work_new(q));
        w->id = i;
        akq_enqueue(q, w);
    }
    pthread_mutex_lock(&lock);
    w = static_cast<work *>(akq_work_new(q));
    w->id = N;
    akq_enqueue(q, w);
    pthread_cond_wait(&cond, &lock);
    pthread_mutex_unlock(&lock);
    akq_stop(q);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
usage:  fprintf(stderr, "usage: test a|b|c\n");
        return EXIT_FAILURE;
    }
    switch (argv[1][0]) {
    case 'a': AKQ(); break;
    case 'b': brw(); break;
    case 'c':
    case 'r': rwq(); break;
    default: goto usage; 
    }
    return EXIT_SUCCESS;
}
