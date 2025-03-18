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
    //Ҳ����ʵ���ڹ��캯����
    if (nullptr == mempool.ngx_create_pool(512))
    {
        printf("ngx_create_pool fail...");
        return -1;
    }

    void* p1 = mempool.ngx_palloc(128); // ��С���ڴ�ط���� ��ΪС��max
    if (p1 == nullptr)
    {
        printf("ngx_palloc 128 bytes fail...");
        return -1;
    }

    Data* p2 = (Data*)mempool.ngx_palloc(512); // �Ӵ���ڴ�ط����  
    if (p2 == nullptr)
    {
        printf("ngx_palloc 512 bytes fail...");
        return -1;
    }

    p2->ptr = (char *)malloc(12); //������4B
    strcpy(p2->ptr, "hello world");
    p2->pfile = fopen("data.txt", "w");

    ngx_pool_cleanup_s* c1 = mempool.ngx_pool_cleanup_add(sizeof(char*)); //�ڶ�������������Ҳ��4B
    c1->handler = func1;
    c1->data = p2->ptr;

    ngx_pool_cleanup_s* c2 = mempool.ngx_pool_cleanup_add(sizeof(FILE*));
    c2->handler = func2;
    c2->data = p2->pfile;

    //Ҳ����ʵ��������������
    mempool.ngx_destroy_pool(); // 1.�������е�Ԥ�õ������� 2.�ͷŴ���ڴ� 3.�ͷ�С���ڴ�������ڴ�

    return 0;
}
