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

#ifndef CLASS_LOADER__CLASS_LOADER_HPP_
#define CLASS_LOADER__CLASS_LOADER_HPP_

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <cstddef>
#include <functional>
#include <memory>
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

#include "class_loader/class_loader_core.hpp"
#include "class_loader/register_macro.hpp"
#include "class_loader/visibility_control.hpp"

namespace class_loader
{

  /// 返回基本库名称的平台特定版本 (Returns a platform specific version of a basic library name)
  /**
   * 在*nix平台上，库名称前缀为`lib`。 (On *nix platforms the library name is prefixed with `lib`.)
   * 在所有平台上，都会附加 class_loader::systemLibrarySuffix() 的输出。 (On all platforms the output of class_loader::systemLibrarySuffix() is appended.)
   *
   * @param[in] library_name 需要添加前缀的库名称 (name to add the prefix)
   * @return 添加了前缀的库名称 (library name with the prefix added)
   */
  CLASS_LOADER_PUBLIC
  std::string systemLibraryFormat(const std::string &library_name);

  /**
   * @class ClassLoader
   * @brief 这个类允许在运行时加载和卸载包含类定义的动态链接库，从这些类定义中可以创建/销毁对象（即 class_loader）。
   *        由 ClassLoader 加载的库仅在 ClassLoader 对象的范围内可访问。
   *
   * @brief This class allows loading and unloading of dynamically linked libraries which contain class
   *        definitions from which objects can be created/destroyed during runtime (i.e. class_loader).
   *        Libraries loaded by a ClassLoader are only accessible within scope of that ClassLoader object.
   */
  class ClassLoader
  {
  public:
    // 定义 DeleterType 模板别名，用于表示删除器类型。其中 Base 表示基类类型。
    // Define the DeleterType template alias for representing the deleter type. Where Base is the base class type.
    template <typename Base>
    using DeleterType = std::function<void(Base *)>;

    template <typename Base>
    using UniquePtr = std::unique_ptr<Base, DeleterType<Base>>;

    /**
     * @brief 构造函数，用于创建 ClassLoader 对象
     *
     * @param library_path - 要加载的运行时库的路径
     * @param ondemand_load_unload - 表示在插件创建/销毁时是否进行按需（惰性）卸载/加载库
     *
     * @brief Constructor for ClassLoader
     *
     * @param library_path - The path of the runtime library to load
     * @param ondemand_load_unload - Indicates if on-demand (lazy) unloading/loading of libraries
     * occurs as plugins are created/destroyed.
     */
    CLASS_LOADER_PUBLIC
    explicit ClassLoader(const std::string &library_path, bool ondemand_load_unload = false);

    /**
     * @brief ClassLoader 的析构函数。由此 ClassLoader 打开的所有库都会自动卸载。
     *
     * @brief Destructor for ClassLoader. All libraries opened by this ClassLoader are unloaded automatically.
     */
    CLASS_LOADER_PUBLIC
    virtual ~ClassLoader();

    /**
     * @brief 表示可以由此对象加载哪些类（即 class_loader）
     *
     * @return 一个字符串向量，表示从 <Base> 派生的可实例化类的名称
     *
     * @brief Indicates which classes (i.e. class_loader) that can be loaded by this object
     *
     * @return vector of strings indicating names of instantiable classes derived from <Base>
     */
    template <class Base>
    std::vector<std::string> getAvailableClasses() const
    {
      return class_loader::impl::getAvailableClasses<Base>(this);
    }

    /**
     * @brief 生成可加载类的实例（即 class_loader）。
     *
     * 用户无需调用 loadLibrary()，因为如果库尚未加载（这通常发生在 "按需加载/卸载" 模式下），它将自动调用。
     *
     * @param derived_class_name 我们要创建的类的名称（@see getAvailableClasses()）
     * @return 新创建的插件对象的 std::shared_ptr<Base>
     *
     * @brief Generates an instance of loadable classes (i.e. class_loader).
     *
     * It is not necessary for the user to call loadLibrary() as it will be invoked automatically
     * if the library is not yet loaded (which typically happens when in "On Demand Load/Unload" mode).
     *
     * @param derived_class_name The name of the class we want to create (@see getAvailableClasses())
     * @return A std::shared_ptr<Base> to newly created plugin object
     */
    template <class Base>
    std::shared_ptr<Base> createInstance(const std::string &derived_class_name)
    {
      return std::shared_ptr<Base>(
          createRawInstance<Base>(derived_class_name, true),
          std::bind(&ClassLoader::onPluginDeletion<Base>, this, std::placeholders::_1));
    }

    /// 生成可加载类的实例（例如 class_loader）。
    /// Generates an instance of loadable classes (i.e. class_loader).
    /**
     * 用户无需调用 loadLibrary()，因为如果库尚未加载（通常在 "On Demand Load/Unload" 模式下发生），它将自动调用。
     * It is not necessary for the user to call loadLibrary() as it will be
     * invoked automatically if the library is not yet loaded (which typically
     * happens when in "On Demand Load/Unload" mode).
     *
     * 如果您释放了包装指针，则必须在要销毁已释放指针时手动调用原始删除程序。
     * If you release the wrapped pointer you must manually call the original
     * deleter when you want to destroy the released pointer.
     *
     * @param derived_class_name
     *   我们想要创建的类的名称（@see getAvailableClasses()）。
     *   The name of the class we want to create (@see getAvailableClasses()).
     * @return 一个 std::unique_ptr<Base>，指向新创建的插件对象。
     *   A std::unique_ptr<Base> to newly created plugin object.
     */
    template <class Base>
    UniquePtr<Base> createUniqueInstance(const std::string &derived_class_name)
    {
      // 创建原始实例并获取裸指针
      // Create raw instance and get the raw pointer
      Base *raw = createRawInstance<Base>(derived_class_name, true);

      // 返回具有自定义删除程序的 std::unique_ptr 对象
      // Return a std::unique_ptr with custom deleter
      return std::unique_ptr<Base, DeleterType<Base>>(
          raw,
          std::bind(&ClassLoader::onPluginDeletion<Base>, this, std::placeholders::_1));
    }

    /// 生成可加载类的实例（例如 class_loader）。
    /// Generates an instance of loadable classes (i.e. class_loader).
    /**
     * 用户无需调用 loadLibrary()，因为如果库尚未加载（通常在 "On Demand Load/Unload" 模式下发生），它将自动调用。
     * It is not necessary for the user to call loadLibrary() as it will be
     * invoked automatically if the library is not yet loaded (which typically
     * happens when in "On Demand Load/Unload" mode).
     *
     * 创建非托管实例会在此进程中的所有类加载器中禁用在托管指针超出范围时动态卸载库。
     * Creating an unmanaged instance disables dynamically unloading libraries when
     * managed pointers go out of scope for all class loaders in this process.
     *
     * @param derived_class_name
     *   我们想要创建的类的名称（@see getAvailableClasses()）。
     *   The name of the class we want to create (@see getAvailableClasses()).
     * @return 指向新创建的插件对象的非托管（即不是 shared_ptr）Base*。
     *   An unmanaged (i.e. not a shared_ptr) Base* to newly created plugin object.
     */
    template <class Base>
    Base *createUnmanagedInstance(const std::string &derived_class_name)
    {
      // 创建原始实例并返回裸指针
      // Create raw instance and return the raw pointer
      return createRawInstance<Base>(derived_class_name, false);
    }

    /**
     * @brief 检查插件类是否可用 (Indicates if a plugin class is available)
     *
     * @tparam Base 多态类型，表示基类 (Polymorphic type indicating base class)
     * @param class_name 插件类的名称 (The name of the plugin class)
     * @return 如果可用则返回true，否则返回false (true if yes it is available, false otherwise)
     */
    template <class Base>
    bool isClassAvailable(const std::string &class_name) const
    {
      // 获取所有可用的类名 (Get all available class names)
      std::vector<std::string> available_classes = getAvailableClasses<Base>();
      // 查找指定的类名是否在可用类名列表中 (Find if the specified class name is in the list of available class names)
      return std::find(
                 available_classes.begin(), available_classes.end(), class_name) != available_classes.end();
    }

    /**
     * @brief 获取与此类加载器关联的库的完全限定路径和名称 (Gets the full-qualified path and name of the library associated with this class loader)
     *
     * @return 库的完全限定路径和名称 (the full-qualified path and name of the library)
     */
    CLASS_LOADER_PUBLIC
    const std::string &getLibraryPath() const;

    /**
     * @brief 在此ClassLoader范围内指示库是否已加载 (Indicates if a library is loaded within the scope of this ClassLoader)
     *
     * 请注意，该库可能已通过其他ClassLoader在内部加载，但是在调用loadLibrary()方法之前，ClassLoader无法从所述库创建对象。
     * 如果我们想知道库是否已被其他人打开，请参阅isLibraryLoadedByAnyClassloader()
     * (Note that the library may already be loaded internally through another ClassLoader,
     * but until loadLibrary() method is called, the ClassLoader cannot create objects from said library.
     * If we want to see if the library has been opened by somebody else, @see isLibraryLoadedByAnyClassloader())
     *
     * @param library_path 要加载的库的路径 (The path to the library to load)
     * @return 如果在此ClassLoader对象范围内加载了库，则为true，否则为false (true if library is loaded within this ClassLoader object's scope, otherwise false)
     */
    CLASS_LOADER_PUBLIC
    bool isLibraryLoaded() const;

    /**
     * @brief 指示库是否由插件系统中的某个实体（另一个ClassLoader）加载，但不一定由此ClassLoader加载 (Indicates if a library is loaded by some entity in the plugin system (another ClassLoader), but not necessarily loaded by this ClassLoader)
     *
     * @return 如果在插件系统范围内加载了库，则为true，否则为false (true if library is loaded within the scope of the plugin system, otherwise false)
     */
    CLASS_LOADER_PUBLIC
    bool isLibraryLoadedByAnyClassloader() const;

    /**
     * @brief 表示库是否根据需求加载/卸载...这意味着仅在创建第一个插件时加载库，并在销毁最后一个活动插件时自动关闭。
     * Indicates if the library is to be loaded/unloaded on demand...meaning that only to load
     * a lib when the first plugin is created and automatically shut it down when last active plugin
     * is destroyed.
     *
     * @return 如果启用了按需加载和卸载，则返回 true，否则返回 false
     * @return true if on-demand load and unload is active, otherwise false
     */
    CLASS_LOADER_PUBLIC
    bool isOnDemandLoadUnloadEnabled() const;

    /**
     * @brief 试图代表 ClassLoader 加载一个库。如果库已经打开，则此方法无效。如果库已经由其他实体打开（例如另一个 ClassLoader 或全局接口），则允许此对象访问由该其他实体加载的任何插件类。
     * Attempts to load a library on behalf of the ClassLoader. If the library is already
     * opened, this method has no effect. If the library has been already opened by some other entity
     * (i.e. another ClassLoader or global interface), this object is given permissions to access any
     * plugin classes loaded by that other entity.
     */
    CLASS_LOADER_PUBLIC
    void loadLibrary();

    /**
     * @brief 尝试卸载在 ClassLoader 范围内加载的库。如果库未打开，则此方法无效。如果库是由另一个 ClassLoader 打开的，
     * 则库不会在内部卸载 -- 但是此 ClassLoader 将无法实例化绑定到该库的 class_loader。如果内存中存在由此类加载器创建的插件对象，
     * 则会出现警告消息，并且不会卸载库。如果多次调用 loadLibrary()（例如在多个线程或单个线程中故意），用户需要负责相同次数地调用 unloadLibrary()。
     * 只有在卸载调用次数与加载次数匹配时，才会在此类加载器的上下文中卸载库。
     * Attempts to unload a library loaded within scope of the ClassLoader. If the library is
     * not opened, this method has no effect. If the library is opened by another ClassLoader,
     * the library will NOT be unloaded internally -- however this ClassLoader will no longer be able
     * to instantiate class_loader bound to that library. If there are plugin objects that exist in
     * memory created by this classloader, a warning message will appear and the library will not be
     * unloaded. If loadLibrary() was called multiple times (e.g. in the case of multiple threads or
     * purposefully in a single thread), the user is responsible for calling unloadLibrary() the same
     * number of times. The library will not be unloaded within the context of this classloader until
     * the number of unload calls matches the number of loads.
     *
     * @return 为了使其与此 ClassLoader 解除绑定，还需要再调用多少次 unloadLibrary()
     * @return The number of times more unloadLibrary() has to be called for it to be unbound from this ClassLoader
     */
    CLASS_LOADER_PUBLIC
    int unloadLibrary();

  private:
    /**
     * @brief 插件被销毁时的回调方法 (Callback method when a plugin created by this class loader is destroyed)
     *
     * @param obj - 被删除对象的指针 (A pointer to the deleted object)
     */
    template <class Base>
    void onPluginDeletion(Base *obj)
    {
      // 使用 CONSOLE_BRIDGE_logDebug 记录调试信息，输出当前正在销毁的插件对象指针
      // (Use CONSOLE_BRIDGE_logDebug to log debug information, outputting the pointer of the plugin object being destroyed)
      CONSOLE_BRIDGE_logDebug(
          "class_loader::ClassLoader: Calling onPluginDeletion() for obj ptr = %p.\n",
          reinterpret_cast<void *>(obj));

      // 检查传入的 obj 是否为空指针，如果是则直接返回
      // (Check if the passed-in obj is a nullptr, and return directly if it is)
      if (nullptr == obj)
      {
        return;
      }

      // 使用 std::lock_guard 对递归互斥量进行加锁，防止多线程访问冲突
      // (Use std::lock_guard to lock the recursive mutex to prevent multi-thread access conflicts)
      std::lock_guard<std::recursive_mutex> lock(plugin_ref_count_mutex_);

      // 销毁传入的 obj 对象
      // (Destroy the passed-in obj object)
      delete (obj);

      // 确保插件引用计数大于0
      // (Ensure that the plugin reference count is greater than 0)
      assert(plugin_ref_count_ > 0);

      // 减少插件引用计数
      // (Decrease the plugin reference count)
      --plugin_ref_count_;

      // 如果插件引用计数为0且按需加载卸载功能被启用，则尝试卸载库
      // (If the plugin reference count is 0 and the on-demand load/unload feature is enabled, try unloading the library)
      if (plugin_ref_count_ == 0 && isOnDemandLoadUnloadEnabled())
      {
        // 如果没有创建非托管实例，则卸载内部库
        // (If no unmanaged instance has been created, unload the internal library)
        if (!ClassLoader::hasUnmanagedInstanceBeenCreated())
        {
          unloadLibraryInternal(false);
        }
        else
        {
          // 如果有非托管实例被创建，输出警告信息，说明无法卸载库
          // (If an unmanaged instance has been created, output a warning message indicating that the library cannot be unloaded)
          CONSOLE_BRIDGE_logWarn(
              "class_loader::ClassLoader: "
              "Cannot unload library %s even though last shared pointer went out of scope. "
              "This is because createUnmanagedInstance was used within the scope of this process, "
              "perhaps by a different ClassLoader. "
              "Library will NOT be closed.",
              getLibraryPath().c_str());
        }
      }
    }

    /// 生成可加载类实例（例如 class_loader）。
    /// Generates an instance of loadable classes (i.e. class_loader).
    /**
     * 用户无需调用 loadLibrary()，因为如果库尚未加载（通常在 "按需加载/卸载" 模式下发生），它将自动调用。
     * It is not necessary for the user to call loadLibrary() as it will be
     * invoked automatically if the library is not yet loaded (which typically
     * happens when in "On Demand Load/Unload" mode).
     *
     * @param derived_class_name
     *   我们要创建的类的名称（@see getAvailableClasses()）。
     *   The name of the class we want to create (@see getAvailableClasses()).
     * @param managed
     *   如果为 true，则调用者将返回的指针包装在智能指针中。
     *   If true, the returned pointer is assumed to be wrapped in a smart
     *   pointer by the caller.
     * @return 指向新创建的插件对象的 Base*。
     * @return A Base* to newly created plugin object.
     */
    template <class Base>
    Base *createRawInstance(const std::string &derived_class_name, bool managed)
    {
      // 如果实例未被管理，则设置 unmanaged_instance_been_created_ 为 true
      // If the instance is not managed, set unmanaged_instance_been_created_ to true
      if (!managed)
      {
        this->setUnmanagedInstanceBeenCreated(true);
      }

      // 当实例被管理、未管理实例已创建且启用了按需加载/卸载时，输出一条信息
      // Output a message when the instance is managed, unmanaged instance has been created and on-demand loading/unloading is enabled
      if (
          managed &&
          ClassLoader::hasUnmanagedInstanceBeenCreated() &&
          isOnDemandLoadUnloadEnabled())
      {
        CONSOLE_BRIDGE_logInform(
            "%s",
            "class_loader::ClassLoader: "
            "An attempt is being made to create a managed plugin instance (i.e. boost::shared_ptr), "
            "however an unmanaged instance was created within this process address space. "
            "This means libraries for the managed instances will not be shutdown automatically on "
            "final plugin destruction if on demand (lazy) loading/unloading mode is used.");
      }
      // 如果库未加载，则加载库
      // Load the library if it's not loaded yet
      if (!isLibraryLoaded())
      {
        loadLibrary();
      }

      // 创建类实例并将其分配给 obj
      // Create class instance and assign it to obj
      Base *obj = class_loader::impl::createInstance<Base>(derived_class_name, this);
      // 如果 createInstance() 在失败时抛出异常，则无法达到的断言。
      // Unreachable assertion if createInstance() throws on failure.
      assert(obj != NULL);

      // 如果实例被管理，对插件引用计数进行加锁并递增
      // If the instance is managed, lock the plugin reference count and increment it
      if (managed)
      {
        std::lock_guard<std::recursive_mutex> lock(plugin_ref_count_mutex_);
        ++plugin_ref_count_;
      }

      // 返回新创建的对象
      // Return the newly created object
      return obj;
    }

    /**
     * @brief Getter for if an unmanaged (i.e. unsafe) instance has been created flag
     * @brief 获取一个未托管（即不安全）实例是否已创建的标志的 getter
     */
    CLASS_LOADER_PUBLIC
    static bool hasUnmanagedInstanceBeenCreated();

    /**
     * @brief Setter for if an unmanaged (i.e. unsafe) instance has been created flag
     * @brief 设置一个未托管（即不安全）实例是否已创建的标志
     *
     * @param state - The new state of the unmanaged instance flag
     * @param state - 未托管实例标志的新状态
     */
    CLASS_LOADER_PUBLIC
    static void setUnmanagedInstanceBeenCreated(bool state);

    /**
     * @brief As the library may be unloaded in "on-demand load/unload" mode, unload maybe called from
     * createInstance(). The problem is that createInstance() locks the plugin_ref_count as does
     * unloadLibrary(). This method is the implementation of unloadLibrary but with a parameter to
     * decide if plugin_ref_mutex_ should be locked.
     * @brief 由于库可能在“按需加载/卸载”模式下被卸载，因此可能从 createInstance() 调用卸载。问题是 createInstance()
     * 锁定了 plugin_ref_count，就像 unloadLibrary() 一样。此方法是 unloadLibrary 的实现，但带有一个参数，
     * 以决定是否应锁定 plugin_ref_mutex_。
     *
     * @param lock_plugin_ref_count - Set to true if plugin_ref_count_mutex_ should be locked, else false
     * @param lock_plugin_ref_count - 如果应锁定 plugin_ref_count_mutex_，则设置为 true，否则为 false
     * @return The number of times unloadLibraryInternal has to be called again for it to be unbound
     *   from this ClassLoader
     * @return 为了从此 ClassLoader 解除绑定，需要再次调用 unloadLibraryInternal 的次数
     */
    CLASS_LOADER_PUBLIC
    int unloadLibraryInternal(bool lock_plugin_ref_count);

  private:
    bool ondemand_load_unload_;
    std::string library_path_;
    int load_ref_count_;
    std::recursive_mutex load_ref_count_mutex_;
    int plugin_ref_count_;
    std::recursive_mutex plugin_ref_count_mutex_;
    static bool has_unmananged_instance_been_created_;
  };

} // namespace class_loader

#endif // CLASS_LOADER__CLASS_LOADER_HPP_
