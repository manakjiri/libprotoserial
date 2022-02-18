#include <vector>
#include <list>
#include <tuple>
#include <algorithm>
#include <functional>

#include <string>
#include <iostream>

using namespace std;
using namespace std::placeholders;

class subscription
{
    public:
    using id = unsigned int;
    
    static const id invalid_id = 0;

    subscription() : _id(++_id_count) {}
    id get_id() const {return _id;}
    
    private:
    id _id = invalid_id;
    static id _id_count;
};

bool operator==(const subscription &lhs, const subscription &rhs)
{
    return lhs.get_id() == rhs.get_id();
}

subscription::id subscription::_id_count = 0;


template<typename... Args>
class subject
{   
    public:
    //using type = void(*)(Args...);
    using type = function<void(Args...)>;

    subject() = default;

    subject(const subject &) = delete;
    subject(subject &&) = delete;
    subject & operator=(const subject &) = delete;
    subject & operator=(subject &&) = delete;

    subscription subscribe(type fn)
    {
        subscription s;
        _callbacks.push_back(tuple<type, subscription>(fn, s));
        return s;
    }

    void unsubscribe(subscription s)
    {
        _callbacks.erase(std::remove_if(_callbacks.begin(), _callbacks.end(), [&](auto t){
            return std::get<1>(t) == s;
        }), _callbacks.end());
    }

    void emit(Args... arg)
    {
        for (auto& c : _callbacks)
            std::get<0>(c)(arg...);
    }

    private:
    list<tuple<type, subscription>> _callbacks;
};


void callback1(int num, string str)
{
    cout << "callback1: num: " << num << " str: " << str << endl;
}
void callback2(int num, string str)
{
    cout << "callback2: num: " << num << " str: " << str << endl;
}

class observer
{
    public:
    void callback(int num, string str)
    {
        cout << "observer(" << ++count << "): num: " << num << " str: " << str << endl;
    }
    int count = 0;
};


int main(int argc, char const *argv[])
{
    subject<int, string> s;
    observer o;

    s.subscribe(bind(&observer::callback, &o, _1, _2));
    s.subscribe(callback1);
    auto subs = s.subscribe(callback2);
    s.emit(0, "hello");

    s.unsubscribe(subs);
    s.emit(1, "world");


    return 0;
}
