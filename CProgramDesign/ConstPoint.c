#include<stdio.h>
#include<string.h>

int main()

{

    int a[]={43,27,15,61,55};

    int *const p=a;                                               //定义const指针 p 指向数组 a[] 头部

    int *pa=&a;                                                    //定义二重指针 pa 指向存放 数组指针 a 的内存单元的地址（如果a是指针的话）

    int *pp=&p;                                                    //定义二重指针 pp 指向存放const指针 p 的内存单元的地址

    printf("a=%p\t\t p =%p\n",a,p);                      //输出数组指针 a 和const指针 p 的值

    printf("pa=%p\t\t pp=%p\n",pa,pp);               //输出二重指针 pa 和 pp 的值

    printf("*pa=%d\t\t\t\t*pp=%p\n",*pa,*pp);      //输出二重指针 pa 和 pp 所指向的内存单元所存放的值，按理说应该分别和 a 和 p相等 

    printf(" *a=%d\t\t\t\t*p=%d\n",*a,*p);             //输出const指针指向的内存单元所存放的值

    printf("&a=%p\t",&a);                          //输出 数组指针 a 本身的地址 

    printf("&p=%p\n",&p);                         //输出 const指针 p 本身的地址

    printf("sizeof(a)==%lu\t\t\tsizeof(p)==%lu\n", sizeof(a), sizeof(p));        //查看大小

    return 0;

}
