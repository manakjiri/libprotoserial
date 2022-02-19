#include <string>
#include <iostream>
#include <chrono>

#include "libprotoserial/observer.hpp"

using namespace std;
using namespace std::placeholders;


static int callback1_count = 0;
static int callback2_count = 0;

void callback1(int num, string str)
{
    //cout << "callback1: num: " << num << " str: " << str << endl;
    callback1_count++;
}
void callback2(int num, string str)
{
    //cout << "callback2: num: " << num << " str: " << str << endl;
    callback2_count++;
}

class observer
{
    public:
    void callback(int num, string str)
    {
        //cout << "observer(" << ++count << "): num: " << num << " str: " << str << endl;
        count++;
    }
    int count = 0;
};



#define TIME_THIS(repeats, name, exp) \
auto start = chrono::steady_clock::now();\
for(int i = 0; i < repeats; ++i) {\
    exp;\
}\
auto diff = (chrono::steady_clock::now() - start) / repeats;\
cout << name << ": "<< chrono::duration<double, nano>(diff).count() << " ns" << endl;

int main(int argc, char const *argv[])
{

    /* s.subscribe(bind(&observer::callback, &o, _1, _2));
    //s.subscribe(&observer::callback, &o);
    s.subscribe(callback1);
    auto subs = s.subscribe(callback2);
    s.emit(0, "hello");

    s.unsubscribe(subs);
    s.emit(1, "world"); */

    auto N = 1000;

/*     auto test = [&](string name, auto &s){
        auto start = chrono::steady_clock::now();
        for(int i = 0; i < N; ++i) {
            s.emit(i, "world");
        }
        auto diff = (chrono::steady_clock::now() - start) / N;
        cout << name << ": "<< chrono::duration<double, nano>(diff).count() << " ns" << endl;
    }; */

    {
        TIME_THIS(N, "jsut fn", callback2(i, "world"));
        cout << callback2_count << endl;
    }

    {
        sp::subject<int, string> s;
        TIME_THIS(N, "none", s.emit(i, "world"));
    }

    {
        sp::subject<int, string> s;
        auto subs = s.subscribe(callback1);
        TIME_THIS(N, "normal", s.emit(i, "world"));
        cout << callback1_count << endl;
    }

    {
        observer o;
        sp::subject<int, string> s;
        auto subs = s.subscribe(&observer::callback, &o);
        TIME_THIS(N, "member", s.emit(i, "world"));
        cout << o.count << endl;
    }


    return 0;
}
