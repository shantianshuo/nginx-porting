#pragma once
//�����ض���
#include<stdlib.h>
#include<memory.h>
using u_char = unsigned char;
using ngx_uint_t = unsigned int;
using ngx_pool_cleanup_pt = void (*)(void* data);
//����ǰ������
struct ngx_pool_s;

/*
* ����С���ڴ���ڴ�ص�ͷ��������Ϣ��
*/
struct ngx_pool_data_s {
    u_char* last;    //С���ڴ�ؿ����ڴ����ʼ��ַ
    u_char* end;      //С���ڴ�ؿ����ڴ��ĩβ��ַ
    ngx_pool_s* next;  //С���ڴ汻����һ�������ϡ�
    ngx_uint_t  failed;  //��¼��ǰС���ڴ����ʧ�ܵĴ�����
};
/*
* ����ڴ��ͷ����Ϣ��
*/
struct ngx_pool_large_s {
    ngx_pool_large_s* next;   //����Ĵ���ڴ�Ҳ������һ��
    void* alloc; //��������ȥ�Ĵ���ڴ����ʼ��ַ��
};
/*
* ���������ص������������͡�
*/
struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt handler;  //������һ������ָ�룬������������Ļص�������
    void* data;  //���ݸ��ص������Ĳ�����
    ngx_pool_cleanup_s* next;  //���е�CleanUp�����������������һ�������ϡ�
};

/*
* ngx�ڴ�ص�ͷ����Ϣ�͹����Ա��Ϣ��
*/
struct ngx_pool_s {
    ngx_pool_data_s d;  //��������βָ���
    size_t max;  //С���ڴ����ڴ�ķֽ��ߡ�
    ngx_pool_s* current; //ָ���һ��С���ڴ�ط����С�����ء�
    ngx_pool_large_s* large;  //����ڴ���ڵ�ַ��
    ngx_pool_cleanup_s* cleanup;  //Ԥ����������ص���������ڵ�ַ��
};
//��ָ��p������a���ٽ��ı�����
#define ngx_align_ptr(p, a) (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

#define ngx_align(d, a)  (((d) + (a - 1)) & ~(a - 1)) //��d�������ٽ���a�ı�����

#define NGX_ALIGNMENT   sizeof(unsigned long)
//buf���������㡣
#define ngx_memzero(buf, n) (void) memset(buf, 0, n)

const int ngx_pagesize = 4096; //Ĭ��һ��ҳ��Ĵ�С��4K
const int NGX_MAX_ALLOC_FROM_POOL = ngx_pagesize - 1; //С���ڴ�ؿɷ�������ռ䡣

const int  NGX_DEFAULT_POOL_SIZE = 16 * 1024;  //Ĭ�ϳصĴ�С 16K

const int NGX_POOL_ALIGNMENT = 16;   //�ڴ���䰴��16�ֽڶ���
//�������ٽ���NGX_POOL_ALIGNMENT�ı�����
const int NGX_MIN_POOL_SIZE = ngx_align((sizeof(ngx_pool_s) + 2 * sizeof(ngx_pool_large_s)),NGX_POOL_ALIGNMENT); 

/*
* ��ֲngnix�ڴ�ش��룬��OOP��ʵ��
*/
class ngx_mem_pool
{
public:
    //����size��С���ڴ�ء��������һ��ҳ�棬��sizeΪһ��ҳ�档
    void* ngx_create_pool(size_t size); 
    //�����ڴ���룬�ӽ������ڴ��������size��С���ڴ�
    void* ngx_palloc(size_t size);
    //�������ڴ���룬�ӽ������ڴ��������size��С���ڴ�
    void* ngx_pnalloc(size_t size);
    //���õ���ngx_palloc�����ǿ���ʵ�ֳ�ʼ��0.
    void* ngx_pcalloc(size_t size);
    //�ͷŴ���ڴ档
    void ngx_pfree(void* p);
    //�ڴ�����á�
    void ngx_reset_pool();
    //�ڴ�ص����ٺ���
    void ngx_destroy_pool();
    /*
    * Ϊ�ڴ��ע��һ������ص����������ڴ�ر����ٻ���Ҫ����ʱ��
    ����ص������ᱻ���ã��Ա��ͷ��ɸ��ڴ�ط������Դ��
    */
    ngx_pool_cleanup_s* ngx_pool_cleanup_add(size_t size);
private:
	ngx_pool_s* pool; //ָ��ngx�ڴ�ص�ָ�롣
    //С���ڴ����
    void* ngx_palloc_small(size_t size, ngx_uint_t align);
    //����ڴ����
    void* ngx_palloc_large(size_t size);
    //�����µ�С���ڴ��
    void* ngx_palloc_block(size_t size);
};