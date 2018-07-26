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