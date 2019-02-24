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

![](https://i.imgur.com/Mf5qVqE.png)

![](https://i.imgur.com/tWjkErU.png)

![](https://i.imgur.com/hK3r07F.png)

![](https://i.imgur.com/SCvJ2A6.png)

![](https://i.imgur.com/ofe7YUv.png)

![](https://i.imgur.com/zXMf35J.png)

![](https://i.imgur.com/ubYKWxM.png)

![](https://i.imgur.com/ICXnj4c.png)

![](https://i.imgur.com/p9EfgAj.png)

![](https://i.imgur.com/j26x3xi.png)

![](https://i.imgur.com/t4Gz1D7.png)

![](https://i.imgur.com/QhuRqGz.png)

![](https://i.imgur.com/eL1hcds.png)

![](https://i.imgur.com/cUVMnHp.png)

![](https://i.imgur.com/VcXK94y.png)

![](https://i.imgur.com/zvx7Zmx.png)