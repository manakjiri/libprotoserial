/*
 * This file is a part of the libprotoserial project
 * https://github.com/georges-circuits/libprotoserial
 * 
 * Copyright (C) 2022 Jiří Maňák - All Rights Reserved
 * For contact information visit https://manakjiri.eu/
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/gpl.html>
 */

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

#ifndef _SP_OBSERVER
#define _SP_OBSERVER

#include <list>
#include <algorithm>
#include <functional>
#include <memory>

namespace sp
{
    class subscription
    {
        public:
        using id = unsigned int;
        
        static const id invalid_id = 0;

        subscription() : _id((++_id_count == invalid_id) ? ++_id_count : _id_count) {}
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
    struct subject
    {
        using fn_type = std::function<void(Args...)>;
        
        private:
        struct fn_base 
        {
            virtual ~fn_base() {}
            virtual void exec(Args... arg) {}
            virtual void exec() {}
            virtual bool is_subs() const = 0;
        };
        struct fn_subs : public fn_base
        {
            fn_type fn;
            fn_subs(fn_type f) : fn(f) {}
            void exec(Args... arg) {fn(std::forward<Args>(arg)...);}
            bool is_subs() const {return true;}
        };
        struct fn_watch : public fn_base
        {
            std::function<void(void)> fn;
            fn_watch(std::function<void(void)> f) : fn(f) {}
            void exec() {fn();}
            bool is_subs() const {return false;}
        };

        struct entry
        {
            std::unique_ptr<fn_base> fn;
            subscription s;
        };

        public:

        subject() = default;
        subject(const subject &) = delete;
        subject(subject &&) = delete;
        subject & operator=(const subject &) = delete;
        subject & operator=(subject &&) = delete;

        subscription watch(std::function<void(void)> fn)
        {
            auto f = std::unique_ptr<fn_base>(new fn_watch(fn));
            auto & t = _callbacks.emplace_back(std::move(f), subscription());
            return t.s;
        }

        template<typename Class>
        subscription watch(void(Class::*f)(), Class *instance) {
            return watch(std::bind(f, static_cast<Class*>(instance)));
        }

        subscription subscribe(fn_type fn)
        {
            auto f = std::unique_ptr<fn_base>(new fn_subs(fn));
            auto & t = _callbacks.emplace_back(std::move(f), subscription());
            return t.s;
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
            _callbacks.erase(
                std::remove_if(_callbacks.begin(), _callbacks.end(), [&](const auto & e){return e.s == s;}),
                _callbacks.end()
            );
        }

        constexpr void emit(Args... arg) const
        {
            for (auto& e : _callbacks)
            {
                if (e.fn->is_subs())
                    e.fn->exec(std::forward<Args>(arg)...);
                else
                    e.fn->exec();
            }
        }

        private:
        std::list<entry> _callbacks;
    };


}


#endif


