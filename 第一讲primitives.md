## C++内存管理 ##

### 一、四种内存分配和释放方法 ###

![](https://i.imgur.com/AREog97.png)

在编程时可以通过上图的几种方法直接或间接地操作内存。下面将介绍四种C++内存操作方法：

![](https://i.imgur.com/8F62Mf8.png)

通常可以使用malloc和new来分配内存，当然也可以使用::operator new()和分配器allocator来操作内存，下面将具体介绍这些函数的使用方法。对于不同的编译器，其allocate函数的接口也有所不同：

![](https://i.imgur.com/lljUGp1.png)

对于GNU C，不同版本又有所不同：

![](https://i.imgur.com/xkUvBlJ.png)

这张图中的__gnu_cxx::__pool_alloc<T>().allocate()对应于上张图中的allocator<T>().allocate()。

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

### 二、基本构件之 new/delete expression ###

#### 1、内存申请 ####

![](https://i.imgur.com/SU7545t.png)

上面这张图揭示了new操作背后编译器做的事：

- 1、第一步通过operator new()操作分配一个目标类型的内存大小，这里是Complex的大小；
- 2、第二步通过static_cast将得到的内存块强制转换为目标类型指针，这里是Complex*
- 3、第三版调用目标类型的构造方法，但是需要注意的是，直接通过pc->Complex::Complex(1, 2)这样的方法调用构造函数只有编译器可以做，用户这样做将产生错误。

值得注意的是，operator new()操作的内部是调用了malloc()函数。

#### 2、内存释放 ####

![](https://i.imgur.com/FS9k7Lw.png)

同样地，delete操作第一步也是调用了对象的析构函数，然后再通过operator delete()函数释放内存，本质上也是调用了free函数。

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

VS下可以直接通过内存空间调用构造函数，但侯杰测试在GNU C下无法通过，具体的内容可见代码注解和打印效果。

### 三、Array new ###

![](https://i.imgur.com/aWEq4Ve.png)

上图主要展示的是关于new array内存分配的大致情况。当new一个数组对象时（例如 new Complex[3]），编译器将分配一块内存，这块内存首部是关于对象内存分配的一些标记，然后下面会分配三个连续的对象内存，在使用delete释放内存时需要使用delete[]。如果不使用delete[]，只是使用delete只会将分配的三块内存空间释放，但不会调用对象的析构函数，如果对象内部还使用了new指向其他空间，如果指向的该空间里的对象的析构函数没有意义，那么不会造成问题，如果有意义，那么由于该部分对象析构函数不会调用，那么将会导致内存泄漏。图中new string[3]便是一个例子，虽然str[0]、str[1]、str[2]被析构了，但只是调用了str[0]的析构函数，其他对象的析构函数不被调用，这里就会出问题。

下面将演示数组对象创建与析构过程：

![](https://i.imgur.com/7FwNSzm.png)

	#include <iostream>
	#include <new>		//placement new
	using namespace std;
	
	namespace jj03
	{
	
		class A
		{
		public:
			int id;
	
			A() : id(0)      { cout << "default ctor. this=" << this << " id=" << id << endl; }
			A(int i) : id(i) { cout << "ctor. this=" << this << " id=" << id << endl; }
			~A()             { cout << "dtor. this=" << this << " id=" << id << endl; }
		};
	
		void test_array_new_and_placement_new()
		{
			cout << "\ntest_placement_new().......... \n";
	
			size_t size = 3;
	
			{
				//case 1
				//模擬 memory pool 的作法, array new + placement new. 崩潰 
	
				A* buf = (A*)(new char[sizeof(A)*size]);
				A* tmp = buf;
	
				cout << "buf=" << buf << "  tmp=" << tmp << endl;
	
				for (int i = 0; i < size; ++i)
					new (tmp++) A(i);  			//3次 调用ctor 
	
				cout << "buf=" << buf << "  tmp=" << tmp << endl;
	
				//!	delete [] buf;    	//crash. why?
				//因為這其實是個 char array，看到 delete [] buf; 編譯器會企圖喚起多次 A::~A. 
				// 但 array memory layout 中找不到與 array 元素個數 (本例 3) 相關的信息, 
				// -- 整個格局都錯亂 (從我對 VC 的認識而言)，於是崩潰。 
				delete buf;     	//dtor just one time, ~[0]	
	
				cout << "\n\n";
			}
	
			{
				//case 2
				//回頭測試單純的 array new
	
				A* buf = new A[size];  //default ctor 3 次. [0]先於[1]先於[2])
				//A必須有 default ctor, 否則 [Error] no matching function for call to 'jj02::A::A()'
				A* tmp = buf;
	
				cout << "buf=" << buf << "  tmp=" << tmp << endl;
	
				for (int i = 0; i < size; ++i)
					new (tmp++) A(i);  		//3次 ctor 
	
				cout << "buf=" << buf << "  tmp=" << tmp << endl;
	
				delete[] buf;    //dtor three times (次序逆反, [2]先於[1]先於[0])	
			}
	
			{
				//case 3	
				//掌握崩潰原因, 再次模擬 memory pool作法, array new + placement new. 	
				//不, 不做了, 因為 memory pool 只是供應 memory, 它並不管 construction, 
				//也不管 destruction. 它只負責回收 memory. 
				//所以它是以 void* 或 char* 取得 memory, 釋放 (刪除)的也是 void* or char*.  
				//不像本例 case 1 釋放 (刪除) 的是 A*. 
				//
				//事實上 memory pool 形式如 jj04::test 
			}
	
		}
	} //namespace
	
	int main(void)
	{
		jj03::test_array_new_and_placement_new();
		return 0;
	}

编译运行结果如下：

![](https://i.imgur.com/kcoXFR6.png)

构造函数调用顺序是按照构建对象顺序来执行的，但是析构函数执行却相反。值得注意的是，在调用了delete的大括号代码段中，数组有三个元素，但最后只调用了第一个对象的析构函数。

接下来将更具体地展示new array对象的内存分配情况：

![](https://i.imgur.com/mQAjijM.png)

如果使用new分配十个内存的int，内存空间如上图所示，首先内存块会有一个头和尾，黄色部分为debug信息，灰色部分才是真正使用到的内存，蓝色部分的12bytes是为了让该内存块以16字节对齐。在这个例子中delete pi和delete[] pi效果是一样的，因为int没有析构函数。但是下面的例子就不一样了：

![](https://i.imgur.com/RCX6Hfm.png)

上图通过new申请三个Demo空间大小，内存块使用了96byte，这里是这样计算得到的:黄色部分调试信息32 + 4 = 36byte；黄色部分下面的“3”用于标记实际分配给对象内存个数，这里是三个所以里面内容为3，消耗4byte；Demo内有三个int类型成员变量，一个Demo消耗内存3 * 4 = 12byte，由于有三个Demo，所以消耗了12 * 3 = 36byte空间；到目前为止消耗36 + 4 + 36 = 76byte，加上头尾cookie一共8byte一共消耗84byte，由于需要16位对齐，所以填充蓝色部分为12byte，一共消耗了84 + 12 = 96byte。这里释放内存时需要加上delete[]，上面分配内存中有个标记“3”，所以编译器将释放三个Demo对象空间，如果不加就会报错。

### 四、placement new ###

![](https://i.imgur.com/aWyButl.png)

### 五、重载 ###

#### 1、C++内存分配的途径 ####

![](https://i.imgur.com/xAguah0.png)

如果是正常情况下，调用new之后走的是第二条路线，如果在类中重载了operator new()，那么走的是第一条路线，但最后还是要调用到系统的::operator new()函数，这在后续的例子中会体现。

![](https://i.imgur.com/XkgjnI1.png)

对于GNU C，背后使用的allocate()函数最后也是调用了系统的::operator new()函数。

#### 2、重载new 和 delete ####

![](https://i.imgur.com/2o83TNy.png)

上面这张图演示了如何重载系统的::operator new()函数，该方法最后也是模拟了系统的做法，效果和系统的方法一样，但一般不推荐重载::operator new()函数，因为它对全局有影响，如果使用不当将造成很大的问题。

![](https://i.imgur.com/KMrjz7s.png)

如果是在类中重载operator new()方法，那么该方法有N多种形式，但必须保证函数参数列表第一个参数是size_t类型变量；对于operator delete()，第一个参数必须是void* 类型，第二个size_t是可选项，可以去掉。

![](https://i.imgur.com/sZrLSr8.png)

对于operator new[]和operator delete[]函数的重载，和前面类似。

![](https://i.imgur.com/S2yG6Um.png)

![](https://i.imgur.com/6D7odtt.png)

![](https://i.imgur.com/KjGXpFs.png)

![](https://i.imgur.com/GPW5wRa.png)

![](https://i.imgur.com/0ZoNJdM.png)

![](https://i.imgur.com/VgcPaVf.png)

![](https://i.imgur.com/PcPsPWd.png)

#### 3、测试案例 ####

测试一：

	#include <cstddef>
	#include <iostream>
	#include <string>
	using namespace std;
	
	namespace jj06
	{
	
		class Foo
		{
		public:
			int _id;
			long _data;
			string _str;
	
		public:
			static void* operator new(size_t size);
			static void  operator delete(void* deadObject, size_t size);
			static void* operator new[](size_t size);
			static void  operator delete[](void* deadObject, size_t size);
	
			Foo() : _id(0)      { cout << "default ctor. this=" << this << " id=" << _id << endl; }
			Foo(int i) : _id(i) { cout << "ctor. this=" << this << " id=" << _id << endl; }
			//virtual 
			~Foo()              { cout << "dtor. this=" << this << " id=" << _id << endl; }
	
			//不加 virtual dtor, sizeof = 12, new Foo[5] => operator new[]() 的 size 參數是 64, 
			//加了 virtual dtor, sizeof = 16, new Foo[5] => operator new[]() 的 size 參數是 84, 
			//上述二例，多出來的 4 可能就是個 size_t 欄位用來放置 array size. 
		};
	
		void* Foo::operator new(size_t size)
		{
			Foo* p = (Foo*)malloc(size);
			cout << "Foo::operator new(), size=" << size << "\t  return: " << p << endl;
	
			return p;
		}
	
		void Foo::operator delete(void* pdead, size_t size)
		{
			cout << "Foo::operator delete(), pdead= " << pdead << "  size= " << size << endl;
			free(pdead);
		}
	
		void* Foo::operator new[](size_t size)
		{
			Foo* p = (Foo*)malloc(size);  //crash, 問題可能出在這兒 
			cout << "Foo::operator new[](), size=" << size << "\t  return: " << p << endl;
	
			return p;
		}
	
		void Foo::operator delete[](void* pdead, size_t size)
		{
			cout << "Foo::operator delete[](), pdead= " << pdead << "  size= " << size << endl;
	
			free(pdead);
		}
	
		//-------------	
		void test_overload_operator_new_and_array_new()
		{
			cout << "\ntest_overload_operator_new_and_array_new().......... \n";
	
			cout << "sizeof(Foo)= " << sizeof(Foo) << endl;
	
			{
				Foo* p = new Foo(7);
				delete p;
	
				Foo* pArray = new Foo[5];	//無法給 array elements 以 initializer 
				delete[] pArray;
			}
	
			{
				cout << "testing global expression ::new and ::new[] \n";
				// 這會繞過 overloaded new(), delete(), new[](), delete[]() 
				// 但當然 ctor, dtor 都會被正常呼叫.  
	
				Foo* p = ::new Foo(7);
				::delete p;
	
				Foo* pArray = ::new Foo[5];
				::delete[] pArray;
			}
		}
	} //namespace
	
	int main(void)
	{
		jj06::test_overload_operator_new_and_array_new();
		return 0;
	}

编译运行结果如下：

![](https://i.imgur.com/c6l7tRe.png)

测试二：

	#include <vector>  //for test
	#include <cstddef>
	#include <iostream>
	#include <string>
	using namespace std;
	
	namespace jj07
	{
	
		class Bad { };
		class Foo
		{
		public:
			Foo() { cout << "Foo::Foo()" << endl; }
			Foo(int) {
				cout << "Foo::Foo(int)" << endl;
				// throw Bad();  
			}
	
			//(1) 這個就是一般的 operator new() 的重載 
			void* operator new(size_t size){
				cout << "operator new(size_t size), size= " << size << endl;
				return malloc(size);
			}
	
			//(2) 這個就是標準庫已經提供的 placement new() 的重載 (形式)
			//    (所以我也模擬 standard placement new 的動作, just return ptr) 
			void* operator new(size_t size, void* start){
				cout << "operator new(size_t size, void* start), size= " << size << "  start= " << start << endl;
				return start;
			}
	
			//(3) 這個才是嶄新的 placement new 
			void* operator new(size_t size, long extra){
				cout << "operator new(size_t size, long extra)  " << size << ' ' << extra << endl;
				return malloc(size + extra);
			}
	
			//(4) 這又是一個 placement new 
			void* operator new(size_t size, long extra, char init){
				cout << "operator new(size_t size, long extra, char init)  " << size << ' ' << extra << ' ' << init << endl;
				return malloc(size + extra);
			}
	
			//(5) 這又是一個 placement new, 但故意寫錯第一參數的 type (它必須是 size_t 以滿足正常的 operator new) 
			//!  	void* operator new(long extra, char init) { //[Error] 'operator new' takes type 'size_t' ('unsigned int') as first parameter [-fpermissive]
			//!	  	cout << "op-new(long,char)" << endl;
			//!    	return malloc(extra);
			//!  	} 	
	
			//以下是搭配上述 placement new 的各個 called placement delete. 
			//當 ctor 發出異常，這兒對應的 operator (placement) delete 就會被喚起. 
			//應該是要負責釋放其搭檔兄弟 (placement new) 分配所得的 memory.  
			//(1) 這個就是一般的 operator delete() 的重載 
			void operator delete(void*, size_t)
			{
				cout << "operator delete(void*,size_t)  " << endl;
			}
	
			//(2) 這是對應上述的 (2)  
			void operator delete(void*, void*)
			{
				cout << "operator delete(void*,void*)  " << endl;
			}
	
			//(3) 這是對應上述的 (3)  
			void operator delete(void*, long)
			{
				cout << "operator delete(void*,long)  " << endl;
			}
	
			//(4) 這是對應上述的 (4)  
			//如果沒有一一對應, 也不會有任何編譯報錯 
			void operator delete(void*, long, char)
			{
				cout << "operator delete(void*,long,char)  " << endl;
			}
	
		private:
			int m_i;
		};
	
	
		//-------------	
		void test_overload_placement_new()
		{
			cout << "\n\n\ntest_overload_placement_new().......... \n";
	
			Foo start;  //Foo::Foo
	
			Foo* p1 = new Foo;           //op-new(size_t)
			Foo* p2 = new (&start) Foo;  //op-new(size_t,void*)
			Foo* p3 = new (100) Foo;     //op-new(size_t,long)
			Foo* p4 = new (100, 'a') Foo; //op-new(size_t,long,char)
	
			Foo* p5 = new (100) Foo(1);     //op-new(size_t,long)  op-del(void*,long)
			Foo* p6 = new (100, 'a') Foo(1); //
			Foo* p7 = new (&start) Foo(1);  //
			Foo* p8 = new Foo(1);           //
			//VC6 warning C4291: 'void *__cdecl Foo::operator new(unsigned int)'
			//no matching operator delete found; memory will not be freed if
			//initialization throws an exception
		}
	} //namespace	
	
	int main(void)
	{
		jj07::test_overload_placement_new();
		return 0;
	}

编译运行结果如下：

![](https://i.imgur.com/J7nEVmm.png)


### 五、pre-class allocator ###

![](https://i.imgur.com/o1landO.png)

![](https://i.imgur.com/XeVjepx.png)

案例如下：

	#include <cstddef>
	#include <iostream>
	using namespace std;
	
	namespace jj04
	{
		//ref. C++Primer 3/e, p.765
		//per-class allocator 
	
		class Screen {
		public:
			Screen(int x) : i(x) { };
			int get() { return i; }
	
			void* operator new(size_t);
			void  operator delete(void*, size_t);	//(2)
			//! void  operator delete(void*);			//(1) 二擇一. 若(1)(2)並存,會有很奇怪的報錯 (摸不著頭緒) 
	
		private:
			Screen* next;
			static Screen* freeStore;
			static const int screenChunk;
		private:
			int i;
		};
		Screen* Screen::freeStore = 0;
		const int Screen::screenChunk = 24;
	
		void* Screen::operator new(size_t size)
		{
			Screen *p;
			if (!freeStore) {
				//linked list 是空的，所以攫取一大塊 memory
				//以下呼叫的是 global operator new
				size_t chunk = screenChunk * size;
				freeStore = p =
					reinterpret_cast<Screen*>(new char[chunk]);
				//將分配得來的一大塊 memory 當做 linked list 般小塊小塊串接起來
				for (; p != &freeStore[screenChunk - 1]; ++p)
					p->next = p + 1;
				p->next = 0;
			}
			p = freeStore;
			freeStore = freeStore->next;
			return p;
		}
	
	
		//! void Screen::operator delete(void *p)		//(1)
		void Screen::operator delete(void *p, size_t)	//(2)二擇一 
		{
			//將 deleted object 收回插入 free list 前端
			(static_cast<Screen*>(p))->next = freeStore;
			freeStore = static_cast<Screen*>(p);
		}
	
		//-------------
		void test_per_class_allocator_1()
		{
			cout << "\ntest_per_class_allocator_1().......... \n";
	
			cout << sizeof(Screen) << endl;		//8	
	
			size_t const N = 100;
			Screen* p[N];
	
			for (int i = 0; i< N; ++i)
				p[i] = new Screen(i);
	
			//輸出前 10 個 pointers, 用以比較其間隔 
			for (int i = 0; i< 10; ++i)
				cout << p[i] << endl;
	
			for (int i = 0; i< N; ++i)
				delete p[i];
		}
	} //namespace
	
	int main(void)
	{
		jj04::test_per_class_allocator_1();
		return 0;
	}

编译运行结果如下：

![](https://i.imgur.com/AAD8dur.png)

每个对象以8byte对齐。内存池本质上是分配了一大块内存，然后将该内存分割为多个小块通过链表拼接起来，所以物理上不一定连续但是逻辑上是连续的。

![](https://i.imgur.com/Va92P0d.png)

![](https://i.imgur.com/DEjg6FL.png)

案例如下：

	#include <cstddef>
	#include <iostream>
	using namespace std;
	
	namespace jj05
	{
		//ref. Effective C++ 2e, item10
		//per-class allocator 
	
		class Airplane {   //支援 customized memory management
		private:
			struct AirplaneRep {
				unsigned long miles;
				char type;
			};
		private:
			union {
				AirplaneRep rep;  //此針對 used object
				Airplane* next;   //此針對 free list
			};
		public:
			unsigned long getMiles() { return rep.miles; }
			char getType() { return rep.type; }
			void set(unsigned long m, char t)
			{
				rep.miles = m;
				rep.type = t;
			}
		public:
			static void* operator new(size_t size);
			static void  operator delete(void* deadObject, size_t size);
		private:
			static const int BLOCK_SIZE;
			static Airplane* headOfFreeList;
		};
	
		Airplane* Airplane::headOfFreeList;
		const int Airplane::BLOCK_SIZE = 512;
	
		void* Airplane::operator new(size_t size)
		{
			//如果大小錯誤，轉交給 ::operator new()
			if (size != sizeof(Airplane))
			return ::operator new(size);
	
			Airplane* p = headOfFreeList;
	
			//如果 p 有效，就把list頭部移往下一個元素
			if (p)
				headOfFreeList = p->next;
			else {
				//free list 已空。配置一塊夠大記憶體，
				//令足夠容納 BLOCK_SIZE 個 Airplanes
				Airplane* newBlock = static_cast<Airplane*>
					(::operator new(BLOCK_SIZE * sizeof(Airplane)));
				//組成一個新的 free list：將小區塊串在一起，但跳過 
				//#0 元素，因為要將它傳回給呼叫者。
				for (int i = 1; i < BLOCK_SIZE - 1; ++i)
					newBlock[i].next = &newBlock[i + 1];
				newBlock[BLOCK_SIZE - 1].next = 0; //以null結束
	
				// 將 p 設至頭部，將 headOfFreeList 設至
				// 下一個可被運用的小區塊。
				p = newBlock;
				headOfFreeList = &newBlock[1];
			}
			return p;
		}
	
		// operator delete 接獲一塊記憶體。
		// 如果它的大小正確，就把它加到 free list 的前端
		void Airplane::operator delete(void* deadObject,
			size_t size)
		{
			if (deadObject == 0) return;
			if (size != sizeof(Airplane)) {
				::operator delete(deadObject);
				return;
			}
	
			Airplane *carcass =
				static_cast<Airplane*>(deadObject);
	
			carcass->next = headOfFreeList;
			headOfFreeList = carcass;
		}
	
		//-------------
		void test_per_class_allocator_2()
		{
			cout << "\ntest_per_class_allocator_2().......... \n";
	
			cout << sizeof(Airplane) << endl;    //8
	
			size_t const N = 100;
			Airplane* p[N];
	
			for (int i = 0; i< N; ++i)
				p[i] = new Airplane;
	
	
			//隨機測試 object 正常否 
			p[1]->set(1000, 'A');
			p[5]->set(2000, 'B');
			p[9]->set(500000, 'C');
			cout << p[1] << ' ' << p[1]->getType() << ' ' << p[1]->getMiles() << endl;
			cout << p[5] << ' ' << p[5]->getType() << ' ' << p[5]->getMiles() << endl;
			cout << p[9] << ' ' << p[9]->getType() << ' ' << p[9]->getMiles() << endl;
	
			//輸出前 10 個 pointers, 用以比較其間隔 
			for (int i = 0; i< 10; ++i)
				cout << p[i] << endl;
	
			for (int i = 0; i< N; ++i)
				delete p[i];
		}
	} //namespace
	
	int main(void)
	{
		jj05::test_per_class_allocator_2();
		return 0;
	}

编译运行结果如下：

![](https://i.imgur.com/fM1lMiv.png)

这种做法有几点比较有意思，首先是使用了union保存链表元素的next指针，这样整体上可以节省空间；其次是delete函数，它并没有直接将目标元素删除，而是将它当作下一个可分配的内存空间，也就是说如果delete某元素，那么该元素占有的内存空间不会被free掉，而是在下一次调用new时分配给新的对象。
### 六、static allocator ###

![](https://i.imgur.com/9dojXlJ.png)

![](https://i.imgur.com/OrZw2Ik.png)

![](https://i.imgur.com/akWdqgr.png)

代码如下：

	#include <cstddef>
	#include <iostream>
	#include <complex>
	using namespace std;
	
	namespace jj09
	{
	
		class allocator
		{
		private:
			struct obj {
				struct obj* next;  //embedded pointer
			};
		public:
			void* allocate(size_t);
			void  deallocate(void*, size_t);
			void  check();
	
		private:
			obj* freeStore = nullptr;
			const int CHUNK = 5; //小一點方便觀察 
		};
	
		void* allocator::allocate(size_t size)
		{
			obj* p;
	
			if (!freeStore) {
				//linked list 是空的，所以攫取一大塊 memory
				size_t chunk = CHUNK * size;
				freeStore = p = (obj*)malloc(chunk);
	
				//cout << "empty. malloc: " << chunk << "  " << p << endl;
	
				//將分配得來的一大塊當做 linked list 般小塊小塊串接起來
				for (int i = 0; i < (CHUNK - 1); ++i)	{  //沒寫很漂亮, 不是重點無所謂.  
					p->next = (obj*)((char*)p + size);
					p = p->next;
				}
				p->next = nullptr;  //last       
			}
			p = freeStore;
			freeStore = freeStore->next;
	
			//cout << "p= " << p << "  freeStore= " << freeStore << endl;
	
			return p;
		}
		void allocator::deallocate(void* p, size_t)
		{
			//將 deleted object 收回插入 free list 前端
			((obj*)p)->next = freeStore;
			freeStore = (obj*)p;
		}
		void allocator::check()
		{
			obj* p = freeStore;
			int count = 0;
	
			while (p) {
				cout << p << endl;
				p = p->next;
				count++;
			}
			cout << count << endl;
		}
		//--------------
	
		class Foo {
		public:
			long L;
			string str;
			static allocator myAlloc;
		public:
			Foo(long l) : L(l) {  }
			static void* operator new(size_t size)
			{ return myAlloc.allocate(size); }
			static void  operator delete(void* pdead, size_t size)
			{
				return myAlloc.deallocate(pdead, size);
			}
		};
		allocator Foo::myAlloc;
	
	
		class Goo {
		public:
			complex<double> c;
			string str;
			static allocator myAlloc;
		public:
			Goo(const complex<double>& x) : c(x) {  }
			static void* operator new(size_t size)
			{ return myAlloc.allocate(size); }
			static void  operator delete(void* pdead, size_t size)
			{
				return myAlloc.deallocate(pdead, size);
			}
		};
		allocator Goo::myAlloc;
	
		//-------------	
		void test_static_allocator_3()
		{
			cout << "\n\n\ntest_static_allocator().......... \n";
	
			{
				Foo* p[100];
	
				cout << "sizeof(Foo)= " << sizeof(Foo) << endl;
				for (int i = 0; i<23; ++i) {	//23,任意數, 隨意看看結果 
					p[i] = new Foo(i);
					cout << p[i] << ' ' << p[i]->L << endl;
				}
				//Foo::myAlloc.check();
	
				for (int i = 0; i<23; ++i) {
					delete p[i];
				}
				//Foo::myAlloc.check();
			}
	
			{
				Goo* p[100];
	
				cout << "sizeof(Goo)= " << sizeof(Goo) << endl;
				for (int i = 0; i<17; ++i) {	//17,任意數, 隨意看看結果 
					p[i] = new Goo(complex<double>(i, i));
					cout << p[i] << ' ' << p[i]->c << endl;
				}
				//Goo::myAlloc.check();
	
				for (int i = 0; i<17; ++i) {
					delete p[i];
				}
				//Goo::myAlloc.check();	
			}
		}
	} //namespace	
	
	int main(void)
	{
		jj09::test_static_allocator_3();
		return 0;
	}

编译运行结果如下：

![](https://i.imgur.com/czxP8JH.png)

之前的几个版本都是在类的内部重载了operator new()和operator delete()函数，这些版本都将分配内存的工作放在这些函数中，但现在的这个版本将这些分配内存的操作放在了allocator类中，这就渐渐接近了标准库的方法。从上面的代码中可以看到，两个类Foo和Goo中operator new()和operator delete()函数等很多部分代码类似，于是可以使用宏来将这些高度相似的代码提取出来，简化类的内部结构，但最后达到的结果是一样的：

![](https://i.imgur.com/baXE2RO.png)

![](https://i.imgur.com/nog6qAD.png)

### 七、global allocator ###

上面我们自己定义的分配器使用了一条链表来管理内存的，但标准库却用了多条链表来管理，这在后续会详细介绍：

![](https://i.imgur.com/0M5X6lY.png)

### 八、new handler ###

![](https://i.imgur.com/TJ9GqFz.png)

如果用户调用new申请一块内存，如果由于系统原因或者申请内存过大导致申请失败，这时将抛出异常，在一些老的编译器中可能会直接返回0，可以参考上图右边代码，当无法分配内存时，operator new()函数内部将调用_calnewh()函数，这个函数通过左边的typedef传入，看程序员是否能自己写一个handler处理函数来处理该问题。一般有两个选择，让更多的Memory可用或者直接abort()或exit()。下面是测试的一个结果：

![](https://i.imgur.com/JfazkE0.png)

该部分中自定义了处理函数noMoreMemory()并通过set_new_handler来注册该处理函数，在BCB4编译器中会调用到自定义的noMoreMemory()函数，但在右边的dev c++中却没有调用，这个还要看平台。

### 九、=default和=delete ###

![](https://i.imgur.com/WUtaH2m.png)

![](https://i.imgur.com/2lxEgBI.png)

更加详细的内容可以参考下面这篇文章:
> https://blog.csdn.net/u012333003/article/details/25299939