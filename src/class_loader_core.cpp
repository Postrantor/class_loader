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

#include "class_loader/class_loader_core.hpp"
#include "class_loader/class_loader.hpp"

#include <Poco/SharedLibrary.h>

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

namespace class_loader
{
  namespace impl
  {

    /**
     * @brief 获取已加载库向量的互斥锁 (Get the mutex for the loaded library vector)
     *
     * @return std::recursive_mutex& 互斥锁引用 (Reference to the mutex)
     */
    std::recursive_mutex &getLoadedLibraryVectorMutex()
    {
      // 定义一个静态互斥锁变量 m (Define a static mutex variable m)
      static std::recursive_mutex m;
      // 返回互斥锁引用 (Return reference to the mutex)
      return m;
    }

    /**
     * @brief 获取插件基类到工厂映射的互斥锁 (Get the mutex for the plugin base to factory map map)
     *
     * @return std::recursive_mutex& 互斥锁引用 (Reference to the mutex)
     */
    std::recursive_mutex &getPluginBaseToFactoryMapMapMutex()
    {
      // 定义一个静态互斥锁变量 m (Define a static mutex variable m)
      static std::recursive_mutex m;
      // 返回互斥锁引用 (Return reference to the mutex)
      return m;
    }

    /**
     * @brief 获取全局插件基类到工厂映射的映射 (Get the global plugin base to factory map map)
     *
     * @return BaseToFactoryMapMap& 映射引用 (Reference to the map map)
     */
    BaseToFactoryMapMap &getGlobalPluginBaseToFactoryMapMap()
    {
      // 定义一个静态 BaseToFactoryMapMap 实例 (Define a static BaseToFactoryMapMap instance)
      static BaseToFactoryMapMap instance;
      // 返回映射引用 (Return reference to the map map)
      return instance;
    }

    /**
     * @brief 根据基类名称获取工厂映射 (Get the factory map for the base class by its name)
     *
     * @param typeid_base_class_name 基类名称 (Base class name)
     * @return FactoryMap& 工厂映射引用 (Reference to the factory map)
     */
    FactoryMap &getFactoryMapForBaseClass(const std::string &typeid_base_class_name)
    {
      // 获取全局插件基类到工厂映射的映射 (Get the global plugin base to factory map map)
      BaseToFactoryMapMap &factoryMapMap = getGlobalPluginBaseToFactoryMapMap();
      // 将基类名称赋值给一个新的字符串变量 (Assign the base class name to a new string variable)
      std::string base_class_name = typeid_base_class_name;
      // 如果映射中找不到基类名称，则创建一个新的工厂映射 (If the base class name is not found in the map, create a new factory map)
      if (factoryMapMap.find(base_class_name) == factoryMapMap.end())
      {
        factoryMapMap[base_class_name] = FactoryMap();
      }

      // 返回基类名称对应的工厂映射引用 (Return reference to the factory map for the base class name)
      return factoryMapMap[base_class_name];
    }

    /**
     * @brief 获取元对象墓地向量 (Get the meta-object graveyard vector)
     *
     * @return MetaObjectGraveyardVector& 墓地向量引用 (Reference to the graveyard vector)
     */
    MetaObjectGraveyardVector &getMetaObjectGraveyard()
    {
      // 定义一个静态 MetaObjectGraveyardVector 实例 (Define a static MetaObjectGraveyardVector instance)
      static MetaObjectGraveyardVector instance;
      // 返回墓地向量引用 (Return reference to the graveyard vector)
      return instance;
    }

    /**
     * @brief 获取已加载库向量的引用
     * @return LibraryVector& 已加载库向量的引用
     *
     * @brief Get a reference to the loaded library vector
     * @return LibraryVector& A reference to the loaded library vector
     */
    LibraryVector &getLoadedLibraryVector()
    {
      // 创建一个静态库向量实例，以保证只有一个实例存在
      // Create a static library vector instance to ensure only one instance exists
      static LibraryVector instance;
      return instance;
    }

    /**
     * @brief 获取当前正在加载的库名称的引用
     * @return std::string& 当前正在加载的库名称的引用
     *
     * @brief Get a reference to the name of the currently loading library
     * @return std::string& A reference to the name of the currently loading library
     */
    std::string &getCurrentlyLoadingLibraryNameReference()
    {
      // 创建一个静态库名称字符串，以保证只有一个实例存在
      // Create a static library name string to ensure only one instance exists
      static std::string library_name;
      return library_name;
    }

    /**
     * @brief 获取当前正在加载的库名称
     * @return std::string 当前正在加载的库名称
     *
     * @brief Get the name of the currently loading library
     * @return std::string The name of the currently loading library
     */
    std::string getCurrentlyLoadingLibraryName()
    {
      // 返回当前正在加载的库名称的引用
      // Return the reference to the name of the currently loading library
      return getCurrentlyLoadingLibraryNameReference();
    }

    /**
     * @brief 设置当前正在加载的库名称
     * @param library_name 要设置的库名称
     *
     * @brief Set the name of the currently loading library
     * @param library_name The name of the library to set
     */
    void setCurrentlyLoadingLibraryName(const std::string &library_name)
    {
      // 获取当前正在加载的库名称的引用
      // Get the reference to the name of the currently loading library
      std::string &library_name_ref = getCurrentlyLoadingLibraryNameReference();
      // 设置库名称
      // Set the library name
      library_name_ref = library_name;
    }

    /**
     * @brief 获取当前活动的类加载器的引用
     * @return ClassLoader*& 当前活动的类加载器的引用
     *
     * @brief Get a reference to the currently active class loader
     * @return ClassLoader*& A reference to the currently active class loader
     */
    ClassLoader *&getCurrentlyActiveClassLoaderReference()
    {
      // 创建一个静态类加载器指针，初始值为nullptr，以保证只有一个实例存在
      // Create a static class loader pointer with initial value nullptr to ensure only one instance exists
      static ClassLoader *loader = nullptr;
      return loader;
    }

    /**
     * @brief 获取当前活动的类加载器
     * @return ClassLoader* 当前活动的类加载器
     *
     * @brief Get the currently active class loader
     * @return ClassLoader* The currently active class loader
     */
    ClassLoader *getCurrentlyActiveClassLoader()
    {
      // 返回当前活动的类加载器的引用
      // Return the reference to the currently active class loader
      return getCurrentlyActiveClassLoaderReference();
    }

    /**
     * @brief 设置当前活动的类加载器
     * @param loader 要设置的类加载器
     *
     * @brief Set the currently active class loader
     * @param loader The class loader to set
     */
    void setCurrentlyActiveClassLoader(ClassLoader *loader)
    {
      // 获取当前活动的类加载器的引用
      // Get the reference to the currently active class loader
      ClassLoader *&loader_ref = getCurrentlyActiveClassLoaderReference();
      // 设置类加载器
      // Set the class loader
      loader_ref = loader;
    }

    /**
     * @brief 获取一个表示是否打开过非纯插件库的布尔值引用
     * @return bool& 表示是否打开过非纯插件库的布尔值引用
     *
     * @brief Get a reference to a boolean indicating whether a non-pure plugin library has been opened
     * @return bool& A reference to a boolean indicating whether a non-pure plugin library has been opened
     */
    bool &hasANonPurePluginLibraryBeenOpenedReference()
    {
      // 创建一个静态布尔值，初始值为false，以保证只有一个实例存在
      // Create a static boolean with initial value false to ensure only one instance exists
      static bool hasANonPurePluginLibraryBeenOpenedReference = false;
      return hasANonPurePluginLibraryBeenOpenedReference;
    }

    /**
     * @brief 检查是否打开过非纯插件库
     * @return bool 如果打开过非纯插件库，则返回true，否则返回false
     *
     * @brief Check if a non-pure plugin library has been opened
     * @return bool Returns true if a non-pure plugin library has been opened, otherwise returns false
     */
    bool hasANonPurePluginLibraryBeenOpened()
    {
      // 返回表示是否打开过非纯插件库的布尔值引用
      // Return the reference to a boolean indicating whether a non-pure plugin library has been opened
      return hasANonPurePluginLibraryBeenOpenedReference();
    }

    /**
     * @brief 设置非纯插件库是否已打开的标志 (Set the flag for whether a non-pure plugin library has been opened)
     *
     * @param hasIt 是否已打开非纯插件库 (Whether a non-pure plugin library has been opened)
     */
    void hasANonPurePluginLibraryBeenOpened(bool hasIt)
    {
      // 设置非纯插件库是否已打开的引用值 (Set the reference value for whether a non-pure plugin library has been opened)
      hasANonPurePluginLibraryBeenOpenedReference() = hasIt;
    }

    // MetaObject 搜索/插入/移除/查询 (MetaObject search/insert/removal/query)

    /**
     * @brief 获取所有元对象 (Get all meta objects)
     *
     * @param factories 工厂映射 (Factory mapping)
     * @return MetaObjectVector 元对象向量 (Meta object vector)
     */
    MetaObjectVector allMetaObjects(const FactoryMap &factories)
    {
      // 创建一个空的元对象向量 (Create an empty meta object vector)
      MetaObjectVector all_meta_objs;

      // 遍历工厂映射 (Iterate through the factory mapping)
      for (
          FactoryMap::const_iterator factoryItr = factories.begin();
          factoryItr != factories.end(); factoryItr++)
      {
        // 将当前遍历到的工厂元对象添加到元对象向量中 (Add the current factory's meta object to the meta object vector)
        all_meta_objs.push_back(factoryItr->second);
      }

      // 返回所有元对象向量 (Return the meta object vector)
      return all_meta_objs;
    }

    /**
     * @brief 获取所有元对象 (Get all meta objects)
     *
     * @return MetaObjectVector 元对象向量 (Meta object vector)
     */
    MetaObjectVector allMetaObjects()
    {
      // 获取插件基础到工厂映射映射的互斥锁 (Get the mutex for the plugin base to factory map mapping)
      std::lock_guard<std::recursive_mutex> lock(getPluginBaseToFactoryMapMapMutex());

      // 创建一个空的元对象向量 (Create an empty meta object vector)
      MetaObjectVector all_meta_objs;

      // 获取全局插件基础到工厂映射映射 (Get the global plugin base to factory map mapping)
      BaseToFactoryMapMap &factory_map_map = getGlobalPluginBaseToFactoryMapMap();

      // 遍历插件基础到工厂映射映射 (Iterate through the plugin base to factory map mapping)
      for (auto &it : factory_map_map)
      {
        // 获取当前遍历到的插件基础的所有元对象 (Get all meta objects of the current plugin base)
        MetaObjectVector objs = allMetaObjects(it.second);

        // 将当前遍历到的插件基础的所有元对象添加到元对象向量中 (Add all meta objects of the current plugin base to the meta object vector)
        all_meta_objs.insert(all_meta_objs.end(), objs.begin(), objs.end());
      }

      // 返回所有元对象向量 (Return the meta object vector)
      return all_meta_objs;
    }

    /**
     * @brief 过滤出属于指定类加载器的元对象 (Filter out the meta objects that belong to the specified class loader)
     *
     * @param[in] to_filter 要过滤的元对象向量 (The vector of meta objects to be filtered)
     * @param[in] owner 指定的类加载器 (The specified class loader)
     * @return MetaObjectVector 过滤后的元对象向量 (Filtered vector of meta objects)
     */
    MetaObjectVector
    filterAllMetaObjectsOwnedBy(const MetaObjectVector &to_filter, const ClassLoader *owner)
    {
      // 创建一个空的元对象向量，用于存放过滤后的结果
      // (Create an empty meta object vector to store the filtered results)
      MetaObjectVector filtered_objs;

      // 遍历要过滤的元对象向量 (Iterate through the vector of meta objects to be filtered)
      for (auto &f : to_filter)
      {
        // 判断当前元对象是否属于指定的类加载器 (Check if the current meta object belongs to the specified class loader)
        if (f->isOwnedBy(owner))
        {
          // 如果属于，则将该元对象添加到过滤后的向量中 (If it belongs, add the meta object to the filtered vector)
          filtered_objs.push_back(f);
        }
      }

      // 返回过滤后的元对象向量 (Return the filtered vector of meta objects)
      return filtered_objs;
    }

    /**
     * @brief 过滤出与指定库关联的元对象 (Filter out the meta objects associated with the specified library)
     *
     * @param[in] to_filter 要过滤的元对象向量 (The vector of meta objects to be filtered)
     * @param[in] library_path 指定库的路径 (The path of the specified library)
     * @return MetaObjectVector 过滤后的元对象向量 (Filtered vector of meta objects)
     */
    MetaObjectVector
    filterAllMetaObjectsAssociatedWithLibrary(
        const MetaObjectVector &to_filter, const std::string &library_path)
    {
      // 创建一个空的元对象向量，用于存放过滤后的结果
      // (Create an empty meta object vector to store the filtered results)
      MetaObjectVector filtered_objs;

      // 遍历要过滤的元对象向量 (Iterate through the vector of meta objects to be filtered)
      for (auto &f : to_filter)
      {
        // 判断当前元对象是否与指定库关联 (Check if the current meta object is associated with the specified library)
        if (f->getAssociatedLibraryPath() == library_path)
        {
          // 如果关联，则将该元对象添加到过滤后的向量中 (If it's associated, add the meta object to the filtered vector)
          filtered_objs.push_back(f);
        }
      }

      // 返回过滤后的元对象向量 (Return the filtered vector of meta objects)
      return filtered_objs;
    }

    /**
     * @brief 获取指定类加载器的所有元对象 (Get all meta objects for the specified class loader)
     *
     * @param[in] owner 指定的类加载器 (The specified class loader)
     * @return MetaObjectVector 元对象向量 (Vector of meta objects)
     */
    MetaObjectVector
    allMetaObjectsForClassLoader(const ClassLoader *owner)
    {
      // 调用 filterAllMetaObjectsOwnedBy 函数，传入所有元对象和指定的类加载器
      // (Call the filterAllMetaObjectsOwnedBy function, passing in all meta objects and the specified class loader)
      return filterAllMetaObjectsOwnedBy(allMetaObjects(), owner);
    }

    /**
     * @brief 获取与指定库关联的所有元对象 (Get all meta objects associated with the specified library)
     *
     * @param[in] library_path 指定库的路径 (The path of the specified library)
     * @return MetaObjectVector 元对象向量 (Vector of meta objects)
     */
    MetaObjectVector
    allMetaObjectsForLibrary(const std::string &library_path)
    {
      // 调用 filterAllMetaObjectsAssociatedWithLibrary 函数，传入所有元对象和指定库的路径
      // (Call the filterAllMetaObjectsAssociatedWithLibrary function, passing in all meta objects and the path of the specified library)
      return filterAllMetaObjectsAssociatedWithLibrary(allMetaObjects(), library_path);
    }

    /**
     * @brief 获取与指定库关联且属于指定类加载器的所有元对象 (Get all meta objects associated with the specified library and owned by the specified class loader)
     *
     * @param[in] library_path 指定库的路径 (The path of the specified library)
     * @param[in] owner 指定的类加载器 (The specified class loader)
     * @return MetaObjectVector 元对象向量 (Vector of meta objects)
     */
    MetaObjectVector
    allMetaObjectsForLibraryOwnedBy(const std::string &library_path, const ClassLoader *owner)
    {
      // 调用 filterAllMetaObjectsOwnedBy 函数，传入与指定库关联的所有元对象和指定的类加载器
      // (Call the filterAllMetaObjectsOwnedBy function, passing in all meta objects associated with the specified library and the specified class loader)
      return filterAllMetaObjectsOwnedBy(allMetaObjectsForLibrary(library_path), owner);
    }

    /**
     * @brief 将元对象插入到墓地（graveyard）
     * @param meta_obj 要插入的元对象指针
     *
     * @details Insert a MetaObject into the graveyard.
     * @param meta_obj Pointer to the MetaObject to be inserted.
     */
    void insertMetaObjectIntoGraveyard(AbstractMetaObjectBase *meta_obj)
    {
      // 打印调试信息，显示插入的类名、基类名和指针地址
      // Log debug message showing the class name, base class name, and pointer address of the inserted object
      CONSOLE_BRIDGE_logDebug(
          "class_loader.impl: "
          "Inserting MetaObject (class = %s, base_class = %s, ptr = %p) into graveyard",
          meta_obj->className().c_str(), meta_obj->baseClassName().c_str(),
          reinterpret_cast<void *>(meta_obj));

      // 将元对象添加到墓地
      // Add the MetaObject to the graveyard
      getMetaObjectGraveyard().push_back(meta_obj);
    }

    /**
     * @brief 销毁库中的元对象
     * @param library_path 库的路径
     * @param factories 工厂映射
     * @param loader 类加载器指针
     *
     * @details Destroy MetaObjects for a specific library.
     * @param library_path Path of the library.
     * @param factories Factory mapping.
     * @param loader Pointer to the ClassLoader.
     */
    void destroyMetaObjectsForLibrary(
        const std::string &library_path, FactoryMap &factories, const ClassLoader *loader)
    {
      // 定义一个迭代器，用于遍历工厂数组
      // Define an iterator for traversing the factory map
      FactoryMap::iterator factory_itr = factories.begin();

      // 遍历工厂数组
      // Iterate through the factory map
      while (factory_itr != factories.end())
      {
        // 获取元对象指针
        // Get the MetaObject pointer
        AbstractMetaObjectBase *meta_obj = factory_itr->second;

        // 检查元对象是否与库路径和类加载器关联
        // Check if the MetaObject is associated with the library path and owned by the ClassLoader
        if (meta_obj->getAssociatedLibraryPath() == library_path && meta_obj->isOwnedBy(loader))
        {
          // 移除元对象的拥有者类加载器
          // Remove the owning ClassLoader of the MetaObject
          meta_obj->removeOwningClassLoader(loader);

          // 检查元对象是否仍然被其他类加载器拥有
          // Check if the MetaObject is still owned by any other ClassLoader
          if (!meta_obj->isOwnedByAnybody())
          {
            // 创建迭代器副本
            // Create a copy of the iterator
            FactoryMap::iterator factory_itr_copy = factory_itr;

            // 增加迭代器
            // Increment the iterator
            factory_itr++;

            // 删除工厂映射中的元对象
            // Erase the MetaObject from the factory map
            factories.erase(factory_itr_copy);

            // 将元对象插入墓地
            // Insert the MetaObject into the graveyard
            insertMetaObjectIntoGraveyard(meta_obj);
          }
          else
          {
            // 增加迭代器
            // Increment the iterator
            ++factory_itr;
          }
        }
        else
        {
          // 增加迭代器
          // Increment the iterator
          ++factory_itr;
        }
      }
    }

    /**
     * @brief 销毁给定库和类加载器的元对象 (Destroy the meta objects for the given library and class loader)
     *
     * @param[in] library_path 库路径 (The path of the library)
     * @param[in] loader 类加载器指针 (Pointer to the class loader)
     */
    void destroyMetaObjectsForLibrary(const std::string &library_path, const ClassLoader *loader)
    {
      // 获取全局插件基础到工厂映射映射的互斥锁 (Get the mutex of the global plugin base to factory map map)
      std::lock_guard<std::recursive_mutex> lock(getPluginBaseToFactoryMapMapMutex());

      // 打印调试信息，表示正在删除与库和类加载器关联的元对象 (Print debug information indicating that the MetaObjects associated with the library and class loader are being removed)
      CONSOLE_BRIDGE_logDebug(
          "class_loader.impl: "
          "Removing MetaObjects associated with library %s and class loader %p from global "
          "plugin-to-factorymap map.\n",
          library_path.c_str(), reinterpret_cast<const void *>(loader));

      // 遍历所有的 FactoryMaps 以确保所有相关的元对象都被销毁 (We have to walk through all FactoryMaps to be sure all related meta objects are destroyed)
      BaseToFactoryMapMap &factory_map_map = getGlobalPluginBaseToFactoryMapMap();
      for (auto &it : factory_map_map)
      {
        destroyMetaObjectsForLibrary(library_path, it.second, loader);
      }

      // 打印调试信息，表示元对象已删除 (Print debug information indicating that the MetaObjects have been removed)
      CONSOLE_BRIDGE_logDebug("%s", "class_loader.impl: Metaobjects removed.");
    }

    /**
     * @brief 检查给定库路径是否存在任何元对象 (Check if there are any existing meta objects for the given library path)
     *
     * @param[in] library_path 库路径 (The path of the library)
     * @return 如果存在元对象，则返回 true，否则返回 false (Return true if there are meta objects, false otherwise)
     */
    bool areThereAnyExistingMetaObjectsForLibrary(const std::string &library_path)
    {
      return allMetaObjectsForLibrary(library_path).size() > 0;
    }

    /**
     * @brief 查找已加载库的迭代器 (Find the iterator of the loaded library)
     *
     * @param[in] library_path 库路径 (The path of the library)
     * @return 如果找到已加载库，则返回相应的迭代器，否则返回已加载库向量的末尾 (Return the corresponding iterator if the loaded library is found, otherwise return the end of the loaded library vector)
     */
    LibraryVector::iterator findLoadedLibrary(const std::string &library_path)
    {
      // 获取已加载库向量的引用 (Get reference to the loaded library vector)
      LibraryVector &open_libraries = getLoadedLibraryVector();

      // 遍历已加载库向量，寻找与给定库路径匹配的项 (Iterate through the loaded library vector, looking for an item that matches the given library path)
      for (auto it = open_libraries.begin(); it != open_libraries.end(); ++it)
      {
        if (it->first == library_path)
        {
          return it;
        }
      }

      // 如果未找到匹配项，则返回已加载库向量的末尾 (Return the end of the loaded library vector if no matching item is found)
      return open_libraries.end();
    }

    /**
     * @brief 判断库是否已被任何人加载 (Check if the library is loaded by anybody)
     *
     * @param library_path 库的路径 (Path of the library)
     * @return true 如果库已被加载 (If the library is loaded)
     * @return false 如果库未被加载 (If the library is not loaded)
     */
    bool isLibraryLoadedByAnybody(const std::string &library_path)
    {
      // 对已加载库向量进行加锁，以避免并发问题 (Lock the loaded library vector to avoid concurrency issues)
      std::lock_guard<std::recursive_mutex> lock(getLoadedLibraryVectorMutex());

      // 获取已加载库向量的引用 (Get a reference to the loaded library vector)
      LibraryVector &open_libraries = getLoadedLibraryVector();
      // 查找指定路径的已加载库 (Find the loaded library with the specified path)
      LibraryVector::iterator itr = findLoadedLibrary(library_path);

      // 如果找到了已加载库 (If the loaded library is found)
      if (itr != open_libraries.end())
      {
        // 确保库已加载 (Ensure that the library is loaded)
        assert(itr->second != nullptr);
        return true;
      }
      else
      {
        return false;
      }
    }

    /**
     * @brief 检查库是否已被特定类加载器加载 (Check if the library is loaded by the specific class loader)
     *
     * @param library_path 库的路径 (Path of the library)
     * @param loader 类加载器指针 (Pointer to the class loader)
     * @return true 如果库已被特定类加载器加载 (If the library is loaded by the specific class loader)
     * @return false 如果库未被特定类加载器加载 (If the library is not loaded by the specific class loader)
     */
    bool isLibraryLoaded(const std::string &library_path, const ClassLoader *loader)
    {
      // 检查库是否已被任何人加载 (Check if the library is loaded by anybody)
      bool is_lib_loaded_by_anyone = isLibraryLoadedByAnybody(library_path);
      // 获取库的元对象数量 (Get the number of meta objects for the library)
      size_t num_meta_objs_for_lib = allMetaObjectsForLibrary(library_path).size();
      // 获取绑定到特定类加载器的库的元对象数量 (Get the number of meta objects for the library bound to the specific class loader)
      size_t num_meta_objs_for_lib_bound_to_loader =
          allMetaObjectsForLibraryOwnedBy(library_path, loader).size();
      // 判断元对象是否绑定到特定类加载器 (Check if the meta objects are bound to the specific class loader)
      bool are_meta_objs_bound_to_loader =
          (0 == num_meta_objs_for_lib) ? true : (num_meta_objs_for_lib_bound_to_loader <= num_meta_objs_for_lib);

      return is_lib_loaded_by_anyone && are_meta_objs_bound_to_loader;
    }

    /**
     * @brief 获取由特定类加载器使用的所有库 (Get all libraries used by the specific class loader)
     *
     * @param loader 类加载器指针 (Pointer to the class loader)
     * @return std::vector<std::string> 包含所有库路径的向量 (A vector containing paths of all libraries)
     */
    std::vector<std::string> getAllLibrariesUsedByClassLoader(const ClassLoader *loader)
    {
      // 获取特定类加载器的所有元对象 (Get all meta objects for the specific class loader)
      MetaObjectVector all_loader_meta_objs = allMetaObjectsForClassLoader(loader);
      // 用于存储所有库路径的向量 (A vector to store paths of all libraries)
      std::vector<std::string> all_libs;
      // 遍历特定类加载器的所有元对象 (Iterate through all meta objects for the specific class loader)
      for (auto &meta_obj : all_loader_meta_objs)
      {
        // 获取与元对象关联的库路径 (Get the library path associated with the meta object)
        std::string lib_path = meta_obj->getAssociatedLibraryPath();
        // 如果库路径不在 all_libs 中，则将其添加到 all_libs 中 (If the library path is not in all_libs, add it to all_libs)
        if (std::find(all_libs.begin(), all_libs.end(), lib_path) == all_libs.end())
        {
          all_libs.push_back(lib_path);
        }
      }
      return all_libs;
    }

    /**
     * @brief 添加类加载器所有者到指定库的所有元对象
     *        Add class loader owner to all existing meta objects for the specified library.
     *
     * @param[in] library_path 库路径 (Library path)
     * @param[in] loader 类加载器指针 (Pointer to ClassLoader)
     */
    void addClassLoaderOwnerForAllExistingMetaObjectsForLibrary(
        const std::string &library_path, ClassLoader *loader)
    {
      // 获取指定库的所有元对象 (Get all meta objects for the specified library)
      MetaObjectVector all_meta_objs = allMetaObjectsForLibrary(library_path);

      // 遍历所有元对象 (Iterate through all meta objects)
      for (auto &meta_obj : all_meta_objs)
      {
        // 记录调试信息 (Log debug information)
        CONSOLE_BRIDGE_logDebug(
            "class_loader.impl: "
            "Tagging existing MetaObject %p (base = %s, derived = %s) with "
            "class loader %p (library path = %s).",
            reinterpret_cast<void *>(meta_obj), meta_obj->baseClassName().c_str(),
            meta_obj->className().c_str(),
            reinterpret_cast<void *>(loader),
            nullptr == loader ? "NULL" : loader->getLibraryPath().c_str());

        // 为元对象添加拥有的类加载器 (Add owning class loader to the meta object)
        meta_obj->addOwningClassLoader(loader);
      }
    }

    /**
     * @brief 从墓地中恢复之前创建的元对象
     *        Revive previously created meta objects from the graveyard.
     *
     * @param[in] library_path 库路径 (Library path)
     * @param[in] loader 类加载器指针 (Pointer to ClassLoader)
     */
    void revivePreviouslyCreateMetaobjectsFromGraveyard(
        const std::string &library_path, ClassLoader *loader)
    {
      // 加锁保护插件基类到工厂映射的映射 (Lock guard to protect plugin base to factory map mapping)
      std::lock_guard<std::recursive_mutex> b2fmm_lock(getPluginBaseToFactoryMapMapMutex());

      // 获取元对象墓地引用 (Get reference to the meta object graveyard)
      MetaObjectGraveyardVector &graveyard = getMetaObjectGraveyard();

      // 遍历墓地中的元对象 (Iterate through the meta objects in the graveyard)
      for (auto &obj : graveyard)
      {
        // 如果元对象关联的库路径与指定库路径相同 (If the associated library path of the meta object is the same as the specified library path)
        if (obj->getAssociatedLibraryPath() == library_path)
        {
          // 记录调试信息 (Log debug information)
          CONSOLE_BRIDGE_logDebug(
              "class_loader.impl: "
              "Resurrected factory metaobject from graveyard, class = %s, base_class = %s ptr = %p..."
              "bound to ClassLoader %p (library path = %s)",
              obj->className().c_str(), obj->baseClassName().c_str(), reinterpret_cast<void *>(obj),
              reinterpret_cast<void *>(loader),
              nullptr == loader ? "NULL" : loader->getLibraryPath().c_str());

          // 为元对象添加拥有的类加载器 (Add owning class loader to the meta object)
          obj->addOwningClassLoader(loader);

          // 断言基类的类型ID不是"UNSET" (Assert that the typeid of the base class is not "UNSET")
          assert(obj->typeidBaseClassName() != "UNSET");

          // 获取基类的工厂映射 (Get the factory map for the base class)
          FactoryMap &factory = getFactoryMapForBaseClass(obj->typeidBaseClassName());

          // 将元对象添加到工厂映射中 (Add the meta object to the factory map)
          factory[obj->className()] = obj;
        }
      }
    }

    /**
     * @brief 清除元对象墓场中与指定库路径和类加载器关联的元对象（Purge the graveyard of metaobjects associated with the specified library path and class loader）
     *
     * @param library_path 要清除的库路径（The library path to be purged）
     * @param loader 与元对象关联的类加载器（The class loader associated with the metaobjects）
     */
    void purgeGraveyardOfMetaobjects(
        const std::string &library_path, ClassLoader *loader)
    {
      // 获取所有元对象（Get all metaobjects）
      MetaObjectVector all_meta_objs = allMetaObjects();

      // 注意：锁定必须在调用 allMetaObjects 之后进行，因为该调用会锁定（Note: Lock must happen after call to allMetaObjects as that will lock）
      std::lock_guard<std::recursive_mutex> b2fmm_lock(getPluginBaseToFactoryMapMapMutex());

      // 获取元对象墓场引用（Get a reference to the metaobject graveyard）
      MetaObjectGraveyardVector &graveyard = getMetaObjectGraveyard();
      MetaObjectGraveyardVector::iterator itr = graveyard.begin();

      // 遍历元对象墓场（Iterate through the metaobject graveyard）
      while (itr != graveyard.end())
      {
        AbstractMetaObjectBase *obj = *itr;

        // 判断元对象是否与指定库路径关联（Check if the metaobject is associated with the specified library path）
        if (obj->getAssociatedLibraryPath() == library_path)
        {
          // 记录调试信息（Log debug information）
          CONSOLE_BRIDGE_logDebug(
              "class_loader.impl: "
              "Purging factory metaobject from graveyard, class = %s, base_class = %s ptr = %p.."
              ".bound to ClassLoader %p (library path = %s)",
              obj->className().c_str(), obj->baseClassName().c_str(), reinterpret_cast<void *>(obj),
              reinterpret_cast<void *>(loader),
              nullptr == loader ? "NULL" : loader->getLibraryPath().c_str());

          // 从墓场中删除元对象（Erase the metaobject from the graveyard）
          itr = graveyard.erase(itr);
        }
        else
        {
          // 移动到下一个元对象（Move to the next metaobject）
          itr++;
        }
      }
    }

    /**
     * @brief 加载一个共享库，并将其与指定的类加载器关联
     * @param library_path 共享库的路径
     * @param loader 与共享库关联的类加载器
     *
     * Load a shared library and associate it with the specified class loader.
     * @param library_path The path of the shared library.
     * @param loader The class loader to associate with the shared library.
     */
    void loadLibrary(const std::string &library_path, ClassLoader *loader)
    {
      // 输出调试信息，尝试加载库，并记录当前的类加载器
      // Output debug information, attempt to load the library, and record the current class loader
      CONSOLE_BRIDGE_logDebug(
          "class_loader.impl: "
          "Attempting to load library %s on behalf of ClassLoader handle %p...\n",
          library_path.c_str(), reinterpret_cast<void *>(loader));

      // 如果库已经被其他人加载，则只需更新现有元对象以具有其他所有者
      // If the library is already loaded by someone else, just update existing metaobjects to have an additional owner
      if (isLibraryLoadedByAnybody(library_path))
      {
        CONSOLE_BRIDGE_logDebug(
            "%s",
            "class_loader.impl: "
            "Library already in memory, but binding existing MetaObjects to loader if necesesary.\n");
        addClassLoaderOwnerForAllExistingMetaObjectsForLibrary(library_path, loader);
        return;
      }

      std::shared_ptr<rcpputils::SharedLibrary> library_handle;

      // 定义一个递归互斥锁，用于保护加载过程
      // Define a recursive mutex to protect the loading process
      static std::recursive_mutex loader_mutex;

      {
        std::lock_guard<std::recursive_mutex> loader_lock(loader_mutex);

        setCurrentlyActiveClassLoader(loader);
        setCurrentlyLoadingLibraryName(library_path);
        try
        {
          library_handle = std::make_shared<rcpputils::SharedLibrary>(library_path.c_str());
        }
        catch (const std::runtime_error &e)
        {
          setCurrentlyLoadingLibraryName("");
          setCurrentlyActiveClassLoader(nullptr);
          throw class_loader::LibraryLoadException("Could not load library " + std::string(e.what()));
        }
        catch (const std::bad_alloc &e)
        {
          setCurrentlyLoadingLibraryName("");
          setCurrentlyActiveClassLoader(nullptr);
          throw class_loader::LibraryLoadException("Bad alloc " + std::string(e.what()));
        }

        setCurrentlyLoadingLibraryName("");
        setCurrentlyActiveClassLoader(nullptr);
      }

      assert(library_handle != nullptr);
      CONSOLE_BRIDGE_logDebug(
          "class_loader.impl: "
          "Successfully loaded library %s into memory (handle = %p).",
          library_path.c_str(), reinterpret_cast<void *>(library_handle.get()));

      // 墓地场景：检查是否有之前加载的元对象
      // Graveyard scenario: Check for previously loaded metaobjects
      size_t num_lib_objs = allMetaObjectsForLibrary(library_path).size();
      if (0 == num_lib_objs)
      {
        CONSOLE_BRIDGE_logDebug(
            "class_loader.impl: "
            "Though the library %s was just loaded, it seems no factory metaobjects were registered. "
            "Checking factory graveyard for previously loaded metaobjects...",
            library_path.c_str());
        revivePreviouslyCreateMetaobjectsFromGraveyard(library_path, loader);
      }
      else
      {
        CONSOLE_BRIDGE_logDebug(
            "class_loader.impl: "
            "Library %s generated new factory metaobjects on load. "
            "Destroying graveyarded objects from previous loads...",
            library_path.c_str());
      }
      purgeGraveyardOfMetaobjects(library_path, loader);

      // 将库插入全局已加载库向量
      // Insert the library into the global loaded library vector
      std::lock_guard<std::recursive_mutex> llv_lock(getLoadedLibraryVectorMutex());
      LibraryVector &open_libraries = getLoadedLibraryVector();
      // 注意：当库传递给构造函数时，rcpputils::SharedLibrary 自动调用 load()
      // Note: rcpputils::SharedLibrary automatically calls load() when the library is passed to the constructor
      open_libraries.push_back(LibraryPair(library_path, library_handle));
    }

    /**
     * @brief 卸载库 (Unload a library)
     *
     * @param[in] library_path 要卸载的库的路径 (Path of the library to unload)
     * @param[in] loader 类加载器指针，用于处理库的加载和卸载 (Pointer to ClassLoader for handling library loading and unloading)
     */
    void unloadLibrary(const std::string &library_path, ClassLoader *loader)
    {
      // 检查是否已打开一个非纯插件库 (Check if a non-pure plugin library has been opened)
      if (hasANonPurePluginLibraryBeenOpened())
      {
        // 如果已经打开了一个非纯插件库，则无法卸载任何库 (If a non-pure plugin library is opened, no libraries can be unloaded)
        CONSOLE_BRIDGE_logDebug(
            "class_loader.impl: "
            "Cannot unload %s or ANY other library as a non-pure plugin library was opened. "
            "As class_loader has no idea which libraries class factories were exported from, "
            "it can safely close any library without potentially unlinking symbols that are still "
            "actively being used. "
            "You must refactor your plugin libraries to be made exclusively of plugins "
            "in order for this error to stop happening.",
            library_path.c_str());
      }
      else
      {
        // 开始卸载库 (Start unloading the library)
        CONSOLE_BRIDGE_logDebug(
            "class_loader.impl: "
            "Unloading library %s on behalf of ClassLoader %p...",
            library_path.c_str(), reinterpret_cast<void *>(loader));

        LibraryVector &open_libraries = getLoadedLibraryVector();
        LibraryVector::iterator itr = findLoadedLibrary(library_path);
        if (itr != open_libraries.end())
        {
          auto library = itr->second;
          std::string library_path = itr->first;
          try
          {
            // 销毁库中的元对象 (Destroy meta objects in the library)
            destroyMetaObjectsForLibrary(library_path, loader);

            // 如果没有与该库关联的工厂，则从已加载的库列表中删除 (Remove from loaded library list if no more factories are associated with said library)
            if (!areThereAnyExistingMetaObjectsForLibrary(library_path))
            {
              CONSOLE_BRIDGE_logDebug(
                  "class_loader.impl: "
                  "There are no more MetaObjects left for %s so unloading library and "
                  "removing from loaded library vector.\n",
                  library_path.c_str());

              library->unload_library();
              itr = open_libraries.erase(itr);
            }
            else
            {
              // 如果仍有元对象存在于内存中，表示其他类加载器仍在使用库，因此保持库打开 (If meta objects still exist in memory, other ClassLoaders are using the library, so keep it open)
              CONSOLE_BRIDGE_logDebug(
                  "class_loader.impl: "
                  "MetaObjects still remain in memory meaning other ClassLoaders are still using library"
                  ", keeping library %s open.",
                  library_path.c_str());
            }
            return;
          }
          catch (const std::runtime_error &e)
          {
            // 抛出无法卸载库的异常 (Throw exception for not being able to unload the library)
            throw class_loader::LibraryUnloadException(
                "Could not unload library (rcpputils exception = " + std::string(e.what()) + ")");
          }
        }
        else
        {
          // 如果尝试卸载类加载器不知道的库或已经卸载的库 (If attempting to unload a library that the class_loader is unaware of or already unloaded)
          CONSOLE_BRIDGE_logDebug(
              "class_loader.impl: "
              "Attempt to unload library %s that class_loader is unaware of or is already unloaded",
              library_path.c_str());
        }
      }
    }

    /**
     * @brief 打印调试信息到屏幕 (Print debug information to the screen)
     *
     * 该函数用于打印类加载器实现的相关调试信息，包括内存中打开的库和元对象（工厂）。
     * (This function is used to print debug information related to the class_loader implementation, including open libraries and metaobjects (factories) in memory.)
     */
    void printDebugInfoToScreen()
    {
      // 打印分隔线 (Print separator line)
      printf("*******************************************************************************\n");
      printf("*****                 class_loader impl DEBUG INFORMATION                 *****\n");
      printf("*******************************************************************************\n");

      // 打印内存中打开的库信息 (Print open libraries in memory information)
      printf("OPEN LIBRARIES IN MEMORY:\n");
      printf("--------------------------------------------------------------------------------\n");

      // 获取已加载库向量的互斥锁 (Get the mutex lock of the loaded library vector)
      std::lock_guard<std::recursive_mutex> lock(getLoadedLibraryVectorMutex());

      // 获取已加载库向量 (Get the loaded library vector)
      LibraryVector libs = getLoadedLibraryVector();

      // 遍历已加载库向量并打印信息 (Iterate through the loaded library vector and print information)
      for (size_t c = 0; c < libs.size(); c++)
      {
        printf(
            "Open library %zu = %s (handle = %p)\n",
            c, (libs.at(c)).first.c_str(), reinterpret_cast<void *>((libs.at(c)).second.get()));
      }

      // 打印内存中的元对象（工厂）信息 (Print metaobjects (factories) in memory information)
      printf("METAOBJECTS (i.e. FACTORIES) IN MEMORY:\n");
      printf("--------------------------------------------------------------------------------\n");

      // 获取所有元对象向量 (Get all metaobjects vector)
      MetaObjectVector meta_objs = allMetaObjects();

      // 遍历元对象向量并打印信息 (Iterate through the metaobject vector and print information)
      for (size_t c = 0; c < meta_objs.size(); c++)
      {
        AbstractMetaObjectBase *obj = meta_objs.at(c);
        printf(
            "Metaobject %zu (ptr = %p):\n TypeId = %s\n Associated Library = %s\n",
            c,
            reinterpret_cast<void *>(obj),
            (typeid(*obj).name()),
            obj->getAssociatedLibraryPath().c_str());

        // 获取关联的类加载器数量 (Get the number of associated class loaders)
        size_t size = obj->getAssociatedClassLoadersCount();

        // 遍历关联的类加载器并打印信息 (Iterate through the associated class loaders and print information)
        for (size_t i = 0; i < size; ++i)
        {
          printf(
              " Associated Loader %zu = %p\n",
              i, reinterpret_cast<void *>(obj->getAssociatedClassLoader(i)));
        }
        printf("--------------------------------------------------------------------------------\n");
      }

      // 打印结束调试信息 (Print end debug information)
      printf("********************************** END DEBUG **********************************\n");
      printf("*******************************************************************************\n\n");
    }

  } // namespace impl
} // namespace class_loader
