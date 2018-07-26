## C++内存管理 ##

### 一、四种内存分配和释放方法 ###

![](https://i.imgur.com/AREog97.png)

在编程时可以通过上图的几种方法直接或间接地操作内存。下面将介绍四种C++内存操作方法：

![](https://i.imgur.com/8F62Mf8.png)

通常可以使用malloc和new来分配内存，当然也可以使用::operator new()和分配器allocator来操作内存，下面将具体介绍这些函数的使用方法。

![](https://i.imgur.com/lljUGp1.png)

![](https://i.imgur.com/xkUvBlJ.png)

通过malloc和new分配内存、通过free和delete释放内存是十分常用的，通过::operator new操作内存比较少见，allocator分配器操作内存在STL源码中使用较多，对于不同的编译环境使用也有所不同。下面这个例子是基与VS2013环境做测试的:

	#include <iostream>
	#include <complex>
	#include <memory>				 //std::allocator  
	//#include <ext\pool_allocator.h>	 //GCC使用，欲使用 std::allocator 以外的 allocator, 就得自行 #include <ext/...> 
	using namespace std;
	namespace jj01
	{
		void test_primitives()
		{
			cout << "\ntest_primitives().......... \n";
	
			void* p1 = malloc(512);	//512 bytes
			free(p1);
	
			complex<int>* p2 = new complex<int>; //one object
			delete p2;
	
			void* p3 = ::operator new(512); //512 bytes
			::operator delete(p3);
	
			//以下使用 C++ 標準庫提供的 allocators。
			//其接口雖有標準規格，但實現廠商並未完全遵守；下面三者形式略異。
	#ifdef _MSC_VER
			//以下兩函數都是 non-static，定要通過 object 調用。以下分配 3 個 ints.
			int* p4 = allocator<int>().allocate(3, (int*)0);
			p4[0] = 666;
			p4[1] = 999;
			p4[2] = 888;
			cout << "p4[0] = " << p4[0] << endl;
			cout << "p4[1] = " << p4[1] << endl;
			cout << "p4[2] = " << p4[2] << endl;
			allocator<int>().deallocate(p4, 3);
	#endif
	#ifdef __BORLANDC__
			//以下兩函數都是 non-static，定要通過 object 調用。以下分配 5 個 ints.
			int* p4 = allocator<int>().allocate(5);
			allocator<int>().deallocate(p4, 5);
	#endif
	#ifdef __GNUC__
			//以下兩函數都是 static，可通過全名調用之。以下分配 512 bytes.
			//void* p4 = alloc::allocate(512); 
			//alloc::deallocate(p4,512);   
	
			//以下兩函數都是 non-static，定要通過 object 調用。以下分配 7 個 ints.    
			void* p4 = allocator<int>().allocate(7);
			allocator<int>().deallocate((int*)p4, 7);
	
			//以下兩函數都是 non-static，定要通過 object 調用。以下分配 9 個 ints.	
			void* p5 = __gnu_cxx::__pool_alloc<int>().allocate(9);
			__gnu_cxx::__pool_alloc<int>().deallocate((int*)p5, 9);
	#endif
		}
	} //namespace
	
	int main(void)
	{
		jj01::test_primitives();
		return 0;
	}

编译运行结果如下：

![](https://i.imgur.com/wFWZoad.png)

可见 int* p4 = allocator<int>().allocate(3, (int*)0) 操作成功申请了三个int的空间。

### 二、4.基本构件之 newdelete expression ###

#### 1、内存申请 ####

![](https://i.imgur.com/SU7545t.png)

上面这张图揭示了new操作背后编译器做的事：

- 1、第一步通过operator new()操作分配一个目标类型的内存大小，这里是Complex的大小；
- 2、第二步通过static_cast将得到的内存块强制转换为目标类型指针，这里是Complex*
- 3、第三版调用目标类型的构造方法，但是需要注意的是，直接通过pc->Complex::Complex(1, 2)这样的方法调用构造函数只有编译器可以做，用户这样做将产生错误。

值得注意的是，operator new()操作的内部是调用了malloc()函数。

#### 2、内存释放 ####

![](https://i.imgur.com/FS9k7Lw.png)

同样地，delete操作第一步也是调用了对象的析构函数，然后再通过operator delete()函数释放内存。

#### 3、模拟编译器直接调用构造和析构函数 ####
下面的代码测试环节为VS2013：

	#include <iostream>
	#include <string>
	//#include <memory>				 //std::allocator  
	using namespace std;
	
	namespace jj02
	{
	
		class A
		{
		public:
			int id;
	
			A() : id(0)      { cout << "default ctor. this=" << this << " id=" << id << endl; }
			A(int i) : id(i) { cout << "ctor. this=" << this << " id=" << id << endl; }
			~A()             { cout << "dtor. this=" << this << " id=" << id << endl; }
		};
	
		void test_call_ctor_directly()
		{
			cout << "\ntest_call_ctor_directly().......... \n";
	
			string* pstr = new string;
			cout << "str= " << *pstr << endl;
			//! pstr->string::string("jjhou");  
			//[Error] 'class std::basic_string<char>' has no member named 'string'
			//! pstr->~string();	//crash -- 其語法語意都是正確的, crash 只因為上一行被 remark 起來嘛.  
			cout << "str= " << *pstr << endl;
	
	
			//------------
	
			A* pA = new A(1);         	//ctor. this=000307A8 id=1
			cout << pA->id << endl;   	//1
			pA->A::A(3);
			cout << pA->id << endl;
			//!	pA->A::A(3);                //in VC6 : ctor. this=000307A8 id=3
			//in GCC : [Error] cannot call constructor 'jj02::A::A' directly
	
			A::A(5);
			//!	A::A(5);	  				//in VC6 : ctor. this=0013FF60 id=5
			//         dtor. this=0013FF60  	
			//in GCC : [Error] cannot call constructor 'jj02::A::A' directly
			//         [Note] for a function-style cast, remove the redundant '::A'
	
			cout << pA->id << endl;   	//in VC6 : 3
			//in GCC : 1  	
	
			delete pA;                	//dtor. this=000307A8 
	
			//simulate new
			void* p = ::operator new(sizeof(A));
			cout << "p=" << p << endl; 	//p=000307A8
			pA = static_cast<A*>(p);
			pA->A::A(2);
			//!	pA->A::A(2);				//in VC6 : ctor. this=000307A8 id=2
			//in GCC : [Error] cannot call constructor 'jj02::A::A' directly  	
	
			cout << pA->id << endl;     //in VC6 : 2
			//in GCC : 0  	
	
			//simulate delete
			pA->~A();					//dtor. this=000307A8 
			::operator delete(pA);		//free()
		}
	} //namespace
	
	int main(void)
	{
		jj02::test_call_ctor_directly();
		return 0;
	}

编译运行结果如下：

![](https://i.imgur.com/pFUmLy0.png)

具体的内容可见代码注解和打印效果。