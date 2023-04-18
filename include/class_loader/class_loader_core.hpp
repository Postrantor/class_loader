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

#ifndef CLASS_LOADER__CLASS_LOADER_CORE_HPP_
#define CLASS_LOADER__CLASS_LOADER_CORE_HPP_

#include <boost/thread/recursive_mutex.hpp>
#include <cstddef>
#include <cstdio>
#include <map>
#include <string>
#include <typeinfo>
#include <utility>
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

#include "class_loader/exceptions.hpp"
#include "class_loader/meta_object.hpp"
#include "class_loader/visibility_control.hpp"

// forward declaration
namespace Poco
{
  class SharedLibrary;
}  // namespace Poco

/**
 * @note This header file is the internal implementation of the plugin system which is exposed via the ClassLoader class
 */

namespace class_loader
{

  class ClassLoader; // 前向声明 (Forward declaration)

  namespace impl
  {

    // 类型定义 (Typedefs)
    typedef std::string LibraryPath;                                                       // 库路径类型 (Library path type)
    typedef std::string ClassName;                                                         // 类名类型 (Class name type)
    typedef std::string BaseClassName;                                                     // 基类名类型 (Base class name type)
    typedef std::map<ClassName, impl::AbstractMetaObjectBase *> FactoryMap;                // 工厂映射类型，用于存储类名与抽象元对象指针的映射关系 (Factory map type, used to store the mapping relationship between class name and abstract meta-object pointer)
    typedef std::map<BaseClassName, FactoryMap> BaseToFactoryMapMap;                       // 基类到工厂映射的映射类型，用于存储基类名与工厂映射之间的映射关系 (Base class to factory map mapping type, used to store the mapping relationship between base class name and factory map)
    typedef std::pair<LibraryPath, std::shared_ptr<rcpputils::SharedLibrary>> LibraryPair; // 库对类型，用于存储库路径与共享库智能指针的组合 (Library pair type, used to store the combination of library path and shared library smart pointer)
    typedef std::vector<LibraryPair> LibraryVector;                                        // 库向量类型，用于存储多个库对 (Library vector type, used to store multiple library pairs)
    typedef std::vector<AbstractMetaObjectBase *> MetaObjectVector;                        // 元对象向量类型，用于存储多个抽象元对象指针 (Meta-object vector type, used to store multiple abstract meta-object pointers)

    // MetaObjectGraveyardVector 类 (MetaObjectGraveyardVector class)
    class MetaObjectGraveyardVector : public MetaObjectVector
    {
    public:
      // 析构函数 (Destructor)
      ~MetaObjectGraveyardVector()
      {
        // 在 unique_ptr deleter 中销毁 `meta_object` 时，确保不访问 `getMetaObjectGraveyard()` 静态变量中的指针值。
        // 因为在某些情况下，`getMetaObjectGraveyard()` 中的静态变量可能会在静态变量 `g_register_plugin_ ## UniqueID` 之前被销毁。
        // 关于 STL 中 vector dtor 的注意事项：如果元素本身是指针，则不以任何方式触及指向的内存。管理指针是用户的责任。
        // (Make sure not to access the pointer value in the static variable of `getMetaObjectGraveyard()
        // when destroying `meta_object` in the unique_ptr deleter. Because the static variable in
        // `getMetaObjectGraveyard()` can be destructed before the static variable
        // `g_register_plugin_ ## UniqueID` in some circumstances.
        // NOTE of the vector dtor in the STL: if the elements themselves are pointers, the pointed-to
        // memory is not touched in any way.  Managing the pointer is the user's responsibility.)
        clear();
      }
    };

    CLASS_LOADER_PUBLIC
    void printDebugInfoToScreen();

    // 全局存储 (Global storage)

    /**
     * @brief 获取一个指向全局数据结构的句柄，该结构包含一个基类名称映射（Base class描述插件接口）到FactoryMap的映射，
     * FactoryMap保存了可以实例化的各种不同具体类的工厂。请注意，基类名并非字面上的类名，而是typeid(Base).name()的结果，
     * 有时是字面上的类名（如在Windows上），但通常是以改变形式出现的（如在Linux上）。
     * (Gets a handle to a global data structure that holds a map of base class names (Base class
     * describes plugin interface) to a FactoryMap which holds the factories for the various different
     * concrete classes that can be instantiated. Note that the Base class is NOT THE LITERAL CLASSNAME,
     * but rather the result of typeid(Base).name() which sometimes is the literal class name
     * (as on Windows) but is often in mangled form (as on Linux).)
     *
     * @return 返回对全局基类到工厂映射的引用 (A reference to the global base to factory map)
     */
    CLASS_LOADER_PUBLIC
    BaseToFactoryMapMap &getGlobalPluginBaseToFactoryMapMap();

    /**
     * @brief 获取一个指向已打开库列表的句柄，该列表以LibraryPairs的形式编码库路径+名称和底层共享库的句柄
     * (Gets a handle to a list of open libraries in the form of LibraryPairs which encode the
     * library path+name and the handle to the underlying shared library)
     *
     * @return 返回跟踪已加载库的全局向量的引用 (A reference to the global vector that tracks loaded libraries)
     */
    CLASS_LOADER_PUBLIC
    LibraryVector &getLoadedLibraryVector();

    /**
     * @brief 当一个库被加载时，为了让工厂知道它们与哪个库相关联，它们使用此函数查询正在加载哪个库。
     * (When a library is being loaded, in order for factories to know which library they are
     * being associated with, they use this function to query which library is being loaded.)
     *
     * @return 返回当前设置的加载库名称作为字符串 (The currently set loading library name as a string)
     */
    CLASS_LOADER_PUBLIC
    std::string getCurrentlyLoadingLibraryName();

    /**
     * @brief 当一个库被加载时，为了让工厂知道它们与哪个库相关联，调用此函数以设置当前正在加载的库的名称。
     * (When a library is being loaded, in order for factories to know which library they are
     * being associated with, this function is called to set the name of the library currently being loaded.)
     * @param library_name - 当前正在加载的库的名称 (The name of library that is being loaded currently)
     */
    CLASS_LOADER_PUBLIC
    void setCurrentlyLoadingLibraryName(const std::string &library_name);

    /**
     * @brief 获取当前正在加载库时使用的范围内的ClassLoader。 (Gets the ClassLoader currently in scope which used when a library is being loaded.)
     * @return 当前活动的ClassLoader指针。 (A pointer to the currently active ClassLoader.)
     */
    CLASS_LOADER_PUBLIC
    ClassLoader *getCurrentlyActiveClassLoader();

    /**
     * @brief 设置当前正在加载库时使用的范围内的ClassLoader。 (Sets the ClassLoader currently in scope which used when a library is being loaded.)
     *
     * @param loader - 指向当前活动的ClassLoader的指针。 (pointer to the currently active ClassLoader.)
     */
    CLASS_LOADER_PUBLIC
    void setCurrentlyActiveClassLoader(ClassLoader *loader);

    /**
     * @brief 此函数从全局插件基类到工厂映射中提取适当基类的FactoryMap引用。 (This function extracts a reference to the FactoryMap for appropriate base class out of the global plugin base to factory map.)
     *        应该由需要访问各种工厂的此命名空间中的函数使用此函数，以确保生成正确的键来索引全局映射。 (This function should be used by functions in this namespace that need to access the various factories so as to make sure the right key is generated to index into the global map.)
     *
     * @return 全局Base-to-FactoryMap映射中包含的FactoryMap引用。 (A reference to the FactoryMap contained within the global Base-to-FactoryMap map.)
     */
    CLASS_LOADER_PUBLIC
    FactoryMap &getFactoryMapForBaseClass(const std::string &typeid_base_class_name);

    /**
     * @brief 与上面相同，但是使用类型参数而不是字符串以获得更多安全性（如果有信息）。 (Same as above but uses a type parameter instead of string for more safety if info is available.)
     *
     * @return 全局Base-to-FactoryMap映射中包含的FactoryMap引用。 (A reference to the FactoryMap contained within the global Base-to-FactoryMap map.)
     */
    template <typename Base>
    FactoryMap &getFactoryMapForBaseClass()
    {
      return getFactoryMapForBaseClass(typeid(Base).name());
    }

    /**
     * @brief 获取一个指向元对象墓地列表的句柄。 (Gets a handle to a list of meta object of graveyard.)
     *
     * @return 元对象墓地中包含的MetaObjectGraveyardVector引用。 (A reference to the MetaObjectGraveyardVector contained within the meta object of graveyard.)
     */
    CLASS_LOADER_PUBLIC
    MetaObjectGraveyardVector &getMetaObjectGraveyard();

    /**
     * @brief 为了提供线程安全，所有暴露的插件函数只能由多个线程串行运行。
     *        (To provide thread safety, all exposed plugin functions can only be run serially by multiple threads.)
     *
     * 这是通过使用单个互斥锁强制实施的临界区来实现的，该互斥锁使用以下两个函数进行锁定和释放。
     * (This is implemented by using critical sections enforced by a single mutex which is locked and
     *  released with the following two functions.)
     *
     * @return 对全局互斥锁的引用 (A reference to the global mutex)
     */
    CLASS_LOADER_PUBLIC
    std::recursive_mutex &getLoadedLibraryVectorMutex();
    // 获取已加载库向量的互斥锁 (Get the mutex of the loaded library vector)

    CLASS_LOADER_PUBLIC
    std::recursive_mutex &getPluginBaseToFactoryMapMapMutex();
    // 获取插件基类到工厂映射的互斥锁 (Get the mutex of the plugin base to factory map)

    /**
     * @brief 指示运行过程中是否打开了包含不止插件的库
     *        (Indicates if a library containing more than just plugins has been opened by the running process)
     *
     * @return 如果已打开非纯插件库，则返回true，否则返回false
     *         (True if a non-pure plugin library has been opened, otherwise false)
     */
    CLASS_LOADER_PUBLIC
    bool hasANonPurePluginLibraryBeenOpened();

    /**
     * @brief 设置一个标志，指示运行过程中是否打开了包含不止插件的库
     *        (Sets a flag indicating if a library containing more than just plugins has been opened by the running process)
     *
     * @param hasIt - 标志 (The flag)
     */
    CLASS_LOADER_PUBLIC
    void hasANonPurePluginLibraryBeenOpened(bool hasIt);

    // 定义删除器类型为一个接受基类指针参数的函数 (Define DeleterType as a function taking a pointer to Base class as parameter)
    template <typename Base>
    using DeleterType = std::function<void(Base *)>;

    // 定义 UniquePtr 为一个具有特定删除器类型的 unique_ptr (Define UniquePtr as a unique_ptr with a specific DeleterType)
    template <typename Base>
    using UniquePtr = std::unique_ptr<Base, DeleterType<Base>>;

    // 插件函数 (Plugin Functions)

    /**
     * @brief 该函数由 plugin_register_macro.h 中的 CLASS_LOADER_REGISTER_CLASS 宏调用以注册工厂。
     * 使用该宏的类将在库加载时调用此函数。
     * 该函数将为相应的 Derived 类创建一个 MetaObject（即工厂），并将其插入到全局 Base-to-FactoryMap 映射中的适当 FactoryMap 中。
     * 请注意，传递的 class_name 是字面类名，而不是混淆版本。
     * (This function is called by the CLASS_LOADER_REGISTER_CLASS macro in plugin_register_macro.h to register factories.
     * Classes that use that macro will cause this function to be invoked when the library is loaded.
     * The function will create a MetaObject (i.e. factory) for the corresponding Derived class and
     * insert it into the appropriate FactoryMap in the global Base-to-FactoryMap map.
     * Note that the passed class_name is the literal class name and not the mangled version.)
     *
     * @param Derived - 表示插件具体类型的参数类型 (parameteric type indicating concrete type of plugin)
     * @param Base - 表示插件基本类型的参数类型 (parameteric type indicating base type of plugin)
     * @param class_name - 被注册类的字面名称（不是混淆的）(the literal name of the class being registered (NOT MANGLED))
     * @return 一个指向新创建的元对象的 class_loader::impl::UniquePtr<AbstractMetaObjectBase>。(A class_loader::impl::UniquePtr<AbstractMetaObjectBase> to newly created meta object.)
     */
    template <typename Derived, typename Base>
    UniquePtr<AbstractMetaObjectBase>
    registerPlugin(const std::string &class_name, const std::string &base_class_name)
    {
      // 注意：此函数将在 dlopen() 调用打开库时自动调用。
      // 通常，它会在 loadLibrary() 的范围内发生，
      // 但这不能保证。
      // Note: This function will be automatically invoked when a dlopen() call
      // opens a library. Normally it will happen within the scope of loadLibrary(),
      // but that may not be guaranteed.
      CONSOLE_BRIDGE_logDebug(
          "class_loader.impl: "
          "Registering plugin factory for class = %s, ClassLoader* = %p and library name %s.",
          class_name.c_str(), getCurrentlyActiveClassLoader(),
          getCurrentlyLoadingLibraryName().c_str());

      // 如果当前没有活动的类加载器
      // If there is no currently active class loader
      if (nullptr == getCurrentlyActiveClassLoader())
      {
        CONSOLE_BRIDGE_logDebug(
            "%s",
            "class_loader.impl: ALERT!!! "
            // 通过其他方式而非通过 class_loader 或 pluginlib 包打开了包含插件的库。
            // A library containing plugins has been opened through a means other than through the
            // class_loader or pluginlib package.
            "This can happen if you build plugin libraries that contain more than just plugins "
            "(i.e. normal code your app links against). "
            "This inherently will trigger a dlopen() prior to main() and cause problems as class_loader "
            "is not aware of plugin factories that autoregister under the hood. "
            "The class_loader package can compensate, but you may run into namespace collision problems "
            "(e.g. if you have the same plugin class in two different libraries and you load them both "
            "at the same time). "
            "The biggest problem is that library can now no longer be safely unloaded as the "
            "ClassLoader does not know when non-plugin code is still in use. "
            "In fact, no ClassLoader instance in your application will be unable to unload any library "
            "once a non-pure one has been opened. "
            // 请重构您的代码，将插件隔离到它们自己的库中。
            // Please refactor your code to isolate plugins into their own libraries.
        );
        hasANonPurePluginLibraryBeenOpened(true);
      }

      // 创建工厂 (Create factory)
      UniquePtr<AbstractMetaObjectBase> new_factory(
          // 使用 MetaObject 实例化一个新的工厂对象，传入 Derived 类型和 Base 类型的名称
          // (Instantiate a new factory object using MetaObject, passing the names of the Derived and Base types)
          new impl::MetaObject<Derived, Base>(class_name, base_class_name),
          [](AbstractMetaObjectBase *p)
          {
            // 获取全局插件基类到工厂映射的互斥锁并加锁
            // (Get the mutex for the global plugin base to factory map and lock it)
            getPluginBaseToFactoryMapMapMutex().lock();

            // 获取元对象墓地向量的引用
            // (Get a reference to the MetaObject Graveyard vector)
            MetaObjectGraveyardVector &graveyard = getMetaObjectGraveyard();

            // 遍历墓地向量，查找与当前指针匹配的元对象
            // (Iterate through the graveyard vector, searching for a metaobject that matches the current pointer)
            for (auto iter = graveyard.begin(); iter != graveyard.end(); ++iter)
            {
              if (*iter == p)
              {
                // 删除匹配的元对象并退出循环
                // (Erase the matching metaobject and break out of the loop)
                graveyard.erase(iter);
                break;
              }
            }

            // 获取全局插件基类到工厂映射的引用
            // (Get a reference to the global plugin base to factory map)
            BaseToFactoryMapMap &factory_map_map = getGlobalPluginBaseToFactoryMapMap();

            // 设置擦除标志
            // (Set the erase flag)
            bool erase_flag = false;

            // 遍历工厂映射，查找与当前指针匹配的元对象
            // (Iterate through the factory map, searching for a metaobject that matches the current pointer)
            for (auto &factory_map_item : factory_map_map)
            {
              FactoryMap &factory_map = factory_map_item.second;
              for (auto iter = factory_map.begin(); iter != factory_map.end(); ++iter)
              {
                if (iter->second == p)
                {
                  // 删除匹配的元对象并设置擦除标志为真
                  // (Erase the matching metaobject and set the erase flag to true)
                  factory_map.erase(iter);
                  erase_flag = true;
                  break;
                }
              }
              if (erase_flag)
              {
                break;
              }
            }

            // 解锁全局插件基类到工厂映射的互斥锁
            // (Unlock the mutex for the global plugin base to factory map)
            getPluginBaseToFactoryMapMapMutex().unlock();

#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#endif
            // 注意：这是唯一可以销毁元对象的地方
            // (Note: This is the only place where metaobjects can be destroyed)
            delete (p);
#ifndef _WIN32
#pragma GCC diagnostic pop
#endif
          });
      // 将当前活动的类加载器添加为新工厂的所有者
      // (Add the currently active class loader as an owner of the new factory)
      new_factory->addOwningClassLoader(getCurrentlyActiveClassLoader());
      // 将当前正在加载的库的路径与新工厂关联
      // (Associate the path of the currently loading library with the new factory)
      new_factory->setAssociatedLibraryPath(getCurrentlyLoadingLibraryName());

      // 将其添加到全局工厂映射 map 中
      // Add it to global factory map map
      getPluginBaseToFactoryMapMapMutex().lock(); // 对全局工厂映射 map 的互斥锁加锁
                                                  // Lock the mutex for the global factory map map

      FactoryMap &factoryMap = getFactoryMapForBaseClass<Base>(); // 获取基类 Base 的工厂映射
                                                                  // Get the factory map for base class Base

      if (factoryMap.find(class_name) != factoryMap.end()) // 如果找到了与 class_name 相同的类
                                                           // If a class with the same class_name is found
      {
        CONSOLE_BRIDGE_logWarn( // 输出警告日志
            "class_loader.impl: SEVERE WARNING!!! "
            "A namespace collision has occurred with plugin factory for class %s. "
            "New factory will OVERWRITE existing one. "
            "This situation occurs when libraries containing plugins are directly linked against an "
            "executable (the one running right now generating this message). "
            "Please separate plugins out into their own library or just don't link against the library "
            "and use either class_loader::ClassLoader/MultiLibraryClassLoader to open.",
            class_name.c_str()); // 警告信息，表示发生了命名空间冲突，新工厂将覆盖现有工厂
      }

      factoryMap[class_name] = new_factory.get(); // 将新工厂添加到工厂映射中
                                                  // Add the new factory to the factory map

      getPluginBaseToFactoryMapMapMutex().unlock(); // 对全局工厂映射 map 的互斥锁解锁
                                                    // Unlock the mutex for the global factory map map

      CONSOLE_BRIDGE_logDebug( // 输出调试日志
          "class_loader.impl: "
          "Registration of %s complete (Metaobject Address = %p)",
          class_name.c_str(), reinterpret_cast<void *>(new_factory.get())); // 注册完成，输出类名和元对象地址
                                                                            // Registration complete, output class name and metaobject address

      return new_factory; // 返回新工厂
                          // Return the new factory
    }

    /**
     * @brief 这个函数根据插件类的派生类名称创建一个插件类实例，并返回基类类型的指针。
     *        This function creates an instance of a plugin class given the derived name of the class and returns a pointer of the Base class type.
     *
     * @param derived_class_name - 派生类的名称（未修饰）
     *                             The name of the derived class (unmangled)
     * @param loader - 我们所在范围内的 ClassLoader
     *                 The ClassLoader whose scope we are within
     * @return 新创建的插件的指针，注意调用者负责对象销毁
     *         A pointer to newly created plugin, note caller is responsible for object destruction
     */
    template <typename Base>
    Base *createInstance(const std::string &derived_class_name, ClassLoader *loader)
    {
      // 声明一个 AbstractMetaObject<Base> 类型的指针变量 factory，并初始化为 nullptr
      // Declare a pointer variable of type AbstractMetaObject<Base> named factory and initialize it to nullptr
      AbstractMetaObject<Base> *factory = nullptr;

      /// \brief 获取插件基类到工厂映射的互斥锁 (Get the mutex for the plugin base class to factory map)
      getPluginBaseToFactoryMapMapMutex().lock();

      /// \brief 获取基类对应的工厂映射 (Get the factory map for the given base class)
      FactoryMap &factoryMap = getFactoryMapForBaseClass<Base>();

      /// \brief 查找派生类名称是否在工厂映射中 (Check if the derived class name is present in the factory map)
      if (factoryMap.find(derived_class_name) != factoryMap.end())
      {
        /// \brief 如果存在，将工厂指针转换为相应的基类元对象类型 (If present, cast the factory pointer to the appropriate base class meta-object type)
        factory = dynamic_cast<impl::AbstractMetaObject<Base> *>(factoryMap[derived_class_name]);
      }
      else
      {
        /// \brief 如果不存在，记录错误信息 (If not present, log an error message)
        CONSOLE_BRIDGE_logError(
            "class_loader.impl: No metaobject exists for class type %s.", derived_class_name.c_str());
      }
      /// \brief 解锁插件基类到工厂映射的互斥锁 (Unlock the mutex for the plugin base class to factory map)
      getPluginBaseToFactoryMapMapMutex().unlock();

      /// \brief 初始化对象指针为空指针 (Initialize object pointer to null)
      Base *obj = nullptr;

      /// \brief 如果工厂不为空且工厂拥有加载器，则创建对象 (If factory is not null and factory is owned by the loader, create the object)
      if (factory != nullptr && factory->isOwnedBy(loader))
      {
        obj = factory->create();
      }

      /// \brief 如果对象未创建 (If the object was not created)
      if (nullptr == obj)
      {
        /// \brief 如果工厂存在且没有拥有者 (If factory exists and has no owner)
        if (factory && factory->isOwnedBy(nullptr))
        {
          CONSOLE_BRIDGE_logDebug(
              "%s",
              "class_loader.impl: ALERT!!! "
              "A metaobject (i.e. factory) exists for desired class, but has no owner. "
              "This implies that the library containing the class was dlopen()ed by means other than "
              "through the class_loader interface. "
              "This can happen if you build plugin libraries that contain more than just plugins "
              "(i.e. normal code your app links against) -- that intrinsically will trigger a dlopen() "
              "prior to main(). "
              "You should isolate your plugins into their own library, otherwise it will not be "
              "possible to shutdown the library!");

          /// \brief 创建对象 (Create the object)
          obj = factory->create();
        }
        else
        {
          /// \brief 抛出无法创建类实例的异常 (Throw an exception for failing to create an instance of the class)
          throw class_loader::CreateClassException(
              "Could not create instance of type " + derived_class_name);
        }
      }

      /// \brief 记录调试信息，包括创建的对象类型和指针值 (Log debug information including the created object type and pointer value)
      CONSOLE_BRIDGE_logDebug(
          "class_loader.impl: Created instance of type %s and object pointer = %p",
          (typeid(obj).name()), obj);

      /// \brief 返回创建的对象指针 (Return the created object pointer)
      return obj;
    }

    /**
     * @brief 这个函数返回插件系统中所有可用的从Base派生并在传递的ClassLoader范围内的类加载器。
     * This function returns all the available class_loader in the plugin system
     * that are derived from Base and within scope of the passed ClassLoader.
     *
     * @param loader - 我们所在范围的ClassLoader的指针。
     * The pointer to the ClassLoader whose scope we are within,
     * @return 一个字符串向量，其中每个字符串都是我们可以创建的插件。
     * A vector of strings where each string is a plugin we can create
     */
    template <typename Base>
    std::vector<std::string> getAvailableClasses(const ClassLoader *loader)
    {
      // 对getPluginBaseToFactoryMapMapMutex()返回的互斥体进行加锁，确保线程安全。
      // Locks the mutex returned by getPluginBaseToFactoryMapMapMutex() to ensure thread safety.
      std::lock_guard<std::recursive_mutex> lock(getPluginBaseToFactoryMapMapMutex());

      // 获取基类为Base的工厂映射。
      // Gets the factory map for base class Base.
      FactoryMap &factory_map = getFactoryMapForBaseClass<Base>();

      // 定义两个字符串向量，分别存储拥有者为loader的类和无拥有者的类。
      // Define two string vectors to store classes owned by loader and classes with no owner.
      std::vector<std::string> classes;
      std::vector<std::string> classes_with_no_owner;

      // 遍历factory_map
      // Iterate through factory_map
      for (auto &it : factory_map)
      {
        AbstractMetaObjectBase *factory = it.second;

        // 如果工厂的拥有者是传入的loader，将类名添加到classes向量中。
        // If the factory is owned by the passed loader, add the class name to the classes vector.
        if (factory->isOwnedBy(loader))
        {
          classes.push_back(it.first);
        }
        // 如果工厂没有拥有者（nullptr），将类名添加到classes_with_no_owner向量中。
        // If the factory has no owner (nullptr), add the class name to the classes_with_no_owner vector.
        else if (factory->isOwnedBy(nullptr))
        {
          classes_with_no_owner.push_back(it.first);
        }
      }

      // 将不与类加载器关联的类（通过意外的dlopen()加载库时可能发生）添加到classes向量中。
      // Add classes not associated with a class loader (which can happen through
      // an unexpected dlopen() to the library) to the classes vector.
      classes.insert(classes.end(), classes_with_no_owner.begin(), classes_with_no_owner.end());

      // 返回所有可用的类名。
      // Return all available class names.
      return classes;
    }

    /**
     * @brief 此函数返回在给定类加载器中使用的所有库的名称。 (This function returns the names of all libraries in use by a given class loader.)
     *
     * @param loader 类加载器指针，我们在其范围内 (The ClassLoader whose scope we are within)
     * @return 一个字符串向量，其中每个字符串都是在ClassLoader可见范围内的每个库的路径+名称 (A vector of strings where each string is the path+name of each library that are within a ClassLoader's visible scope)
     */
    CLASS_LOADER_PUBLIC
    std::vector<std::string> getAllLibrariesUsedByClassLoader(const ClassLoader *loader);

    /**
     * @brief 表示传递的库是否在ClassLoader范围内加载。库可能已经加载到内存中，但对于类加载器来说可能还没有。 (Indicates if passed library loaded within scope of a ClassLoader. The library maybe loaded in memory, but to the class loader it may not be.)
     *
     * @param library_path 我们希望检查是否打开的库的名称 (The name of the library we wish to check is open)
     * @param loader 指向ClassLoader的指针，我们在其范围内 (The pointer to the ClassLoader whose scope we are within)
     * @return 如果库在加载器范围内加载，则为true，否则为false (true if the library is loaded within loader's scope, else false)
     */
    CLASS_LOADER_PUBLIC
    bool isLibraryLoaded(const std::string &library_path, const ClassLoader *loader);

    /**
     * @brief 表示传递的库是否被任何ClassLoader加载 (Indicates if passed library has been loaded by ANY ClassLoader)
     *
     * @param library_path 我们希望检查是否打开的库的名称 (The name of the library we wish to check is open)
     * @return 如果库已加载到内存中，则为true，否则为false (true if the library is loaded in memory, otherwise false)
     */
    CLASS_LOADER_PUBLIC
    bool isLibraryLoadedByAnybody(const std::string &library_path);

    /**
     * @brief 如果尚未这样做，将库加载到内存中。尝试加载已加载的库没有任何效果。 (Loads a library into memory if it has not already been done so. Attempting to load an already loaded library has no effect.)
     *
     * @param library_path 要打开的库的名称 (The name of the library to open)
     * @param loader 指向ClassLoader的指针，我们在其范围内 (The pointer to the ClassLoader whose scope we are within)
     */
    CLASS_LOADER_PUBLIC
    void loadLibrary(const std::string &library_path, ClassLoader *loader);

    /**
     * @brief 如果库在内存中加载，则卸载库并清理其相应的类工厂。如果未加载，则该函数无效 (Unloads a library if it loaded in memory and cleans up its corresponding class factories. If it is not loaded, the function has no effect)
     *
     * @param library_path 要打开的库的名称 (The name of the library to open)
     * @param loader 指向ClassLoader的指针，我们在其范围内 (The pointer to the ClassLoader whose scope we are within)
     */
    CLASS_LOADER_PUBLIC
    void unloadLibrary(const std::string &library_path, ClassLoader *loader);

  } // namespace impl
} // namespace class_loader

#endif // CLASS_LOADER__CLASS_LOADER_CORE_HPP_
