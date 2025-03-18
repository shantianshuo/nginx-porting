#include "ngx_mem_pool.h"
#include<cstdlib> 

//创建size大小的内存池。如果大于一个页面，则size为一个页面。
void* ngx_mem_pool::ngx_create_pool(size_t size)
{
    ngx_pool_s* p;

    //根据用户给出的大小，开辟内存池。
    p = (ngx_pool_s*)malloc(size);
    if (p == nullptr) 
    {
        return nullptr;
    }
    //p指针++的时候，增加一个字节。
    p->d.last = (u_char*)p + sizeof(ngx_pool_s); //指向可使用的末尾
    p->d.end = (u_char*)p + size; //指向内存池末尾
    p->d.next = nullptr;
    p->d.failed = 0;

    size = size - sizeof(ngx_pool_s); //总大小减去头信息就是可用空间。
    p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL; //max存储当前小块内存存储的最大值

    p->current = p;	//内存的起始地址
    p->large = nullptr;
    p->cleanup = nullptr;

    pool = p;
    return p;
}

void* ngx_mem_pool::ngx_palloc(size_t size)
{

    if (size <= pool->max) {
        return ngx_palloc_small(size, 1); 
    }

    return ngx_palloc_large(size);
}

void* ngx_mem_pool::ngx_pnalloc(size_t size)
{
    if (size <= pool->max) {
        return ngx_palloc_small(size, 0);
    }
    return ngx_palloc_large(size);
}
//调用的是ngx_palloc，但是可以实现初始化0.
void* ngx_mem_pool::ngx_pcalloc(size_t size)
{
  
    void* p;
    p = ngx_palloc(size);
    if (p) {
        ngx_memzero(p, size);
    }
    return p;   
}
//小块内存分配
void* ngx_mem_pool::ngx_palloc_small(size_t size, ngx_uint_t align)
{
    u_char* m;
    ngx_pool_s* p;

    p = pool->current;

    do {
        m = p->d.last;

        if (align) {
            m = ngx_align_ptr(m, NGX_ALIGNMENT); //把m调整成，相关平台4B或者8B的整数倍。
        }

        if ((size_t)(p->d.end - m) >= size) {//内存池空间>=申请的内存空间。
            p->d.last = m + size;

            return m;
        }

        p = p->d.next; //只有一个块，next初始是空。直接跳出循环。

    } while (p);

    // 如果所有内存池块都没有足够的空间，调用 ngx_palloc_block 函数创建新的内存池块并分配内存
    return ngx_palloc_block(size);
}
//大块内存分配
void* ngx_mem_pool::ngx_palloc_large(size_t size)
{
    void* p;
    ngx_uint_t         n;
    ngx_pool_large_s* large;

    p = malloc(size);
    if (p == nullptr) {
        return nullptr;
    }

    n = 0;

    for (large = pool->large; large; large = large->next) {
        if (large->alloc == nullptr) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    large = (ngx_pool_large_s*)ngx_palloc_small(sizeof(ngx_pool_large_s), 1);
    if (large == nullptr) {
        free(p);
        return nullptr;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}
//分配新的小块内存池
void* ngx_mem_pool::ngx_palloc_block(size_t size)
{
    {
        u_char* m;
        size_t       psize;
        ngx_pool_s* p, * newpool;

        psize = (size_t)(pool->d.end - (u_char*)pool);

        m = (u_char*)malloc(psize);
        if (m == nullptr) {
            return nullptr;
        }

        newpool = (ngx_pool_s*)m;

        newpool->d.end = m + psize;
        newpool->d.next = nullptr;
        newpool->d.failed = 0;

        m += sizeof(ngx_pool_data_s);
        m = ngx_align_ptr(m, NGX_ALIGNMENT); //同样是把指针调整成整数倍。
        newpool->d.last = m + size;//分配内存。通过指针偏移。

       // 遍历原有内存池块链表
        for (p = pool->current; p->d.next; p = p->d.next) {
            // 若某个内存池块的分配失败次数超过 4 次
            if (p->d.failed++ > 4) {
                // 如果连续申请内存都比较大。每次都失败，将当前可用的内存池块指针更新为该块的下一个块
                pool->current = p->d.next; //因为每次内存分配都是在current指向的块。current指向下一个块，代表以前的快已经使用完毕，不会在分配。
            }
        }

        // 将新内存池块添加到原有内存池块链表的末尾
        p->d.next = newpool;
        // 返回新分配内存的起始地址
        return m;
    }
}

void ngx_mem_pool::ngx_pfree(void* p)
{
    ngx_pool_large_s* l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            free(l->alloc);
            l->alloc = nullptr;
            return;
        }
    }
}

//内存池重置。
void ngx_mem_pool::ngx_reset_pool()
{
    //遍历大小块内存。
    ngx_pool_s* p;
    ngx_pool_large_s* l;

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            free(l->alloc);
        }
    }

    //处理第一块内存
    p = pool;
    p->d.last = (u_char*)p + sizeof(ngx_pool_s);
    p->d.failed = 0;

    //处理第二块内存直到末尾。
    for (p = p->d.next; p; p = p->d.next)
    {
        p->d.last = (u_char*)p + sizeof(ngx_pool_data_s);
        p->d.failed = 0;
    }
    pool->current = pool;
    pool->large = nullptr;
}
//内存池的销毁函数
void ngx_mem_pool::ngx_destroy_pool()
{
    ngx_pool_s* p, * n;
    ngx_pool_large_s* l;
    ngx_pool_cleanup_s* c;
    for (c = pool->cleanup; c; c = c->next) {     //预先释放大块内存中占用的资源。
        if (c->handler) {
            c->handler(c->data);
        }
    }
    for (l = pool->large; l; l = l->next) {    //释放大块内存
        if (l->alloc) {
            free(l->alloc);
        }
    }
    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        free(p);

        if (n == nullptr) {
            break;
        }
    }
}

ngx_pool_cleanup_s* ngx_mem_pool::ngx_pool_cleanup_add(size_t size)
{
    ngx_pool_cleanup_s* c;

    c = (ngx_pool_cleanup_s*)ngx_palloc(sizeof(ngx_pool_cleanup_s)); //在小块内存中开辟的。
    if (c == nullptr) {
        return nullptr;
    }

    if (size) {
        c->data = ngx_palloc(size);//在小块内存中开辟的。
        if (c->data == nullptr) {
            return nullptr;
        }

    }
    else {
        c->data = nullptr;
    }

    c->handler = nullptr;
    c->next = pool->cleanup; //这两句相当于链表的头插法。

    pool->cleanup = c; // 当前的结构体，被链接在第一个内存池块的cleanup上。
    return c;
}