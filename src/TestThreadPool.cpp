#define TESTTHREADPOOL 0
#if TESTTHREADPOOL 

#include <iostream>
#include "../include/ThreadPool.h"

using namespace std;
using namespace Heeb;

int fun1(int n) {
    int result=0;
    for(int i=0; i<n; ++i) {
        cout<<"This is fun1:"<<i<<endl;
        result+=i;
    }
    return result;
}
class fun2 {
public:
    int operator() (int n) {
        cout<<"This is fun1:"<<n<<endl;
        return n;
    }

};

int main() {
    auto f3=[](int n) {
        cout<<"This is fun1:"<<n<<endl;
        return n;
    };

    ThreadPool m_pool(3, "num_pool");
    ThreadPool s_pool{"sPool_1", "sPool_2", "sPool_3"};
    for(int i=0; i<10; ++i) {
        future<int> r1 = m_pool.addTask(fun1, i);
        fun2 Fun2;
        future<int> r2 = m_pool.addTask(Fun2, i);
        future<int> r3 = m_pool.addTask(f3, i);
        future<int> r11 = s_pool.addTask(fun1, i);
        future<int> r22 = s_pool.addTask(Fun2, i);
        future<int> r33 = s_pool.addTask(f3, i);
        cout<<"m->fun1->r1:"<<r1.get()<<endl;
        cout<<"m->fun2->r2:"<<r2.get()<<endl;
        cout<<"m->fun3->r3:"<<r3.get()<<endl;
        cout<<"s->fun1->r12:"<<r11.get()<<endl;
        cout<<"s->fun2->r22:"<<r22.get()<<endl;
        cout<<"s->fun3->r33:"<<r33.get()<<endl;
    }

}

#endif