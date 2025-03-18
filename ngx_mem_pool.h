#pragma once
//类型重定义
#include<stdlib.h>
#include<memory.h>
using u_char = unsigned char;
using ngx_uint_t = unsigned int;
using ngx_pool_cleanup_pt = void (*)(void* data);
//类型前置声明
struct ngx_pool_s;

/*
* 分配小块内存的内存池的头部数据信息。
*/
struct ngx_pool_data_s {
    u_char* last;    //小块内存池可用内存的起始地址
    u_char* end;      //小块内存池可用内存的末尾地址
    ngx_pool_s* next;  //小块内存被串在一条链表上。
    ngx_uint_t  failed;  //记录当前小块内存分配失败的次数。
};
/*
* 大块内存的头部信息。
*/
struct ngx_pool_large_s {
    ngx_pool_large_s* next;   //分配的大块内存也被连在一起。
    void* alloc; //保存分配出去的大块内存的起始地址。
};
/*
* 清理函数（回调函数）的类型。
*/
struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt handler;  //定义了一个函数指针，保存清理操作的回调函数。
    void* data;  //传递给回调函数的参数。
    ngx_pool_cleanup_s* next;  //所有的CleanUp清理操作都被穿在了一个链表上。
};

/*
* ngx内存池的头部信息和管理成员信息。
*/
struct ngx_pool_s {
    ngx_pool_data_s d;  //数据域，首尾指针等
    size_t max;  //小块内存大块内存的分界线。
    ngx_pool_s* current; //指向第一个小块内存池分配的小块分配池。
    ngx_pool_large_s* large;  //大块内存入口地址。
    ngx_pool_cleanup_s* cleanup;  //预置清理操作回调函数的入口地址。
};
//把指针p调整成a的临近的倍数。
#define ngx_align_ptr(p, a) (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

#define ngx_align(d, a)  (((d) + (a - 1)) & ~(a - 1)) //把d调整到临近的a的倍数上

#define NGX_ALIGNMENT   sizeof(unsigned long)
//buf缓冲区清零。
#define ngx_memzero(buf, n) (void) memset(buf, 0, n)

const int ngx_pagesize = 4096; //默认一个页面的大小。4K
const int NGX_MAX_ALLOC_FROM_POOL = ngx_pagesize - 1; //小块内存池可分配的最大空间。

const int  NGX_DEFAULT_POOL_SIZE = 16 * 1024;  //默认池的大小 16K

const int NGX_POOL_ALIGNMENT = 16;   //内存分配按照16字节对齐
//调整到临近的NGX_POOL_ALIGNMENT的倍数。
const int NGX_MIN_POOL_SIZE = ngx_align((sizeof(ngx_pool_s) + 2 * sizeof(ngx_pool_large_s)),NGX_POOL_ALIGNMENT); 

/*
* 移植ngnix内存池代码，用OOP来实现
*/
class ngx_mem_pool
{
public:
    //创建size大小的内存池。如果大于一个页面，则size为一个页面。
    void* ngx_create_pool(size_t size); 
    //考虑内存对齐，从建立的内存池中申请size大小的内存
    void* ngx_palloc(size_t size);
    //不考虑内存对齐，从建立的内存池中申请size大小的内存
    void* ngx_pnalloc(size_t size);
    //调用的是ngx_palloc，但是可以实现初始化0.
    void* ngx_pcalloc(size_t size);
    //释放大块内存。
    void ngx_pfree(void* p);
    //内存池重置。
    void ngx_reset_pool();
    //内存池的销毁函数
    void ngx_destroy_pool();
    /*
    * 为内存池注册一个清理回调函数，当内存池被销毁或需要清理时，
    这个回调函数会被调用，以便释放由该内存池分配的资源。
    */
    ngx_pool_cleanup_s* ngx_pool_cleanup_add(size_t size);
private:
	ngx_pool_s* pool; //指向ngx内存池的指针。
    //小块内存分配
    void* ngx_palloc_small(size_t size, ngx_uint_t align);
    //大块内存分配
    void* ngx_palloc_large(size_t size);
    //分配新的小块内存池
    void* ngx_palloc_block(size_t size);
};