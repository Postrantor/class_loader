/*
 * Software License Agreement (BSD License)
 *
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

#ifndef CLASS_LOADER__EXCEPTIONS_HPP_
#define CLASS_LOADER__EXCEPTIONS_HPP_

#include <stdexcept>
#include <string>

namespace class_loader
{

  /**
   * @class ClassLoaderException
   * @brief A base class for all class_loader exceptions that inherits from std::runtime_exception
   *
   * 类加载器异常基类，继承自 std::runtime_error
   */
  class ClassLoaderException : public std::runtime_error
  {
  public:
    /**
     * @brief Constructor of ClassLoaderException
     * @param error_desc Description of the error
     *
     * 类加载器异常构造函数
     * @param error_desc 错误描述
     */
    explicit inline ClassLoaderException(const std::string &error_desc)
        : std::runtime_error(error_desc)
    {
    }
  };

  /**
   * @class LibraryLoadException
   * @brief An exception class thrown when class_loader is unable to load a runtime library
   *
   * 当类加载器无法加载运行时库时抛出的异常类
   */
  class LibraryLoadException : public ClassLoaderException
  {
  public:
    /**
     * @brief Constructor of LibraryLoadException
     * @param error_desc Description of the error
     *
     * 库加载异常构造函数
     * @param error_desc 错误描述
     */
    explicit inline LibraryLoadException(const std::string &error_desc)
        : ClassLoaderException(error_desc)
    {
    }
  };

  /**
   * @class LibraryUnloadException
   * @brief An exception class thrown when class_loader is unable to unload a runtime library
   *
   * 当类加载器无法卸载运行时库时抛出的异常类
   */
  class LibraryUnloadException : public ClassLoaderException
  {
  public:
    /**
     * @brief Constructor of LibraryUnloadException
     * @param error_desc Description of the error
     *
     * 库卸载异常构造函数
     * @param error_desc 错误描述
     */
    explicit inline LibraryUnloadException(const std::string &error_desc)
        : ClassLoaderException(error_desc)
    {
    }
  };

  /**
   * @class CreateClassException
   * @brief An exception class thrown when class_loader is unable to create a plugin
   *
   * 当类加载器无法创建插件时抛出的异常类
   */
  class CreateClassException : public ClassLoaderException
  {
  public:
    /**
     * @brief Constructor of CreateClassException
     * @param error_desc Description of the error
     *
     * 创建类异常构造函数
     * @param error_desc 错误描述
     */
    explicit inline CreateClassException(const std::string &error_desc)
        : ClassLoaderException(error_desc)
    {
    }
  };

  /**
   * @class NoClassLoaderExistsException
   * @brief An exception class thrown when a multilibrary class loader does not have a ClassLoader
   * bound to it
   *
   * 当多库类加载器没有绑定类加载器时抛出的异常类
   */
  class NoClassLoaderExistsException : public ClassLoaderException
  {
  public:
    /**
     * @brief Constructor of NoClassLoaderExistsException
     * @param error_desc Description of the error
     *
     * 无类加载器存在异常构造函数
     * @param error_desc 错误描述
     */
    explicit inline NoClassLoaderExistsException(const std::string &error_desc)
        : ClassLoaderException(error_desc)
    {
    }
  };

} // namespace class_loader
#endif // CLASS_LOADER__EXCEPTIONS_HPP_
