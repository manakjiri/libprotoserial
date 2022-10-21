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

#ifndef _SP_OBSERVER
#define _SP_OBSERVER

#include <list>
#include <algorithm>
#include <functional>
#include <memory>

namespace sp
{
    template<typename... Args>
    class subject
    {   
        public:
        using callback_type = std::function<void(Args...)>;

        class subscription
        {
            public:
            using id = unsigned int;
            
            constexpr static id invalid_id = 0;

            subscription() : _id(++_id_count) {}
            id get_id() const {return _id;}
            
            friend bool operator==(const subscription &lhs, const subscription &rhs)
            {
                return lhs.get_id() == rhs.get_id();
            }
            private:
            id _id = invalid_id;
            inline static id _id_count = invalid_id;
        };


        subject() = default;
        subject(const subject &) = delete;
        subject(subject &&) = delete;
        subject & operator=(const subject &) = delete;
        subject & operator=(subject &&) = delete;

        subscription subscribe(callback_type fn)
        {
            subscription s;
            _callbacks.push_back(std::tuple<callback_type, subscription>(fn, s));
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
        std::list<std::tuple<callback_type, subscription>> _callbacks;
    };


}


#endif


