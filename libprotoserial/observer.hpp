/*  
 * a way to provide one way communication between objects through functions
 * regardless of the arguments
 * 
 * subject.emit(data, adress, port)
 *  -> observer1
 *  ..
 *  -> observerN
 * 
 * subject can be a separate class from the the object that is actually creating 
 * the event since the data can be passed into the subjects easily
 * 
 * observers must be member functions of objects since they usually need to keep
 * their own data and contexts - this is where the problem arises since now we can't
 * teplate the entire object because observers may want to subscribe to multiple
 * subjects who each can have different signatures
 * 
 * subjects must be templates of the observer's function signature, or rather 
 * only such observers can subscribe who match the signature of the subject
 * 
 */

#include <list>
#include <tuple>
#include <algorithm>
#include <functional>

namespace sp
{
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
        using type = std::function<void(Args...)>;

        subject() = default;
        subject(const subject &) = delete;
        subject(subject &&) = delete;
        subject & operator=(const subject &) = delete;
        subject & operator=(subject &&) = delete;

        subscription subscribe(type fn)
        {
            subscription s;
            _callbacks.push_back(std::tuple<type, subscription>(fn, s));
            return s;
        }

        /* consider https://stackoverflow.com/questions/15024223/how-to-implement-an-easy-bind-that-automagically-inserts-implied-placeholders */
        template<typename Ret, typename Class, typename A1>
        subscription subscribe(Ret (Class::*f)(A1), Class *instance) {
            return subscribe(std::bind(f, static_cast<Class*>(instance), std::placeholders::_1));
        }
        template<typename Ret, typename Class, typename A1, typename A2>
        subscription subscribe(Ret (Class::*f)(A1, A2), Class *instance) {
            return subscribe(std::bind(f, static_cast<Class*>(instance), std::placeholders::_1, std::placeholders::_2));
        }
        template<typename Ret, typename Class, typename A1, typename A2, typename A3>
        subscription subscribe(Ret (Class::*f)(A1, A2, A3), Class *instance) {
            return subscribe(std::bind(f, static_cast<Class*>(instance), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        }

        void unsubscribe(subscription s)
        {
            _callbacks.erase(std::remove_if(_callbacks.begin(), _callbacks.end(), [&](auto t){
                return std::get<1>(t) == s;
            }), _callbacks.end());
        }

        constexpr void emit(Args... arg) const
        {
            for (auto& c : _callbacks)
                std::get<0>(c)(arg...);
        }

        private:
        std::list<std::tuple<type, subscription>> _callbacks;
    };


}



