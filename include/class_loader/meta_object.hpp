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

// Note: This header defines a simplication of Poco::MetaObject
// that allows us to tag MetaObjects with an associated library name.

#ifndef CLASS_LOADER__META_OBJECT_HPP_
#define CLASS_LOADER__META_OBJECT_HPP_

#include <console_bridge/console.h>
#include "class_loader/visibility_control.hpp"

#include <typeinfo>
#include <string>
#include <vector>

namespace class_loader
{

  class ClassLoader; // Forward declaration

  namespace impl
  {

    class AbstractMetaObjectBaseImpl;

    /**
     * @class AbstractMetaObjectBase
     * @brief 一个不包含多态类型参数的 MetaObjects 基类。子类是类模板。
     * @brief A base class for MetaObjects that excludes a polymorphic type parameter. Subclasses are class templates though.
     */
    class CLASS_LOADER_PUBLIC AbstractMetaObjectBase
    {
    public:
      /**
       * @brief 类的构造函数
       * @brief Constructor for the class
       * @param class_name 类名
       * @param base_class_name 基类名
       * @param typeid_base_class_name 基类的类型ID名称，默认为 "UNSET"
       */
      AbstractMetaObjectBase(
          const std::string &class_name,
          const std::string &base_class_name,
          const std::string &typeid_base_class_name = "UNSET");

      /**
       * @brief 类的析构函数。这个析构函数不能是虚拟的，也不能被模板子类重写，否则它们将在插件库中的 libclass_loader 之外拉入一个冗余的 MetaObject 析构函数！
       * @brief Destructor for the class. THIS MUST NOT BE VIRTUAL AND OVERRIDDEN BY TEMPLATE SUBCLASSES, OTHERWISE THEY WILL PULL IN A REDUNDANT METAOBJECT DESTRUCTOR OUTSIDE OF libclass_loader WITHIN THE PLUGIN LIBRARY!
       */
      ~AbstractMetaObjectBase();
    };

    /**
     * @brief 获取类的字面名称。
     *
     * @return 类的字面名称，以 C-字符串 形式返回。
     * @brief Gets the literal name of the class.
     *
     * @return The literal name of the class as a C-string.
     */
    const std::string &className() const;

    /**
     * @brief 获取此工厂表示的类的基类
     * @brief Gets the base class for the class this factory represents
     */
    const std::string &baseClassName() const;

    /**
     * @brief 获取类名，就像 typeid(BASE_CLASS).name() 返回的一样
     * @brief Gets the name of the class as typeid(BASE_CLASS).name() would return it
     */
    const std::string &typeidBaseClassName() const;

    /**
     * @brief 获取与此工厂关联的库的路径
     *
     * @return 库路径，以 std::string 形式返回
     * @brief Gets the path to the library associated with this factory
     *
     * @return Library path as a std::string
     */
    const std::string &getAssociatedLibraryPath() const;

    /**
     * @brief 设置与此工厂关联的库的路径
     * @brief Sets the path to the library associated with this factory
     */
    void setAssociatedLibraryPath(const std::string &library_path);

    /**
     * @brief 将一个 ClassLoader 所有者与此工厂关联，
     *
     * @param loader 指向拥有的 ClassLoader 的句柄。
     * @brief Associates a ClassLoader owner with this factory,
     *
     * @param loader Handle to the owning ClassLoader.
     */
    void addOwningClassLoader(ClassLoader *loader);

    /**
     * @brief 移除一个是此工厂所有者的 ClassLoader
     *
     * @param loader 指向拥有的 ClassLoader 的句柄。
     * @brief Removes a ClassLoader that is an owner of this factory
     *
     * @param loader Handle to the owning ClassLoader.
     */
    void removeOwningClassLoader(const ClassLoader *loader);

    /**
     * @brief 指示工厂是否在 ClassLoader 的可用范围内
     *
     * @param loader 指向拥有的 ClassLoader 的句柄。
     * @return 如果工厂在 ClassLoader 的可用范围内，则为 true；否则为 false
     * @brief Indicates if the factory is within the usable scope of a ClassLoader
     *
     * @param loader Handle to the owning ClassLoader.
     * @return True if the factory is within the usable scope of a ClassLoader, false otherwise
     */
    bool isOwnedBy(const ClassLoader *loader) const;

    /**
     * @brief 指示工厂是否在任何ClassLoader的可用范围内 (Indicates if the factory is within the usable scope of any ClassLoader)
     *
     * @return 如果工厂在任何ClassLoader的可用范围内，则为true，否则为false (true if the factory is within the usable scope of any ClassLoader, false otherwise)
     */
    bool isOwnedByAnybody() const;

    /**
     * @brief 获取关联的类加载器数量 (Get the number of associated class Loaders)
     *
     * @return 关联的类加载器数量 (number of associated class loaders)
     */
    size_t getAssociatedClassLoadersCount() const;

    /**
     * @brief 通过索引获取关联的ClassLoader指针 (Get an associated ClassLoader pointer by index)
     *
     * @param[in] index 类加载器的索引 (The index of the ClassLoader.)
     * @return 类加载器指针，如果索引越界则为未定义行为 (The ClassLoader pointer or undefined behaviour if the index is out of bounds)
     */
    ClassLoader *getAssociatedClassLoader(size_t index) const;

  protected:
    /**
     * 使基类多态所需的方法（即具有虚拟表）(This is needed to make base class polymorphic (i.e. have a vtable))
     */
    virtual void dummyMethod() {}

    // 实现抽象元对象基类的指针 (Pointer to the implementation of the AbstractMetaObjectBase class)
    AbstractMetaObjectBaseImpl *impl_;
  };

  /**
   * @class AbstractMetaObject
   * @brief Abstract base class for factories where polymorphic type variable indicates base class for
   *  plugin interface.
   *
   * @tparam B The base class interface for the plugin
   */
  template <class B>
  class AbstractMetaObject : public AbstractMetaObjectBase
  {
  public:
    /**
     * @brief 构造函数 (Constructor)
     * @brief A constructor for this class
     *
     * @param class_name 类的名称 (The literal name of the class.)
     * @param base_class_name 基类的名称 (The literal name of the base class.)
     */
    AbstractMetaObject(const std::string &class_name, const std::string &base_class_name)
        : AbstractMetaObjectBase(class_name, base_class_name, typeid(B).name())
    {
    }

    /**
     * @brief 定义 MetaObject 必须实现的工厂接口 (Defines the factory interface that the MetaObject must implement.)
     * @brief Defines the factory interface that the MetaObject must implement.
     *
     * @return 返回指向新创建对象的参数类型 B 的指针 (A pointer of parametric type B to a newly created object.)
     */
    virtual B *create() const = 0;
    /// 创建一个类的新实例 (Create a new instance of a class.)
    /// 不能用于单例模式 (Cannot be used for singletons.)

  private:
    // 禁用默认构造函数 (Disable default constructor)
    AbstractMetaObject();

    // 禁用拷贝构造函数 (Disable copy constructor)
    AbstractMetaObject(const AbstractMetaObject &);

    // 禁用赋值运算符 (Disable assignment operator)
    AbstractMetaObject &operator=(const AbstractMetaObject &);
  };

  /**
   * @class MetaObject
   * @brief 实际的工厂类 (The actual factory)
   *
   * @tparam C 派生类（实际的插件）(The derived class - the actual plugin)
   * @tparam B 插件的基类接口 (The base class interface for the plugin)
   */
  template <class C, class B>
  class MetaObject : public AbstractMetaObject<B>
  {
  public:
    /**
     * @brief 类的构造函数 (Constructor for the class)
     *
     * @param class_name 派生类（插件）的类名 (The class name of the derived class - the actual plugin)
     * @param base_class_name 插件的基类接口的类名 (The class name of the base class interface for the plugin)
     */
    MetaObject(const std::string &class_name, const std::string &base_class_name)
        : AbstractMetaObject<B>(class_name, base_class_name)
    {
    }

    /**
     * @brief 生成对象的工厂接口。实际上，对象具有类型C，尽管返回了基类类型的指针。
     *        (The factory interface to generate an object. The object has type C in reality, though a
     *        pointer of the base class type is returned.)
     *
     * @return 返回一个新创建的具有基类类型（类型参数B）的插件指针
     *         (A pointer to a newly created plugin with the base class type - type parameter B)
     */
    B *create() const
    {
      return new C;
    }
  };

} // namespace impl
} // namespace class_loader

#endif // CLASS_LOADER__META_OBJECT_HPP_
