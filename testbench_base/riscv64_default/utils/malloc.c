#include "malloc.h"

//内存池(32字节对齐)
uint8_t mem1base[MEM1_MAX_SIZE] __attribute__((section("._user_heap")));                                                 //内部SRAM内存池
//内存管理表
uint32_t mem1mapbase[MEM1_ALLOC_TABLE_SIZE];                                                 //内部SRAM内存池MAP
//内存管理参数
const uint32_t memtblsize[SRAMBANK] = {MEM1_ALLOC_TABLE_SIZE}; //内存表大小
const uint32_t memblksize[SRAMBANK] = {MEM1_BLOCK_SIZE};                 //内存分块大小
const uint32_t memsize[SRAMBANK] = {MEM1_MAX_SIZE};                          //内存总大小

//内存管理控制器
struct _m_mallco_dev mallco_dev = {
  my_mem_init,                        //内存初始化
  my_mem_perused,                     //内存使用率
  {mem1base},       //内存池
  {mem1mapbase}, //内存管理状态表
  {0},                          //内存管理未就绪
};

//复制内存
//*des:目的地址
//*src:源地址
//n:需要复制的内存长度(字节为单位)
void mymemcpy(void *des, void *src, uint32_t n) {
  uint8_t *xdes = des;
  uint8_t *xsrc = src;
  while (n--)*xdes++ = *xsrc++;
}

void* my_memmove(void* dest, const void* src, uint32_t count) {
    char* ret = (char*)dest;

    if (dest < src) {
        //处理开始部分的不对齐字节
        while (((uintptr_t)dest % 8 != 0) && count--) {
            *(char*)dest = *(char*)src;
            dest = (char*)dest + 1;
            src = (char*)src + 1;
        }

        // 使用64位（8字节）移动
        uint64_t* l_dest = (uint64_t*)dest;
        uint64_t* l_src = (uint64_t*)src;
        while (count >= 8) {
            *l_dest = *l_src;
            l_dest++;
            l_src++;
            count -= 8;
        }

        //处理结束部分的不对齐字节
        dest = (char*)l_dest;
        src = (char*)l_src;
        while (count--) {
            *(char*)dest = *(char*)src;
            dest = (char*)dest + 1;
            src = (char*)src + 1;
        }
    } else {
        dest = (char*)dest + count - 1;
        src = (char*)src + count - 1;
        
        //处理开始部分的不对齐字节
        while (((uintptr_t)dest % 8 != 0) && count--) {
            *(char*)dest = *(char*)src;
            dest = (char*)dest - 1;
            src = (char*)src - 1;
        }

        // 使用64位（8字节）移动
        uint64_t* l_dest = (uint64_t*)dest;
        uint64_t* l_src = (uint64_t*)src;
        while (count >= 8) {
            *l_dest = *l_src;
            l_dest--;
            l_src--;
            count -= 8;
        }

        //处理结束部分的不对齐字节
        dest = (char*)l_dest;
        src = (char*)l_src;
        while (count--) {
            *(char*)dest = *(char*)src;
            dest = (char*)dest - 1;
            src = (char*)src - 1;
        }
    }
    
    return ret;
}


//设置内存
//*s:内存首地址
//c :要设置的值
//count:需要设置的内存大小(字节为单位)
void mymemset(void *s, uint8_t c, uint32_t count) {
    uint8_t *byte_ptr = s;
    uint64_t *qword_ptr;
    uint64_t pattern = c;
    
    pattern = (pattern << 8)  | c;
    pattern = (pattern << 16) | pattern;
    pattern = (pattern << 32) | pattern;  // 创建一个64位的填充模式

    // 对齐到64位边界
    while (((uintptr_t)byte_ptr % 8 != 0) && count) {
        *byte_ptr++ = c;
        count--;
    }

    qword_ptr = (uint64_t *)byte_ptr;

    // 使用64位块进行填充，并进行循环展开
    while (count >= 32) {
        qword_ptr[0] = pattern;
        qword_ptr[1] = pattern;
        qword_ptr[2] = pattern;
        qword_ptr[3] = pattern;
        qword_ptr += 4;
        count -= 32;
    }

    // 使用64位块处理剩余的部分
    while (count >= 8) {
        *qword_ptr++ = pattern;
        count -= 8;
    }

    byte_ptr = (uint8_t *)qword_ptr;

    // 处理剩余的不是64位块的部分
    while (count--) {
        *byte_ptr++ = c;
    }
}

//内存管理初始化
//memx:所属内存块
void my_mem_init(uint8_t memx) {
  mymemset(mallco_dev.memmap[memx], 0, memtblsize[memx] * 4); //内存状态表数据清零
  mallco_dev.memrdy[memx] = 1;                            //内存管理初始化OK
}
//获取内存使用率
//memx:所属内存块
//返回值:使用率(扩大了10倍,0~1000,代表0.0%~100.0%)
uint16_t my_mem_perused(uint8_t memx) {
  uint32_t used = 0;
  uint32_t i;
  for (i = 0; i < memtblsize[memx]; i++) {
    if (mallco_dev.memmap[memx][i])used++;
  }
  return (used * 1000) / (memtblsize[memx]);
}
//内存分配(内部调用)
//memx:所属内存块
//size:要分配的内存大小(字节)
//返回值:0XFFFFFFFF,代表错误;其他,内存偏移地址
uint32_t my_mem_malloc(uint8_t memx, uint32_t size) {
  signed long offset = 0;
  uint32_t nmemb;  //需要的内存块数
  uint32_t cmemb = 0; //连续空内存块数
  uint32_t i;
  if (!mallco_dev.memrdy[memx])mallco_dev.init(memx); //未初始化,先执行初始化
  if (size == 0)return 0XFFFFFFFF; //不需要分配
  nmemb = size / memblksize[memx]; //获取需要分配的连续内存块数
  if (size % memblksize[memx])nmemb++;
  for (offset = memtblsize[memx] - 1; offset >= 0; offset--) { //搜索整个内存控制区
    if (!mallco_dev.memmap[memx][offset])cmemb++; //连续空内存块数增加
    else cmemb = 0;                             //连续内存块清零
    if (cmemb == nmemb) {                       //找到了连续nmemb个空内存块
      for (i = 0; i < nmemb; i++) {           //标注内存块非空
        mallco_dev.memmap[memx][offset + i] = nmemb;
      }
      return (offset * memblksize[memx]); //返回偏移地址
    }
  }
  return 0XFFFFFFFF;//未找到符合分配条件的内存块
}
//释放内存(内部调用)
//memx:所属内存块
//offset:内存地址偏移
//返回值:0,释放成功;1,释放失败;
uint8_t my_mem_free(uint8_t memx, uint32_t offset) {
  int i;
  if (!mallco_dev.memrdy[memx]) { //未初始化,先执行初始化
    mallco_dev.init(memx);
    return 1;//未初始化
  }
  if (offset < memsize[memx]) { //偏移在内存池内.
    int index = offset / memblksize[memx];      //偏移所在内存块号码
    int nmemb = mallco_dev.memmap[memx][index]; //内存块数量
    for (i = 0; i < nmemb; i++) {               //内存块清零
      mallco_dev.memmap[memx][index + i] = 0;
    }
    return 0;
  } else return 2; //偏移超区了.
}
//释放内存(外部调用)
//memx:所属内存块
//ptr:内存首地址
void myfree(uint8_t memx, void *ptr) {
  uint32_t offset;
  if (ptr == NULL)return; //地址为0.
  offset = (intptr_t)ptr - (intptr_t)mallco_dev.membase[memx];
  my_mem_free(memx, offset);  //释放内存
}
//分配内存(外部调用)
//memx:所属内存块
//size:内存大小(字节)
//返回值:分配到的内存首地址.
void *mymalloc(uint8_t memx, uint32_t size) {
  uint32_t offset;
  offset = my_mem_malloc(memx, size);
  if (offset == 0XFFFFFFFF)return NULL;
  else return (void *)((intptr_t)mallco_dev.membase[memx] + offset);
}
//重新分配内存(外部调用)
//memx:所属内存块
//*ptr:旧内存首地址
//size:要分配的内存大小(字节)
//返回值:新分配到的内存首地址.
void *myrealloc(uint8_t memx, void *ptr, uint32_t size) {
  uint32_t offset;
  offset = my_mem_malloc(memx, size);
  if (offset == 0XFFFFFFFF)return NULL;
  else {
    mymemcpy((void *)((intptr_t)mallco_dev.membase[memx] + offset), ptr, size); //拷贝旧内存内容到新内存
    myfree(memx, ptr);                                                  //释放旧内存
    return (void *)((intptr_t)mallco_dev.membase[memx] + offset);            //返回新内存首地址
  }
}

/* Compare S1 and S2, returning less than, equal to or
   greater than zero if S1 is lexicographically less than,
   equal to or greater than S2.  */
int mystrcmp(const char *p1, const char *p2) {
  const unsigned char *s1 = (const unsigned char *) p1;
  const unsigned char *s2 = (const unsigned char *) p2;
  unsigned char c1, c2;
  do {
    c1 = (unsigned char) * s1++;
    c2 = (unsigned char) * s2++;
    if (c1 == '\0')
      return c1 - c2;
  } while (c1 == c2);
  return c1 - c2;
}











