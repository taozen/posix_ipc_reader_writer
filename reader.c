#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

typedef struct {
    char flag;
    int  len;
    char data[1];
} framebuf_t;

typedef struct {
    int shmid;
    char *shm;
    framebuf_t *framebuf;
} mem_context_t;

mem_context_t* mem_context_new()
{
    mem_context_t *ctx = (mem_context_t*)malloc(sizeof(mem_context_t));
    ctx->shmid = -1;
    ctx->shm = NULL;
    ctx->framebuf = NULL;
    return ctx;
}

void mem_context_destroy(mem_context_t **ctx)
{
    if (*ctx) {
        if ((*ctx)->shm != NULL) {
            int rc = shmdt((*ctx)->shm);
            if (rc < 0) {
                fprintf(stderr, "shmdt error: %s\n", strerror(errno));
            }
        }
        free(*ctx);
        *ctx = NULL;
    }
}

int mem_setup(mem_context_t *ctx, key_t key, int size)
{
    int shmid = shmget(key, sizeof(framebuf_t) + size, 0600);

    if (shmid < 0) {
        fprintf(stderr, "shmget error: %s\n", strerror(errno));
        return -1;
    }

    char *shm = (char*)shmat(shmid, NULL, 0);
    if (shm == (char*)-1) {
        fprintf(stderr, "shmat error: %s\n", strerror(errno));
        return -1;
    }

    ctx->shmid = shmid;
    ctx->shm = shm;
    ctx->framebuf = (framebuf_t*)shm;
    return 0;
}

void read_work(framebuf_t *framebuf) 
{
    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = 5000000L; // check flag every 5ms

    do {
        if (framebuf->flag == 0) {
            //printf("buffer not ready\n");
            continue; // not ready for read
        }

        printf("Got a frame, len=%d\n", framebuf->len);
        nanosleep(&tim, &tim2); // simulate some work
        framebuf->flag = 0;
    } while (nanosleep(&tim, &tim2) != -1);
}

int main(int argc, const char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s mem_key\n", argv[0]);
        return 2;
    }

    key_t key = atoi(argv[1]);
    const int MaxFrameSize = 1920*1080;
    mem_context_t *mctx = mem_context_new();

    if (mem_setup(mctx, key, MaxFrameSize) != 0) {
        mem_context_destroy(&mctx);
        return -1;
    }

    read_work(mctx->framebuf);
    mem_context_destroy(&mctx);
    return 0;
}

