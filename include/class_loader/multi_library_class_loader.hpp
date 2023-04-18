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

#ifndef CLASS_LOADER__MULTI_LIBRARY_CLASS_LOADER_HPP_
#define CLASS_LOADER__MULTI_LIBRARY_CLASS_LOADER_HPP_

#include <boost/thread.hpp>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

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

#include "class_loader/class_loader.hpp"
#include "class_loader/visibility_control.hpp"

namespace class_loader
{

  typedef std::string LibraryPath;
  typedef std::map<LibraryPath, class_loader::ClassLoader *> LibraryToClassLoaderMap;
  typedef std::vector<ClassLoader *> ClassLoaderVector;

  class MultiLibraryClassLoaderImpl;

  /**
   * @class MultiLibraryClassLoader
   * @brief 一个可以绑定多个运行时库的 ClassLoader 类 (A ClassLoader that can bind more than one runtime library)
   */
  class CLASS_LOADER_PUBLIC MultiLibraryClassLoader
  {
  public:
    /**
     * @brief 类的构造函数 (Constructor for the class)
     *
     * @param enable_ondemand_loadunload - 表示在创建和销毁 class_loader 时，类是否自动加载/卸载的标志 (Flag indicates if classes are to be loaded/unloaded automatically as class_loader are created and destroyed)
     */
    explicit MultiLibraryClassLoader(bool enable_ondemand_loadunload);

    /**
     * @brief 虚拟析构函数
     * @brief Virtual destructor for the class
     */
    virtual ~MultiLibraryClassLoader();

    /**
     * @brief 用给定的基类 Base 创建一个指定类名对象的实例
     * @brief Creates an instance of an object of given class name with ancestor class Base
     * 这个版本不会在特定的库中寻找工厂，而是在第一个定义该类的打开的库中查找
     * This version does not look in a specific library for the factory, but rather the first open
     * library that defines the class
     *
     * @tparam Base - 表示基类的多态类型
     * @tparam Base - polymorphic type indicating base class
     * @param class_name - 我们要实例化的具体插件类的名称
     * @param class_name - the name of the concrete plugin class we want to instantiate
     * @return 一个指向新创建的插件的 std::shared_ptr<Base>
     * @return A std::shared_ptr<Base> to newly created plugin
     */
    template <class Base>
    std::shared_ptr<Base> createInstance(const std::string &class_name)
    {
      // 打印调试信息，尝试创建类类型为 %s 的实例
      // Log debug message, attempting to create an instance of class type %s
      CONSOLE_BRIDGE_logDebug(
          "class_loader::MultiLibraryClassLoader: "
          "Attempting to create instance of class type %s.",
          class_name.c_str());

      // 获取负责创建指定类名实例的类加载器
      // Get the ClassLoader responsible for creating instances of the specified class name
      ClassLoader *loader = getClassLoaderForClass<Base>(class_name);

      // 如果没有找到类加载器，则抛出异常
      // If no ClassLoader is found, throw an exception
      if (nullptr == loader)
      {
        throw class_loader::CreateClassException(
            "MultiLibraryClassLoader: Could not create object of class type " +
            class_name +
            " as no factory exists for it. Make sure that the library exists and " +
            "was explicitly loaded through MultiLibraryClassLoader::loadLibrary()");
      }

      // 使用找到的类加载器创建实例
      // Create the instance using the found ClassLoader
      return loader->createInstance<Base>(class_name);
    }

    /**
     * @brief 创建具有给定类名的对象实例，该对象具有祖先类 Base
     * 这个版本采用特定库来明确使用的工厂
     * Creates an instance of an object of given class name with ancestor class Base
     * This version takes a specific library to make explicit the factory being used
     *
     * @tparam Base - 多态类型表示基类
     * @param Base - polymorphic type indicating base class
     * @param class_name - 我们要实例化的具体插件类的名称
     * @param class_name - the name of the concrete plugin class we want to instantiate
     * @param library_path - 我们想要从中创建插件的库路径
     * @param library_path - the library from which we want to create the plugin
     * @return 一个指向新创建插件的 std::shared_ptr<Base>
     * @return A std::shared_ptr<Base> to newly created plugin
     */
    template <class Base>
    std::shared_ptr<Base> createInstance(
        const std::string &class_name, const std::string &library_path)
    {
      // 获取与 library_path 绑定的 ClassLoader
      // Get the ClassLoader associated with the library_path
      ClassLoader *loader = getClassLoaderForLibrary(library_path);

      // 如果找不到对应的 ClassLoader
      // If no ClassLoader is found
      if (nullptr == loader)
      {
        // 抛出 NoClassLoaderExistsException 异常
        // Throw a NoClassLoaderExistsException exception
        throw class_loader::NoClassLoaderExistsException(
            "Could not create instance as there is no ClassLoader in "
            "MultiLibraryClassLoader bound to library " +
            library_path +
            " Ensure you called MultiLibraryClassLoader::loadLibrary()");
      }

      // 使用找到的 ClassLoader 创建实例
      // Create an instance using the found ClassLoader
      return loader->createInstance<Base>(class_name);
    }

    /// 创建具有给定类名的对象实例，其祖先类为 Base
    /// Creates an instance of an object of given class name with ancestor class Base
    /**
     * 此版本不会在特定库中查找工厂，而是查找第一个定义该类的已打开库
     * This version does not look in a specific library for the factory, but rather the first open
     * library that defines the class
     *
     * @param Base - 表示基类的多态类型
     * @param Base - polymorphic type indicating base class
     * @param class_name - 我们要实例化的具体插件类的名称
     * @param class_name - the name of the concrete plugin class we want to instantiate
     * @return 一个指向新创建插件的唯一指针
     * @return A unique pointer to newly created plugin
     */
    template <class Base>
    ClassLoader::UniquePtr<Base> createUniqueInstance(const std::string &class_name)
    {
      // 记录调试信息：尝试创建类类型 %s 的实例
      // Log debug info: Attempting to create instance of class type %s.
      CONSOLE_BRIDGE_logDebug(
          "class_loader::MultiLibraryClassLoader: Attempting to create instance of class type %s.",
          class_name.c_str());

      // 获取类的类加载器
      // Get class loader for the class
      ClassLoader *loader = getClassLoaderForClass<Base>(class_name);

      // 如果类加载器为空，抛出异常
      // If class loader is null, throw an exception
      if (nullptr == loader)
      {
        throw class_loader::CreateClassException(
            "MultiLibraryClassLoader: Could not create object of class type " + class_name +
            " as no factory exists for it. "
            "Make sure that the library exists and was explicitly loaded through "
            "MultiLibraryClassLoader::loadLibrary()");
      }

      // 使用类加载器创建唯一实例
      // Create unique instance using the class loader
      return loader->createUniqueInstance<Base>(class_name);
    }

    /// 创建一个具有给定类名的对象实例，该对象具有基类 Base
    /// Creates an instance of an object of given class name with ancestor class Base
    /**
     * 本版本使用特定库来明确使用的工厂
     * This version takes a specific library to make explicit the factory being used
     *
     * @tparam Base - 表示基类的多态类型
     * @param Base - polymorphic type indicating base class
     * @param class_name - 我们要实例化的具体插件类的名称
     * @param class_name - the name of the concrete plugin class we want to instantiate
     * @param library_path - 我们想要创建插件的库
     * @param library_path - the library from which we want to create the plugin
     * @return 指向新创建的插件的独特指针
     * @return A unique pointer to newly created plugin
     */
    template <class Base>
    ClassLoader::UniquePtr<Base>
    createUniqueInstance(const std::string &class_name, const std::string &library_path)
    {
      // 获取绑定到库的类加载器
      // Get the class loader bound to the library
      ClassLoader *loader = getClassLoaderForLibrary(library_path);
      if (nullptr == loader)
      {
        // 如果没有类加载器，则抛出异常
        // Throw exception if there is no class loader
        throw class_loader::NoClassLoaderExistsException(
            "Could not create instance as there is no ClassLoader in "
            "MultiLibraryClassLoader bound to library " +
            library_path +
            " Ensure you called MultiLibraryClassLoader::loadLibrary()");
      }
      // 使用类加载器创建唯一实例
      // Create unique instance using the class loader
      return loader->createUniqueInstance<Base>(class_name);
    }

    /// 创建一个具有给定类名的对象实例，该对象具有基类 Base
    /// Creates an instance of an object of given class name with ancestor class Base
    /**
     * 这个版本不会在特定库中查找工厂，而是在定义类的第一个打开的库中查找
     * This version does not look in a specific library for the factory, but rather the first open
     * library that defines the class
     * 插件系统无法进行自动安全加载/卸载，因此不应使用此版本
     * This version should not be used as the plugin system cannot do automated safe loading/unloadings
     *
     * @tparam Base - 表示基类的多态类型
     * @param Base - polymorphic type indicating base class
     * @param class_name - 我们要实例化的具体插件类的名称
     * @param class_name - the name of the concrete plugin class we want to instantiate
     * @return 指向新创建的插件的未托管的 Base*
     * @return An unmanaged Base* to newly created plugin
     */
    template <class Base>
    Base *createUnmanagedInstance(const std::string &class_name)
    {
      // 获取绑定到类的类加载器
      // Get the class loader bound to the class
      ClassLoader *loader = getClassLoaderForClass<Base>(class_name);
      if (nullptr == loader)
      {
        // 如果没有类加载器，则抛出异常
        // Throw exception if there is no class loader
        throw class_loader::CreateClassException(
            "MultiLibraryClassLoader: Could not create class of type " + class_name);
      }
      // 使用类加载器创建未托管实例
      // Create unmanaged instance using the class loader
      return loader->createUnmanagedInstance<Base>(class_name);
    }

    /**
     * @brief 创建具有给定类名的对象实例，其祖先类为 Base
     *        这个版本需要指定特定的库，以明确使用的工厂
     *        不建议使用此版本，因为插件系统无法进行自动安全的加载/卸载操作
     *
     * @brief Creates an instance of an object of given class name with ancestor class Base
     *        This version takes a specific library to make explicit the factory being used
     *        This version should not be used as the plugin system cannot do automated safe loading/unloadings
     *
     * @param Base - 表示基类的多态类型
     * @param Base - polymorphic type indicating Base class
     * @param class_name - 我们想要创建实例的类名
     * @param class_name - name of class for which we want to create instance
     * @param library_path - 运行时库的完全限定路径
     * @param library_path - the fully qualified path to the runtime library
     */
    template <class Base>
    Base *createUnmanagedInstance(const std::string &class_name, const std::string &library_path)
    {
      // 获取指定库的类加载器
      // Get the ClassLoader for the specified library
      ClassLoader *loader = getClassLoaderForLibrary(library_path);

      // 如果没有找到类加载器，则抛出异常
      // If no ClassLoader is found, throw an exception
      if (nullptr == loader)
      {
        throw class_loader::NoClassLoaderExistsException(
            "Could not create instance as there is no ClassLoader in MultiLibraryClassLoader "
            "bound to library " +
            library_path +
            " Ensure you called MultiLibraryClassLoader::loadLibrary()");
      }

      // 使用找到的类加载器创建未托管实例
      // Create an unmanaged instance using the found ClassLoader
      return loader->createUnmanagedInstance<Base>(class_name);
    }

    /**
     * @brief 指示一个类是否已加载并可以实例化
     *
     * @brief Indicates if a class has been loaded and can be instantiated
     *
     * @param Base - 表示基类的多态类型
     * @param Base - polymorphic type indicating Base class
     * @param class_name - 正在查询的类名
     * @param class_name - name of class that is be inquired about
     * @return 如果已加载，则返回 true，否则返回 false
     * @return true if loaded, false otherwise
     */
    template <class Base>
    bool isClassAvailable(const std::string &class_name) const
    {
      // 获取可用类的列表
      // Get the list of available classes
      std::vector<std::string> available_classes = getAvailableClasses<Base>();

      // 检查类名是否在可用类列表中
      // Check if the class name exists in the list of available classes
      return available_classes.end() != std::find(
                                            available_classes.begin(), available_classes.end(), class_name);
    }

    /**
     * @brief 指示一个库是否已加载到内存中 (Indicates if a library has been loaded into memory)
     *
     * @param library_path 完全限定的运行时库路径 (The full qualified path to the runtime library)
     * @return 如果库已加载，则返回 true，否则返回 false (true if library is loaded, false otherwise)
     */
    bool isLibraryAvailable(const std::string &library_path) const;

    /**
     * @brief 获取类加载器加载的所有类的列表 (Gets a list of all classes that are loaded by the class loader)
     *
     * @tparam Base 表示基类的多态类型 (polymorphic type indicating Base class)
     * @return 一个包含可用类的字符串向量 (A vector<string> of the available classes)
     */
    template <class Base>
    std::vector<std::string> getAvailableClasses() const
    {
      // 创建一个字符串向量来存储可用的类名
      // (Create a string vector to store the available class names)
      std::vector<std::string> available_classes;

      // 遍历所有可用的类加载器
      // (Iterate through all available class loaders)
      for (auto &loader : getAllAvailableClassLoaders())
      {
        // 使用当前类加载器获取基类的可用类名列表
        // (Get the list of available class names for the Base class using the current class loader)
        std::vector<std::string> loader_classes = loader->getAvailableClasses<Base>();

        // 将当前类加载器的类名列表插入到最终结果的末尾
        // (Insert the class names from the current class loader to the end of the final result)
        available_classes.insert(
            available_classes.end(), loader_classes.begin(), loader_classes.end());
      }

      // 返回包含所有可用类名的字符串向量
      // (Return the string vector containing all available class names)
      return available_classes;
    }

    /**
     * @brief 获取特定库中加载的所有类的列表 (Gets a list of all classes loaded for a particular library)
     *
     * @tparam Base - 多态类型，表示基类 (Polymorphic type indicating Base class)
     * @param library_path 库的路径 (The path of the library)
     * @return 一个字符串向量，包含传入库中可用的类 (A vector<string> of the available classes in the passed library)
     */
    template <class Base>
    std::vector<std::string> getAvailableClassesForLibrary(const std::string &library_path)
    {
      // 获取与库关联的类加载器 (Get the ClassLoader associated with the library)
      ClassLoader *loader = getClassLoaderForLibrary(library_path);
      // 如果加载器为空，则抛出异常 (If the loader is nullptr, throw an exception)
      if (nullptr == loader)
      {
        throw class_loader::NoClassLoaderExistsException(
            "There is no ClassLoader in MultiLibraryClassLoader bound to library " +
            library_path +
            " Ensure you called MultiLibraryClassLoader::loadLibrary()");
      }
      // 返回库中可用类的列表 (Return the list of available classes in the library)
      return loader->getAvailableClasses<Base>();
    }

    /**
     * @brief 获取此类加载器打开的所有库的列表 (Gets a list of all libraries opened by this class loader)
     *
     * @return 此类加载器打开的库的列表 (A list of libraries opened by this class loader)
     */
    std::vector<std::string> getRegisteredLibraries() const;

    /**
     * @brief 将库加载到此类加载器的内存中 (Loads a library into memory for this class loader)
     *
     * @param library_path - 运行时库的完全限定路径 (The fully qualified path to the runtime library)
     */
    void loadLibrary(const std::string &library_path);

    /**
     * @brief 卸载此类加载器的库 (Unloads a library for this class loader)
     *
     * @param library_path - 运行时库的完全限定路径 (The fully qualified path to the runtime library)
     * @return unloadLibrary() 需要再被调用几次才能从这个 MultiLibraryClassLoader 中解除绑定 (The number of times more unloadLibrary() has to be called for it to be unbound from this MultiLibraryClassLoader)
     */
    int unloadLibrary(const std::string &library_path);

  private:
    /**
     * @brief 表示是否启用按需（懒惰）加载/卸载，以便根据需要自动加载/卸载库 (Indicates if on-demand (lazy) load/unload is enabled so libraries are loaded/unloaded automatically as needed)
     *
     * @return 如果按需加载和卸载处于活动状态，则为 true；否则为 false (true if ondemand load and unload is active, otherwise false)
     */
    bool isOnDemandLoadUnloadEnabled() const;

    /**
     * @brief 获取与特定运行时库对应的类加载器句柄 (Gets a handle to the class loader corresponding to a specific runtime library)
     *
     * @param library_path - 我们要从中创建插件的库 (the library from which we want to create the plugin)
     * @return 指向 ClassLoader* 的指针，如果未找到则为 nullptr (A pointer to the ClassLoader*, == nullptr if not found)
     */
    ClassLoader *getClassLoaderForLibrary(const std::string &library_path);

    /// 获取与特定类对应的类加载器句柄。 (Gets a handle to the class loader corresponding to a specific class.)
    /**
     * @param class_name 我们要创建实例的类的名称。(name of class for which we want to create instance.)
     * @return 指向 ClassLoader 的指针，如果未找到则为 NULL。 (A pointer to the ClassLoader, or NULL if not found.)
     */
    template <typename Base>
    ClassLoader *getClassLoaderForClass(const std::string &class_name)
    {
      // 获取所有可用的类加载器 (Get all available class loaders)
      ClassLoaderVector loaders = getAllAvailableClassLoaders();
      // 遍历加载器向量 (Iterate through the loaders vector)
      for (ClassLoaderVector::iterator i = loaders.begin(); i != loaders.end(); ++i)
      {
        // 如果库未加载，则加载库 (If the library is not loaded, load it)
        if (!(*i)->isLibraryLoaded())
        {
          (*i)->loadLibrary();
        }
        // 如果类在当前加载器中可用，返回该加载器 (If class is available in the current loader, return that loader)
        if ((*i)->isClassAvailable<Base>(class_name))
        {
          return *i;
        }
      }
      // 如果找不到类加载器，返回 nullptr (Return nullptr if no class loader is found)
      return nullptr;
    }

    /**
     * @brief 获取范围内加载的所有类加载器 (Gets all class loaders loaded within scope)
     *
     * @return 包含可用 ClassLoader 指针的向量 (vector with available ClassLoader pointers)
     */
    ClassLoaderVector getAllAvailableClassLoaders() const;

    /**
     * @brief 销毁所有的 ClassLoaders (Destroys all ClassLoaders)
     */
    void shutdownAllClassLoaders();

    // MultiLibraryClassLoaderImpl 的实现指针 (Implementation pointer of MultiLibraryClassLoaderImpl)
    MultiLibraryClassLoaderImpl *impl_;
  };

} // namespace class_loader
#endif // CLASS_LOADER__MULTI_LIBRARY_CLASS_LOADER_HPP_
