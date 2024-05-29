#include "common.h"

// メモリブロックの構造体
typedef struct MemoryBlock {
    struct MemoryBlock* next;
} MemoryBlock;

// メモリプールの構造体
typedef struct MemoryPool {
    size_t block_size;
    size_t pool_size;
    size_t used_blocks;
    MemoryBlock* free_list;
    void* pool_memory;
    pthread_mutex_t mutex;
} MemoryPool;

// メモリプールの初期化
int initialize_pool(MemoryPool* pool, size_t block_size, size_t pool_size) {
    pool->block_size = block_size;
    pool->pool_size = pool_size;
    pool->used_blocks = 0;
    pool->pool_memory = malloc(block_size * pool_size);
    if (pool->pool_memory == NULL) {
        return RET_NG;  // メモリ確保失敗
    }
    pool->free_list = NULL;
    pthread_mutex_init(&pool->mutex, NULL);

    // フリーリストを初期化
    for (size_t i = 0; i < pool_size; ++i) {
        MemoryBlock* block = (MemoryBlock*)((char*)pool->pool_memory + i * block_size);
        block->next = pool->free_list;
        pool->free_list = block;
    }

    return RET_OK;
}

// メモリプールの拡張
int expand_pool(MemoryPool* pool, size_t additional_blocks) {
    void *new_memory = realloc(pool->pool_memory, pool->block_size * (pool->pool_size + additional_blocks));
    if (new_memory == NULL) {
        return RET_NG; // メモリ再確保失敗
    }

    pool->pool_memory = new_memory;
    for (size_t i = pool->pool_size; i < pool->pool_size + additional_blocks; ++i) {
        MemoryBlock* block = (MemoryBlock*)((char*)pool->pool_memory + i * pool->block_size);
        block->next = pool->free_list;
        pool->free_list = block;
    }
    pool->pool_size += additional_blocks;
    return RET_OK;
}

// メモリブロックの割り当て
void* allocate_block(MemoryPool* pool) {
    pthread_mutex_lock(&pool->mutex);
    if (pool->free_list == NULL) {
        if (expand_pool(pool, pool->pool_size) != RET_OK) {
            pthread_mutex_unlock(&pool->mutex);
            return NULL;
        }
    }

    MemoryBlock* block = pool->free_list;
    pool->free_list = block->next;
    pool->used_blocks++;
    pthread_mutex_unlock(&pool->mutex);
    return block;
}

// メモリブロックの開放
void free_block(MemoryPool* pool, void* block) {
    pthread_mutex_lock(&pool->mutex);
    ((MemoryBlock*)block)->next = pool->free_list;
    pool->free_list = (MemoryBlock*)block;
    pool->used_blocks--;
    pthread_mutex_unlock(&pool->mutex);
}

// メモリプールのクリーンアップ
void cleanup_pool(MemoryPool* pool) {
    pthread_mutex_destroy(&pool->mutex);
    free(pool->pool_memory);
    pool->pool_memory = NULL;
    pool->free_list = NULL;
}

int main() {
    MemoryPool pool;
    if (initialize_pool(&pool, sizeof(ChatMessage), 10) != RET_OK) {
        fprintf(stderr, "Failed to initialize memory pool\n");
        return EXIT_FAILURE;
    }

     // ループでメモリプールをテストする
    for (int i = 0; i < 100; ++i) {
        ChatMessage* messages[5];

        // 5つのメモリブロックを割り当てる
        for (int j = 0; j < 5; ++j) {
            messages[j] = (ChatMessage*)allocate_block(&pool);
            if (messages[j]) {
                snprintf(messages[j]->text, MAX_MESSAGE_LENGTH, "Hello, this is message %d in loop %d", j + 1, i + 1);
            }
        }

        // 割り当てたメモリブロックの内容を出力する
        for (int j = 0; j < 5; ++j) {
            if (messages[j]) {
                printf("Loop %d, Allocated message: %s\n", i + 1, messages[j]->text);
            }
        }

        // メモリブロックを解放する
        for (int j = 0; j < 5; ++j) {
            if (messages[j]) {
                free_block(&pool, messages[j]);
            }
        }
    }
}