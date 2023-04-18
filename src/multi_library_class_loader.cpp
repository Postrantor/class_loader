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

#include "class_loader/multi_library_class_loader.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace class_loader
{

  /**
   * @brief 用于存储类加载器指针的向量类型别名 (A typedef for a vector of shared pointers to class_loader::ClassLoader objects)
   */
  typedef std::vector<std::shared_ptr<class_loader::ClassLoader>> ClassLoaderPtrVector;

  /**
   * @class ClassLoaderDependency
   * @brief 类加载器依赖项，确保在销毁静态变量之前销毁类加载器指针 (Class Loader Dependency, ensures the destruction order of static variables and class loader pointers)
   */
  class ClassLoaderDependency
  {
  protected:
    /**
     * @brief 默认构造函数 (Default constructor)
     */
    ClassLoaderDependency()
    {
      // 确保`ClassLoader`中的静态变量在`class_loader_ptrs_`（定义为`getClassLoaderPtrVectorImpl`中的静态实例）之后销毁
      // (Ensure the static variable in `ClassLoader` is destroyed after `class_loader_ptrs_`, which is a member of ClassLoaderPtrVectorImpl defined as a static instance in the `getClassLoaderPtrVectorImpl`)
      class_loader::impl::getLoadedLibraryVectorMutex();
      class_loader::impl::getPluginBaseToFactoryMapMapMutex();
      class_loader::impl::getGlobalPluginBaseToFactoryMapMap();
      class_loader::impl::getMetaObjectGraveyard();
      class_loader::impl::getLoadedLibraryVector();
      class_loader::impl::getCurrentlyLoadingLibraryName();
      class_loader::impl::getCurrentlyActiveClassLoader();
      class_loader::impl::hasANonPurePluginLibraryBeenOpened();
    }
  };

  /**
   * @class ClassLoaderPtrVectorImpl
   * @brief 类加载器指针向量实现类，继承自ClassLoaderDependency (Class Loader Pointer Vector implementation class, inherits from ClassLoaderDependency)
   */
  class ClassLoaderPtrVectorImpl : public ClassLoaderDependency
  {
  public:
    // 类加载器指针向量 (Class Loader Pointer Vector)
    ClassLoaderPtrVector class_loader_ptrs_;
    // 用于保护类加载器指针向量的互斥锁 (Mutex for protecting the class loader pointer vector)
    std::mutex loader_mutex_;
  };

  /**
   * @class MultiLibraryClassLoaderImpl
   * @brief 多库类加载器实现类 (Multi-library Class Loader implementation class)
   */
  class MultiLibraryClassLoaderImpl
  {
  public:
    // 是否启用按需加载和卸载功能 (Whether to enable on-demand load/unload functionality)
    bool enable_ondemand_loadunload_;
    // 存储活动类加载器的映射 (Mapping of active class loaders)
    LibraryToClassLoaderMap active_class_loaders_;
  };

  /**
   * @brief 获取 ClassLoaderPtrVectorImpl 实例的引用。
   * @return 返回一个 ClassLoaderPtrVectorImpl 类型的引用。
   *        Get a reference to the ClassLoaderPtrVectorImpl instance.
   * @return A reference of type ClassLoaderPtrVectorImpl.
   */
  ClassLoaderPtrVectorImpl &getClassLoaderPtrVectorImpl()
  {
    // 创建一个静态实例
    // Create a static instance
    static ClassLoaderPtrVectorImpl instance;
    // 返回静态实例的引用
    // Return the reference to the static instance
    return instance;
  }

  /**
   * @brief MultiLibraryClassLoader 构造函数
   * @param enable_ondemand_loadunload 是否启用按需加载和卸载。
   *        MultiLibraryClassLoader constructor.
   * @param enable_ondemand_loadunload Whether to enable on-demand loading and unloading.
   */
  MultiLibraryClassLoader::MultiLibraryClassLoader(bool enable_ondemand_loadunload)
      : impl_(new MultiLibraryClassLoaderImpl())
  {
    // 设置按需加载和卸载的开关
    // Set the switch for on-demand loading and unloading
    impl_->enable_ondemand_loadunload_ = enable_ondemand_loadunload;
  }

  /**
   * @brief MultiLibraryClassLoader 析构函数
   *        MultiLibraryClassLoader destructor.
   */
  MultiLibraryClassLoader::~MultiLibraryClassLoader()
  {
    // 关闭所有的类加载器
    // Shutdown all class loaders
    shutdownAllClassLoaders();
    // 删除实现对象
    // Delete the implementation object
    delete impl_;
  }

  /**
   * @brief 获取已注册的库列表
   * @return 返回一个包含已注册库名的字符串向量。
   *        Get the list of registered libraries.
   * @return A vector of strings containing the names of registered libraries.
   */
  std::vector<std::string> MultiLibraryClassLoader::getRegisteredLibraries() const
  {
    // 创建一个字符串向量用于存储库名
    // Create a string vector to store library names
    std::vector<std::string> libraries;
    // 遍历活动的类加载器
    // Iterate through active class loaders
    for (auto &it : impl_->active_class_loaders_)
    {
      // 如果类加载器不为空，则将库名添加到库列表中
      // If the class loader is not null, add the library name to the library list
      if (it.second != nullptr)
      {
        libraries.push_back(it.first);
      }
    }
    return libraries;
  }

  /**
   * @brief 获取指定库的类加载器
   * @param library_path 要查询的库的路径。
   * @return 返回一个指向 ClassLoader 的指针。
   *        Get the class loader for the specified library.
   * @param library_path The path of the library to query.
   * @return A pointer to a ClassLoader.
   */
  ClassLoader *MultiLibraryClassLoader::getClassLoaderForLibrary(const std::string &library_path)
  {
    // 返回指定库的类加载器
    // Return the class loader for the specified library
    return impl_->active_class_loaders_[library_path];
  }

  /**
   * @brief 获取所有可用的类加载器
   * @return 返回一个包含所有可用类加载器的向量。
   *        Get all available class loaders.
   * @return A vector containing all available class loaders.
   */
  ClassLoaderVector MultiLibraryClassLoader::getAllAvailableClassLoaders() const
  {
    // 创建一个类加载器向量
    // Create a class loader vector
    ClassLoaderVector loaders;
    // 遍历活动的类加载器
    // Iterate through active class loaders
    for (auto &it : impl_->active_class_loaders_)
    {
      // 将类加载器添加到类加载器向量中
      // Add the class loader to the class loader vector
      loaders.push_back(it.second);
    }
    return loaders;
  }

  /**
   * @brief 检查库是否可用 (Check if the library is available)
   *
   * @param library_name 要检查的库名称 (The name of the library to check)
   * @return 如果库可用，返回 true；否则返回 false (Returns true if the library is available, false otherwise)
   */
  bool MultiLibraryClassLoader::isLibraryAvailable(const std::string &library_name) const
  {
    // 获取已注册库的列表 (Get the list of registered libraries)
    std::vector<std::string> available_libraries = getRegisteredLibraries();

    // 在列表中查找指定的库名称，如果找到，则返回 true (Search for the specified library name in the list, return true if found)
    return available_libraries.end() != std::find(
                                            available_libraries.begin(), available_libraries.end(), library_name);
  }

  /**
   * @brief 加载库 (Load the library)
   *
   * @param library_path 要加载的库的路径 (The path of the library to load)
   */
  void MultiLibraryClassLoader::loadLibrary(const std::string &library_path)
  {
    // 检查库是否可用 (Check if the library is available)
    if (!isLibraryAvailable(library_path))
    {
      // 锁定互斥体以确保线程安全 (Lock the mutex to ensure thread safety)
      std::lock_guard<std::mutex> lock(getClassLoaderPtrVectorImpl().loader_mutex_);

      // 创建一个新的 ClassLoader 实例并添加到 class_loader_ptrs_ 中 (Create a new ClassLoader instance and add it to class_loader_ptrs_)
      getClassLoaderPtrVectorImpl().class_loader_ptrs_.emplace_back(
          std::make_shared<class_loader::ClassLoader>(library_path, isOnDemandLoadUnloadEnabled()));

      // 将新创建的 ClassLoader 实例添加到 active_class_loaders_ 中 (Add the newly created ClassLoader instance to active_class_loaders_)
      impl_->active_class_loaders_[library_path] =
          getClassLoaderPtrVectorImpl().class_loader_ptrs_.back().get();
    }
  }

  /**
   * @brief 关闭所有类加载器 (Shutdown all class loaders)
   */
  void MultiLibraryClassLoader::shutdownAllClassLoaders()
  {
    // 遍历已注册库列表 (Iterate through the list of registered libraries)
    for (auto &library_path : getRegisteredLibraries())
    {
      // 卸载库 (Unload the library)
      unloadLibrary(library_path);
    }
  }

  /**
   * @brief 卸载库 (Unload the library)
   *
   * @param library_path 要卸载的库的路径 (The path of the library to unload)
   * @return 剩余的卸载操作数 (The number of remaining unload operations)
   */
  int MultiLibraryClassLoader::unloadLibrary(const std::string &library_path)
  {
    int remaining_unloads = 0;

    // 检查库是否可用 (Check if the library is available)
    if (isLibraryAvailable(library_path))
    {
      // 获取该库的类加载器 (Get the class loader for the library)
      ClassLoader *loader = getClassLoaderForLibrary(library_path);

      // 卸载库并获取剩余的卸载操作数 (Unload the library and get the number of remaining unload operations)
      remaining_unloads = loader->unloadLibrary();

      // 如果没有剩余的卸载操作，则从 active_class_loaders_ 中移除该类加载器 (If there are no remaining unload operations, remove the class loader from active_class_loaders_)
      if (remaining_unloads == 0)
      {
        impl_->active_class_loaders_[library_path] = nullptr;

        // 锁定互斥体以确保线程安全 (Lock the mutex to ensure thread safety)
        std::lock_guard<std::mutex> lock(getClassLoaderPtrVectorImpl().loader_mutex_);

        // 从 class_loader_ptrs_ 中移除该类加载器 (Remove the class loader from class_loader_ptrs_)
        auto &class_loader_ptrs = getClassLoaderPtrVectorImpl().class_loader_ptrs_;
        for (auto iter = class_loader_ptrs.begin(); iter != class_loader_ptrs.end(); ++iter)
        {
          if (iter->get() == loader)
          {
            class_loader_ptrs.erase(iter);
            break;
          }
        }
      }
    }

    return remaining_unloads;
  }

  /**
   * @brief 检查是否启用了按需加载和卸载 (Check if on-demand loading and unloading is enabled)
   *
   * @return 如果启用了按需加载和卸载，则返回 true；否则返回 false (Returns true if on-demand loading and unloading is enabled, false otherwise)
   */
  bool MultiLibraryClassLoader::isOnDemandLoadUnloadEnabled() const
  {
    return impl_->enable_ondemand_loadunload_;
  }

} // namespace class_loader
