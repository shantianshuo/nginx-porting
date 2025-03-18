#include"ngx_mem_pool.h"
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
struct Data
{
    char* ptr;
    FILE* pfile;
};

void func1(void* p)
{
    char* p1 = (char*) p;
    printf("free ptr mem!");
    free(p);
}
void func2(void* pf1)
{
    FILE* pf = (FILE*)pf1;
    printf("close file!");
    fclose(pf);
}
int main()
{
    ngx_mem_pool mempool;
    //也可以实现在构造函数中
    if (nullptr == mempool.ngx_create_pool(512))
    {
        printf("ngx_create_pool fail...");
        return -1;
    }

    void* p1 = mempool.ngx_palloc(128); // 从小块内存池分配的 因为小于max
    if (p1 == nullptr)
    {
        printf("ngx_palloc 128 bytes fail...");
        return -1;
    }

    Data* p2 = (Data*)mempool.ngx_palloc(512); // 从大块内存池分配的  
    if (p2 == nullptr)
    {
        printf("ngx_palloc 512 bytes fail...");
        return -1;
    }

    p2->ptr = (char *)malloc(12); //这里是4B
    strcpy(p2->ptr, "hello world");
    p2->pfile = fopen("data.txt", "w");

    ngx_pool_cleanup_s* c1 = mempool.ngx_pool_cleanup_add(sizeof(char*)); //第二个参数传进的也是4B
    c1->handler = func1;
    c1->data = p2->ptr;

    ngx_pool_cleanup_s* c2 = mempool.ngx_pool_cleanup_add(sizeof(FILE*));
    c2->handler = func2;
    c2->data = p2->pfile;

    //也可以实现在析构函数中
    mempool.ngx_destroy_pool(); // 1.调用所有的预置的清理函数 2.释放大块内存 3.释放小块内存池所有内存

    return 0;
}
