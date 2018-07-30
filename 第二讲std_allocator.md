## std::allocator ##

### 一、malloc()内部原理 ###

#### 1、VC6.0 malloc ####

![](https://i.imgur.com/gQUae6j.png)

![](https://i.imgur.com/cmEOkpQ.png)

#### 2、BC5 malloc ####

![](https://i.imgur.com/ob9hMFD.png)

#### 3、G2.9 malloc ####

![](https://i.imgur.com/e2MZ9ZG.png)

![](https://i.imgur.com/FVlBA20.png)


#### 4、\_\_pool_alloc ###

![](https://i.imgur.com/K7XxqvN.png)

![](https://i.imgur.com/6MTLUB7.png)

![](https://i.imgur.com/AHgQElz.png)

![](https://i.imgur.com/BVIH5XG.png)


### 二、std::alloc ###

#### 1、std:alloc运作模式 ####

![](https://i.imgur.com/lzcpFvY.png)

如果客户需要一个32byte的内存块，那么std::alloc获取一个32 * 20 byte的内存块然后第一块32type的内存小块给用户，对应于上图的绿色部分，其他的类似，但是也有上限。如果该链表组维护的链表最大的一个小块为128byte，但是用户申请内存块超过了128byte，那么std::alloc将调用malloc给用户分配空间，然后该块将带上cookie头和尾。

![](https://i.imgur.com/8MNTpki.png)


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

