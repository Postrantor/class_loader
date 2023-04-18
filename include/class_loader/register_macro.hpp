/*
 * Copyright (c) 2012, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holders nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CLASS_LOADER__REGISTER_MACRO_HPP_
#define CLASS_LOADER__REGISTER_MACRO_HPP_

#include <string>

// TODO(mikaelarguedas) remove this once console_bridge complies with this
// see https://github.com/ros/console_bridge/issues/55
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
#include "console_bridge/console.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include "class_loader/class_loader_core.hpp"
#include "console_bridge/console.h"

/**
 * @brief 注册插件类的宏，带有消息输出功能（Register plugin class macro with message output functionality）
 * @param Derived 派生类（Derived class）
 * @param Base 基类（Base class）
 * @param UniqueID 唯一标识符（Unique identifier）
 * @param Message 输出消息（Output message）
 */
#define CLASS_LOADER_REGISTER_CLASS_INTERNAL_WITH_MESSAGE(Derived, Base, UniqueID, Message)                                      \
  namespace                                                                                                                      \
  {                                                                                                                              \
    /* 定义代理执行结构体（Define proxy execution struct） */                                                         \
    struct ProxyExec##UniqueID                                                                                                   \
    {                                                                                                                            \
      /* 定义派生和基类别名（Define aliases for derived and base classes） */                                         \
      typedef Derived _derived;                                                                                                  \
      typedef Base _base;                                                                                                        \
                                                                                                                                 \
      /* 构造函数（Constructor） */                                                                                        \
      ProxyExec##UniqueID()                                                                                                      \
      {                                                                                                                          \
        /* 如果消息不为空，则输出消息（If the message is not empty, output the message） */                       \
        if (!std::string(Message).empty())                                                                                       \
        {                                                                                                                        \
          CONSOLE_BRIDGE_logInform("%s", Message);                                                                               \
        }                                                                                                                        \
        /* 注册插件（Register the plugin） */                                                                              \
        holder = class_loader::impl::registerPlugin<_derived, _base>(#Derived, #Base);                                           \
      }                                                                                                                          \
                                                                                                                                 \
    private:                                                                                                                     \
      /* 定义持有者，用于存储元对象基类的指针（Define holder to store pointer to meta object base class） */ \
      class_loader::impl::UniquePtr<class_loader::impl::AbstractMetaObjectBase> holder;                                          \
    };                                                                                                                           \
    /* 实例化代理执行结构体（Instantiate the proxy execution struct） */                                             \
    static ProxyExec##UniqueID g_register_plugin_##UniqueID;                                                                     \
  } // namespace

/**
 * @brief 内部跳转宏，用于在 CLASS_LOADER_REGISTER_CLASS_WITH_MESSAGE 中调用
 *        CLASS_LOADER_REGISTER_CLASS_INTERNAL_WITH_MESSAGE
 *        (Internal hop macro for calling CLASS_LOADER_REGISTER_CLASS_INTERNAL_WITH_MESSAGE
 *        within CLASS_LOADER_REGISTER_CLASS_WITH_MESSAGE)
 * @param Derived 派生类（Derived class）
 * @param Base 基类（Base class）
 * @param UniqueID 唯一标识符（Unique identifier）
 * @param Message 输出消息（Output message）
 */
#define CLASS_LOADER_REGISTER_CLASS_INTERNAL_HOP1_WITH_MESSAGE(Derived, Base, UniqueID, Message) \
  CLASS_LOADER_REGISTER_CLASS_INTERNAL_WITH_MESSAGE(Derived, Base, UniqueID, Message)

/**
 * @brief 注册插件类的宏，带有消息输出功能。在库加载时注册插件并输出消息。
 *        (Macro to register a plugin class with message output functionality.
 *        Registers the plugin and outputs a message when the library is loaded)
 * @param Derived 派生类（Derived class）
 * @param Base 基类（Base class）
 * @param Message 输出消息（Output message）
 */
#define CLASS_LOADER_REGISTER_CLASS_WITH_MESSAGE(Derived, Base, Message) \
  CLASS_LOADER_REGISTER_CLASS_INTERNAL_HOP1_WITH_MESSAGE(Derived, Base, __COUNTER__, Message)

/**
 * @macro This is the macro which must be declared within the source (.cpp) file for each class that is to be exported as plugin.
 * The macro utilizes a trick where a new struct is generated along with a declaration of static global variable of same type after it. The struct's constructor invokes a registration function with the plugin system. When the plugin system loads a library with registered classes in it, the initialization of static variables forces the invocation of the struct constructors, and all exported classes are automatically registerd.
 *
 * @param Derived 派生类，即要注册为插件的类 (Derived class, i.e., the class to be registered as a plugin)
 * @param Base 基类，派生类应该继承的基类 (Base class, the base class that the derived class should inherit from)
 */
#define CLASS_LOADER_REGISTER_CLASS(Derived, Base) \
  CLASS_LOADER_REGISTER_CLASS_WITH_MESSAGE(Derived, Base, "")

// 使用此宏时，需要在源文件（.cpp）中为要导出为插件的每个类声明它。
// (When using this macro, it needs to be declared within the source (.cpp) file for each class that is to be exported as a plugin.)

// 此宏利用了一种技巧，在生成新结构后，会生成与之相同类型的静态全局变量声明。结构的构造函数将调用带有插件系统的注册函数。
// (The macro utilizes a trick where a new struct is generated along with a declaration of static global variable of same type after it. The struct's constructor invokes a registration function with the plugin system.)

// 当插件系统加载包含已注册类的库时，静态变量的初始化强制调用结构构造函数，所有导出类都会自动注册。
// (When the plugin system loads a library with registered classes in it, the initialization of static variables forces the invocation of the struct constructors, and all exported classes are automatically registerd.)

#endif // CLASS_LOADER__REGISTER_MACRO_HPP_
