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
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
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

#include "class_loader/class_loader.hpp"

#include <string>

#include "Poco/SharedLibrary.h"

namespace class_loader
{

  /**
   * @brief 静态变量，表示是否已创建未托管实例。
   * @brief Static variable indicating whether an unmanaged instance has been created.
   */
  bool ClassLoader::has_unmananged_instance_been_created_ = false;

  /**
   * @brief 查询是否已创建未托管实例。
   * @brief Check if an unmanaged instance has been created.
   *
   * @return 返回是否已创建未托管实例的布尔值。
   * @return A boolean value indicating whether an unmanaged instance has been created.
   */
  bool ClassLoader::hasUnmanagedInstanceBeenCreated()
  {
    return ClassLoader::has_unmananged_instance_been_created_;
  }

  /**
   * @brief 设置未托管实例的创建状态。
   * @brief Set the creation state of the unmanaged instance.
   *
   * @param state 布尔值，表示未托管实例是否已创建。
   * @param state A boolean value indicating whether the unmanaged instance has been created.
   */
  void ClassLoader::setUnmanagedInstanceBeenCreated(bool state)
  {
    ClassLoader::has_unmananged_instance_been_created_ = state;
  }

  /**
   * @brief 获取系统库格式名称。
   * @brief Get the system library format name.
   *
   * @param library_name 库名称字符串。
   * @param library_name String containing the library name.
   * @return 返回格式化后的系统库名称。
   * @return The formatted system library name.
   */
  std::string systemLibraryFormat(const std::string &library_name)
  {
    return rcpputils::get_platform_library_name(library_name);
  }

  /**
   * @brief 构造一个新的ClassLoader实例。
   * @brief Construct a new ClassLoader instance.
   *
   * @param library_path 库路径字符串。
   * @param ondemand_load_unload 布尔值，表示是否启用按需加载和卸载。
   * @param library_path String containing the library path.
   * @param ondemand_load_unload A boolean value indicating whether to enable on-demand loading and unloading.
   */
  ClassLoader::ClassLoader(const std::string &library_path, bool ondemand_load_unload)
      : ondemand_load_unload_(ondemand_load_unload),
        library_path_(library_path),
        load_ref_count_(0),
        plugin_ref_count_(0)
  {
    CONSOLE_BRIDGE_logDebug(
        "class_loader.ClassLoader: "
        "Constructing new ClassLoader (%p) bound to library %s.",
        this, library_path.c_str());
    if (!isOnDemandLoadUnloadEnabled())
    {
      loadLibrary();
    }
  }

  /**
   * @brief 销毁ClassLoader实例并卸载关联的库。
   * @brief Destroy the ClassLoader instance and unload the associated library.
   */
  ClassLoader::~ClassLoader()
  {
    CONSOLE_BRIDGE_logDebug(
        "%s",
        "class_loader.ClassLoader: Destroying class loader, "
        "unloading associated library...\n");
    unloadLibrary(); // TODO(mikaelarguedas): while(unloadLibrary() > 0){} ??
  }

  /**
   * @brief 获取库的路径 (Get the library path)
   *
   * @return std::string& 库的路径 (The library path)
   */
  const std::string &ClassLoader::getLibraryPath() const
  {
    // 返回库路径成员变量 (Return the library path member variable)
    return library_path_;
  }

  /**
   * @brief 检查库是否已被加载 (Check if the library is loaded)
   *
   * @return bool 是否已加载 (Whether it is loaded or not)
   */
  bool ClassLoader::isLibraryLoaded() const
  {
    // 调用内部实现方法检查库是否已加载 (Call the internal implementation method to check if the library is loaded)
    return class_loader::impl::isLibraryLoaded(getLibraryPath(), this);
  }

  /**
   * @brief 检查库是否已被任意类加载器加载 (Check if the library is loaded by any classloader)
   *
   * @return bool 是否已被任意类加载器加载 (Whether it is loaded by any classloader or not)
   */
  bool ClassLoader::isLibraryLoadedByAnyClassloader() const
  {
    // 调用内部实现方法检查库是否已被任意类加载器加载 (Call the internal implementation method to check if the library is loaded by any classloader)
    return class_loader::impl::isLibraryLoadedByAnybody(getLibraryPath());
  }

  /**
   * @brief 检查是否启用了按需加载和卸载功能 (Check if on-demand load/unload is enabled)
   *
   * @return bool 是否启用了按需加载和卸载 (Whether on-demand load/unload is enabled or not)
   */
  bool ClassLoader::isOnDemandLoadUnloadEnabled() const
  {
    // 返回按需加载和卸载成员变量的值 (Return the value of the on-demand load/unload member variable)
    return ondemand_load_unload_;
  }

  /**
   * @brief 加载库 (Load the library)
   */
  void ClassLoader::loadLibrary()
  {
    // 如果库路径为空字符串 (If the library path is an empty string)
    if (getLibraryPath() == "")
    {
      // 特殊库路径，用于在链接时链接的库（而不是 dlopen-ed）(Special library path for libraries linked at link-time (not dlopen-ed))
      return;
    }
    // 对加载引用计数互斥锁进行加锁 (Lock the load reference count mutex)
    std::lock_guard<std::recursive_mutex> lock(load_ref_count_mutex_);
    // 增加加载引用计数 (Increase the load reference count)
    ++load_ref_count_;
    // 调用内部实现方法加载库 (Call the internal implementation method to load the library)
    class_loader::impl::loadLibrary(getLibraryPath(), this);
  }

  /**
   * @brief 卸载库 (Unload the library)
   *
   * @return int 卸载结果，0表示成功 (Unload result, 0 indicates success)
   */
  int ClassLoader::unloadLibrary()
  {
    // 如果库路径为空字符串 (If the library path is an empty string)
    if (getLibraryPath() == "")
    {
      // 特殊库路径，用于在链接时链接的库（而不是 dlopen-ed）(Special library path for libraries linked at link-time (not dlopen-ed))
      return 0;
    }
    // 调用内部方法卸载库 (Call the internal method to unload the library)
    return unloadLibraryInternal(true);
  }

  /**
   * @brief 卸载库的内部实现 (Unload the library's internal implementation)
   *
   * @param lock_plugin_ref_count 是否锁定插件引用计数 (Whether to lock the plugin reference count)
   * @return int 返回加载引用计数 (Return the load reference count)
   */
  int ClassLoader::unloadLibraryInternal(bool lock_plugin_ref_count)
  {
    // 如果需要锁定插件引用计数，则进行加锁操作 (If you need to lock the plugin reference count, perform a lock operation)
    if (lock_plugin_ref_count)
    {
      plugin_ref_count_mutex_.lock();
    }
    // 使用 std::lock_guard 对递归互斥量进行保护 (Protect the recursive mutex with std::lock_guard)
    std::lock_guard<std::recursive_mutex> load_ref_lock(load_ref_count_mutex_);

    try
    {
      // 检查插件引用计数是否大于0 (Check if the plugin reference count is greater than 0)
      if (plugin_ref_count_ > 0)
      {
        // 输出警告信息，提示用户在卸载库或销毁类加载器之前删除对象 (Output a warning message, prompting the user to delete objects before unloading the library or destroying the class loader)
        CONSOLE_BRIDGE_logWarn(
            "%s",
            "class_loader.ClassLoader: SEVERE WARNING!!! "
            "Attempting to unload library while objects created by this loader exist in the heap! "
            "You should delete your objects before attempting to unload the library or destroying "
            "the ClassLoader. The library will NOT be unloaded.");
      }
      else
      {
        // 减少加载引用计数 (Reduce the load reference count)
        load_ref_count_ = load_ref_count_ - 1;
        // 如果加载引用计数为0，则卸载库 (If the load reference count is 0, unload the library)
        if (load_ref_count_ == 0)
        {
          class_loader::impl::unloadLibrary(getLibraryPath(), this);
        }
        // 如果加载引用计数小于0，则将其重置为0 (If the load reference count is less than 0, reset it to 0)
        else if (load_ref_count_ < 0)
        {
          load_ref_count_ = 0;
        }
      }
    }
    catch (...)
    {
      // 如果发生异常并且需要锁定插件引用计数，则进行解锁操作 (If an exception occurs and the plugin reference count needs to be locked, perform an unlock operation)
      if (lock_plugin_ref_count)
      {
        plugin_ref_count_mutex_.unlock();
      }
      throw;
    }
    // 如果需要锁定插件引用计数，则进行解锁操作 (If you need to lock the plugin reference count, perform an unlock operation)
    if (lock_plugin_ref_count)
    {
      plugin_ref_count_mutex_.unlock();
    }
    // 返回加载引用计数 (Return the load reference count)
    return load_ref_count_;
  }

} // namespace class_loader
