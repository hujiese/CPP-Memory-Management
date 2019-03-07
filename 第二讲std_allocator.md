## std::allocator ##

### 一、malloc()内部原理 ###

#### 1、VC6.0 malloc ####

![](https://i.imgur.com/gQUae6j.png)

从上图可见，VC6中的malloc()函数分配的内存里面除了我们需要申请的内存空间外还有cookie，debug信息和pad，其中cookie是我们不需要的，如果大量调用malloc的话cookie总和会增多，这回造成较大的浪费。

![](https://i.imgur.com/cmEOkpQ.png)

从上面可以看出，VC6.0的allocate()函数只是对malloc的二次封装，并没有做什么很特殊的操作，它是以类型字节长度为单位分配内存的，上图就分配了512个int类型空间。

#### 2、BC5 malloc ####

![](https://i.imgur.com/ob9hMFD.png)

BC5的allocate()函数和VC6.0本质一样。

#### 3、G2.9 malloc ####

![](https://i.imgur.com/e2MZ9ZG.png)

GCC 2.9版本的allocator如上图所示，但是在实际中该部分却没有被包含使用，从下图容器使用的Alloc可以看到，实际的分配器是使用了一个叫alloc的类，该类分配内存是以字节为单位的，而不是以对象为单位。下图右边灰色部分分配的是512字节，而不是512个对象。

![](https://i.imgur.com/FVlBA20.png)

#### 4、\_\_pool_alloc ###

在GCC 4.9版本，2.9版本的allocate不属于正式使用的那个版本，而是变成了__pool_alloc：

![](https://i.imgur.com/K7XxqvN.png)

![](https://i.imgur.com/6MTLUB7.png)

从上面两张图可以对比看出，2.9版本的allocate和4.9版本的__pool_alloc做的事是一样的，只是修改了变量名和一些细小操作而已。

![](https://i.imgur.com/AHgQElz.png)

![](https://i.imgur.com/BVIH5XG.png)

测试的代码如所示：

	#include <iostream>
	#include <vector> 
	#include <ext\pool_allocator.h>
	
	using namespace std;
	
	template<typename Alloc> 
	void cookie_test(Alloc alloc, size_t n)                                                                                
	{
	    typename Alloc::value_type *p1, *p2, *p3;		//需有 typename 
	  	p1 = alloc.allocate(n); 		//allocate() and deallocate() 是 non-static, 需以 object 呼叫之. 
	  	p2 = alloc.allocate(n);   	
	  	p3 = alloc.allocate(n);  
	
	  	cout << "p1= " << p1 << '\t' << "p2= " << p2 << '\t' << "p3= " << p3 << '\n';
		  	
	  	alloc.deallocate(p1,sizeof(typename Alloc::value_type)); 	//需有 typename 
	  	alloc.deallocate(p2,sizeof(typename Alloc::value_type));  	//有些 allocator 對於 2nd argument 的值無所謂  	
	  	alloc.deallocate(p3,sizeof(typename Alloc::value_type)); 	
	}
	
	int main(void)
	{
		cout << sizeof(__gnu_cxx::__pool_alloc<double>) << endl;
		vector<int, __gnu_cxx::__pool_alloc<double> > vecPool;
		cookie_test(__gnu_cxx::__pool_alloc<double>(), 1);
		
		cout << "----------------------" << endl;
		
		cout << sizeof(std::allocator<double>) << endl;
		vector<int, std::allocator<double> > vecPool2;
		cookie_test(std::allocator<double>(), 1);
		
		return 0;
	}

测试环境是Dev C++5.1.1版本，GCC 4.9，测试结果如下：

![](https://i.imgur.com/n7pUFXm.png)

从上面的测试结果可以看出，如果使用了__pool_alloc的话，连续两块内存之间的距离是8，而一个double类型变量的大小也是8个字节，说明这连续几块内存之间是不带cookie的（即使这几块内存在物理上也是不连续的）。如果使用std的allocator，那么相邻两块内存之间距离为18个字节，每块内存带有一个4字节的头和4字节的尾。

### 二、std::alloc ###

#### 1、std:alloc运作模式 ####

![](https://i.imgur.com/lzcpFvY.png)

std::alloc使用一个16个元素的数组来管理内存链表，而我们上一章只是用了一条链表。数组不同的元素管理不同的区块管理，例如#3号元素负责管理32bytes为一小块的链表。

假设现在用户需要32字节的内存，std::allloc先申请一块区间，为32*20*2大小，用一条链表管理，然后让数组的#3元素管理这条链表。接着讲该以32为一个单元的链表的一个单元（32字节）分给用户。为什么是32*20*2？
前面32*20空间是分配给用户的，但是后面的32*20空间是预留的，如果这时用户需要一个64字节的空间，那么剩下的32*20空间将变成64*10，然后将其中64字节分配给用户，而不用再一次地构建链表和申请空间。

但是也有上限。如果该链表组维护的链表最大的一个小块为128byte，但是用户申请内存块超过了128byte，那么std::alloc将调用malloc给用户分配空间，然后该块将带上cookie头和尾。

![](https://i.imgur.com/8MNTpki.png)

在真正的商业级的内存分配器中，一般都会使用嵌入式指针，将每一个小块的前四个字节用作指针连接下一块可用的内存块。

#### 2、std::alloc运行一瞥 ####

![](https://i.imgur.com/lGNyqvP.png)

![](https://i.imgur.com/G4h5VE1.png)

![](https://i.imgur.com/oEh5eUL.png)

![](https://i.imgur.com/gjy2DCM.png)

![](https://i.imgur.com/Ik5j4AB.png)

![](https://i.imgur.com/0EbenSF.png)

![](https://i.imgur.com/KiVVXm0.png)

![](https://i.imgur.com/KzfwDdr.png)

![](https://i.imgur.com/Vb9WrUI.png)

![](https://i.imgur.com/iYdhtkB.png)

![](https://i.imgur.com/NTqyfwF.png)

![](https://i.imgur.com/kGj86gM.png)

![](https://i.imgur.com/2udQslV.png)

![](https://i.imgur.com/jfBEG5f.png)

#### 3、std::alloc源码剖析 ####

侯杰老师的ppt上总结的很好，在看这部分内容时需要结合老师的ppt，为了方便分析，这里结合老师的课程，使用“倒叙”的方式，先介绍中间的几张ppt，然后跳回前面，顺序和原版ppt不一样。

原版ppt的1-3张介绍的是GCC 2.9的std::alloc的第一级分配器，这里先从第二级开始分析，然后再到第一级。

![](https://i.imgur.com/SCvJ2A6.png)

该分配器为__default_alloc_template，一开始默认使用的分配器，在该类中定义了ROUND_UP函数，用来将申请内存数量做16字节对齐。定义了union free_list_link，在后面会介绍它的作用，在上一章中我们构建的一个小的分配器中也定义了该联合体，作用类似，该联合体可以使用struct代替。free_list是一个有16个obj*元素的数组，在前面讲过，GCC 2.9的分配器用一个16字节数组管理16条链表，free_list便是该管理数组。refill和chunk_alloc在后面再介绍。start_free和end_free分别指向该内存池的头和尾。

![](https://i.imgur.com/ofe7YUv.png)

首先看allocate函数，在函数的一开始便定义了:

	obj* volatile *my_free_list;

结合上图右侧的链表图和上上一张图片内容，my_free_list指向的是free_list中16个元素中的任何一个，*my_free_list则取出free_list某元素中的值，该值指向一条分配内存的链表。所以my_free_list要定义为二级指针。

result则保存分配给用户的一块内存的地址。

首先：

    if (n > (size_t)__MAX_BYTES) {
        return(malloc_alloc::allocate(n));
    }

检查用户申请内存块大小，如果大于__MAX_BYTES（128）那么将调用malloc_alloc::allocate()，这便是第一级分配器，这在后面分析。现在假设用户申请内存小于128字节，那么将根据用户申请内存大小分配对应的内存，由于内存池使用free_list链表管理的，每个free_list链表元素管理不同的内存块大小，这在前面介绍过了。于是有：

	my_free_list = free_list + FREELIST_INDEX(n);

定位到该内存块的位置，这时my_free_list指向的是管理该内存块的空间的地址，使用*my_free_list便可以取到该内存块的地址：

	result = *my_free_list;

然后判断result是否为空：

    if (result == 0) {
        void* r = refill(ROUND_UP(n));
        return r;
    }

如果为空，说明系统内存不够用了，将使用refill()函数分配内存，这部分在后面会介绍。

如果情况正常，那么将该链表中下一个可以使用的空间设置为当前分配给用户空间指向的下一个、在逻辑上连续的空间，最后将result返回给用户：

    *my_free_list = result->free_list_link;
    return (result);

下面的这张图很形象地演示了内存分配的过程：

![](https://i.imgur.com/zXMf35J.png)

接下来分析释放内存。

	  static void deallocate(void *p, size_t n)  //p may not be 0
	  {
	    obj* q = (obj*)p;
	    obj* volatile *my_free_list;   //obj** my_free_list;
	
	    if (n > (size_t) __MAX_BYTES) {
	        malloc_alloc::deallocate(p, n);
	        return;
	    }
	    my_free_list = free_list + FREELIST_INDEX(n);
	    q->free_list_link = *my_free_list;
	    *my_free_list = q;
	  }

释放内存的代码也不难理解，找到需要释放内存的那块空间的地址，然后将当前可分配给用户的空间地址设置为需要释放的该内存空间，一开始指向的可分配的内存空间地址赋值给需要释放空间地址的逻辑连续的下一个内存地址。感觉十分拗口，图和代码更能体现这一过程：

![](https://i.imgur.com/ubYKWxM.png)

上面说到，不论是分配内存还是释放内存，则有：

    if (n > (size_t)__MAX_BYTES) {
        return(malloc_alloc::allocate(n));
    }

和：

    if (n > (size_t) __MAX_BYTES) {
        malloc_alloc::deallocate(p, n);
        return;
    }

也就是将内存分配与释放操作放到第一级allocator中：

![](https://i.imgur.com/Mf5qVqE.png)

从上图中可以看到，第一级分配器叫做：

	class __malloc_alloc_template

其实有：

	typedef __malloc_alloc_template<0>  malloc_alloc;

这在后面会介绍。

分配器的allocate函数如下：

	  static void* allocate(size_t n)
	  {
	    void *result = malloc(n);   //直接使用 malloc()
	    if (0 == result) result = oom_malloc(n);
	    return result;
	  }

直接调用malloc函数分配内存，如果分配失败则调用oom_malloc函数。

同样地，reallocate也是如此：

	  static void* reallocate(void *p, size_t /* old_sz */, size_t new_sz)
	  {
	    void * result = realloc(p, new_sz); //直接使用 realloc()
	    if (0 == result) result = oom_realloc(p, new_sz);
	    return result;
	  }

如果重新要求内存失败，则调用oom_realloc函数，这两个函数在后续会介绍。

deallocate操作则直接释放内存：

	static void deallocate(void *p, size_t /* n */)
	{
		free(p);                    //直接使用 free()
	}

set_malloc_handler是个函数指针，里面传入一个void (*f)()类型函数：

	  static void (*set_malloc_handler(void (*f)()))()
	  { //類似 C++ 的 set_new_handler().
	    void (*old)() = __malloc_alloc_oom_handler;
	    __malloc_alloc_oom_handler = f;
	    return(old);
	  }

该函数设置的是内存分配不够情况下的错误处理函数，这个需要交给用户来管理，首先保存先前的处理函数，然后再将新的处理函数f赋值给__malloc_alloc_oom_handler，然后返回旧的错误处理函数，这也在下一张图片中会介绍：

![](https://i.imgur.com/tWjkErU.png)

可以看到oom_malloc函数内部做的事：

	template <int inst>
	void* __malloc_alloc_template<inst>::oom_malloc(size_t n)
	{
	  void (*my_malloc_handler)();
	  void* result;
	
	  for (;;) {    //不斷嘗試釋放、配置、再釋放、再配置…
	    my_malloc_handler = __malloc_alloc_oom_handler;
	    if (0 == my_malloc_handler) { __THROW_BAD_ALLOC; }
	    (*my_malloc_handler)();    //呼叫處理常式，企圖釋放記憶體
	    result = malloc(n);        //再次嘗試配置記憶體
	    if (result) return(result);
	  }
	}

该函数不断调用__malloc_alloc_oom_handler和malloc函数，直到内存分配成功才返回。oom_realloc也是如此：

	template <int inst>
	void * __malloc_alloc_template<inst>::oom_realloc(void *p, size_t n)
	{
	  void (*my_malloc_handler)();
	  void* result;
	
	  for (;;) {    //不斷嘗試釋放、配置、再釋放、再配置…
	    my_malloc_handler = __malloc_alloc_oom_handler;
	    if (0 == my_malloc_handler) { __THROW_BAD_ALLOC; }
	    (*my_malloc_handler)();    //呼叫處理常式，企圖釋放記憶體。
	    result = realloc(p, n);    //再次嘗試配置記憶體。
	    if (result) return(result);
	  }
	}

![](https://i.imgur.com/hK3r07F.png)

到这里，分配器只剩下refill函数没有分析了，下面将重点讨论该函数。不过在讨论refill函数之前有必要分析chunk_alloc函数：

![](https://i.imgur.com/ICXnj4c.png)

![](https://i.imgur.com/p9EfgAj.png)

该函数声明如下：

	template <bool threads, int inst>
	char*
	__default_alloc_template<threads, inst>::
	chunk_alloc(size_t size, int& nobjs)

函数一开始计算了一些需要的值：

	char* result;
	size_t total_bytes = size * nobjs;
	size_t bytes_left = end_free - start_free;

result指向分配给用户的内存，total_bytes为需要分配的内存块的大小，bytes_left则是当前内存池中剩余的空间大小。

然后：

	if (bytes_left >= total_bytes) {
	  result = start_free;
	  start_free += total_bytes;
	  return(result);
	}

判断如果内存池剩余的内存大小多余需要分配的内存块大小，那么将内存池的首地址start_free直接赋值给result，然后将start_free指针下移total_bytes距离，将当下的result~start_free之间的空间返回给用户。

当然，如果bytes_left比total_bytes小，但是却比size大：

	else if (bytes_left >= size) {
	      nobjs = bytes_left / size;
	      total_bytes = size * nobjs;
	      result = start_free;
	      start_free += total_bytes;
	      return(result);
	  }

这意味着不能直接分配size * nobjs大小内存给用户，那么可以先看看内存池当下的空间能分配多少个size大小的块给用户，然后将该块分配给用户，start_free指针移动total_bytes长度。


	  size_t bytes_to_get =
	             2 * total_bytes + ROUND_UP(heap_size >> 4);
	  // Try to make use of the left-over piece.
	  if (bytes_left > 0) {
	      obj* volatile *my_free_list =
	             free_list + FREELIST_INDEX(bytes_left);
	
	      ((obj*)start_free)->free_list_link = *my_free_list;
	      *my_free_list = (obj*)start_free;
	  }

这部分查看内存池里面还有没有多余的内存，如果有，就充分利用。然后就是不断地获取内存块，将这些内存块不断切割用链表连接起来，递归这些过程：

      start_free = (char*)malloc(bytes_to_get);
      if (0 == start_free) {
          int i;
          obj* volatile *my_free_list, *p;

          //Try to make do with what we have. That can't
          //hurt. We do not try smaller requests, since that tends
          //to result in disaster on multi-process machines.
          for (i = size; i <= __MAX_BYTES; i += __ALIGN) {
              my_free_list = free_list + FREELIST_INDEX(i);
              p = *my_free_list;
              if (0 != p) {
                  *my_free_list = p -> free_list_link;
                  start_free = (char*)p;
                  end_free = start_free + i;
                  return(chunk_alloc(size, nobjs));
                  //Any leftover piece will eventually make it to the
                  //right free list.
              }
          }
          end_free = 0;       //In case of exception.
          start_free = (char*)malloc_alloc::allocate(bytes_to_get);
          //This should either throw an exception or
          //remedy the situation. Thus we assume it
          //succeeded.
      }
      heap_size += bytes_to_get;
      end_free = start_free + bytes_to_get;
      return(chunk_alloc(size, nobjs));

![](https://i.imgur.com/j26x3xi.png)

![](https://i.imgur.com/t4Gz1D7.png)

![](https://i.imgur.com/QhuRqGz.png)

![](https://i.imgur.com/eL1hcds.png)

![](https://i.imgur.com/cUVMnHp.png)

![](https://i.imgur.com/VcXK94y.png)

![](https://i.imgur.com/zvx7Zmx.png)