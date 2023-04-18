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

#include <string>

#include "class_loader/class_loader.hpp"
#include "class_loader/meta_object.hpp"

namespace class_loader
{
  namespace impl
  {

    /**
     * @brief 类加载器向量类型定义 (Class loader vector type definition)
     */
    typedef std::vector<class_loader::ClassLoader *> ClassLoaderVector;

    /**
     * @class AbstractMetaObjectBaseImpl
     * @brief 抽象元对象基类实现 (Abstract meta object base class implementation)
     */
    class AbstractMetaObjectBaseImpl
    {
    public:
      // 关联的类加载器集合 (Associated class loaders collection)
      ClassLoaderVector associated_class_loaders_;

      // 关联的库路径 (Associated library path)
      std::string associated_library_path_;

      // 基类名称 (Base class name)
      std::string base_class_name_;

      // 类名称 (Class name)
      std::string class_name_;

      // 基类的类型ID名称 (Type ID base class name)
      std::string typeid_base_class_name_;
    };

    /**
     * @brief 构造函数，初始化抽象元对象基类 (Constructor, initializes the abstract meta object base class)
     *
     * @param[in] class_name 类名称 (Class name)
     * @param[in] base_class_name 基类名称 (Base class name)
     * @param[in] typeid_base_class_name 基类的类型ID名称 (Type ID base class name)
     */
    AbstractMetaObjectBase::AbstractMetaObjectBase(
        const std::string &class_name, const std::string &base_class_name,
        const std::string &typeid_base_class_name)
        : impl_(new AbstractMetaObjectBaseImpl())
    {
      // 设置关联的库路径为 "Unknown" (Set the associated library path to "Unknown")
      impl_->associated_library_path_ = "Unknown";

      // 设置基类名称 (Set the base class name)
      impl_->base_class_name_ = base_class_name;

      // 设置类名称 (Set the class name)
      impl_->class_name_ = class_name;

      // 设置基类的类型ID名称 (Set the type ID base class name)
      impl_->typeid_base_class_name_ = typeid_base_class_name;

      // 输出调试信息 (Output debug information)
      CONSOLE_BRIDGE_logDebug(
          "class_loader.impl.AbstractMetaObjectBase: Creating MetaObject %p "
          "(base = %s, derived = %s, library path = %s)",
          this, baseClassName().c_str(), className().c_str(), getAssociatedLibraryPath().c_str());
    }

    /**
     * @brief 析构函数，销毁抽象元对象基类 (Destructor, destroys the abstract meta object base class)
     */
    AbstractMetaObjectBase::~AbstractMetaObjectBase()
    {
      // 输出调试信息 (Output debug information)
      CONSOLE_BRIDGE_logDebug(
          "class_loader.impl.AbstractMetaObjectBase: "
          "Destroying MetaObject %p (base = %s, derived = %s, library path = %s)",
          this, baseClassName().c_str(), className().c_str(), getAssociatedLibraryPath().c_str());

      // 删除实现指针 (Delete implementation pointer)
      delete impl_;
    }

    /**
     * @brief 获取类名称 (Get class name)
     *
     * @return 类名称字符串引用 (Reference to the class name string)
     */
    const std::string &AbstractMetaObjectBase::className() const
    {
      return impl_->class_name_;
    }

    /**
     * @brief 获取基类名称 (Get the base class name)
     *
     * @return 基类名称的引用 (Reference to the base class name)
     */
    const std::string &AbstractMetaObjectBase::baseClassName() const
    {
      // 返回实现对象中存储的基类名称 (Return the base class name stored in the implementation object)
      return impl_->base_class_name_;
    }

    /**
     * @brief 获取类型ID基类名称 (Get the typeid base class name)
     *
     * @return 类型ID基类名称的引用 (Reference to the typeid base class name)
     */
    const std::string &AbstractMetaObjectBase::typeidBaseClassName() const
    {
      // 返回实现对象中存储的类型ID基类名称 (Return the typeid base class name stored in the implementation object)
      return impl_->typeid_base_class_name_;
    }

    /**
     * @brief 获取关联库路径 (Get the associated library path)
     *
     * @return 关联库路径的引用 (Reference to the associated library path)
     */
    const std::string &AbstractMetaObjectBase::getAssociatedLibraryPath() const
    {
      // 返回实现对象中存储的关联库路径 (Return the associated library path stored in the implementation object)
      return impl_->associated_library_path_;
    }

    /**
     * @brief 设置关联库路径 (Set the associated library path)
     *
     * @param library_path 要设置的库路径 (The library path to set)
     */
    void AbstractMetaObjectBase::setAssociatedLibraryPath(const std::string &library_path)
    {
      // 将实现对象中的关联库路径设置为传入的库路径 (Set the associated library path in the implementation object to the given library path)
      impl_->associated_library_path_ = library_path;
    }

    /**
     * @brief 添加拥有的类加载器 (Add owning class loader)
     *
     * @param loader 要添加的类加载器指针 (Pointer to the class loader to add)
     */
    void AbstractMetaObjectBase::addOwningClassLoader(ClassLoader *loader)
    {
      // 获取实现对象中存储的关联类加载器向量的引用 (Get reference to the associated class loaders vector stored in the implementation object)
      ClassLoaderVector &v = impl_->associated_class_loaders_;

      // 检查要添加的类加载器是否已经在向量中 (Check if the class loader to add is already in the vector)
      if (std::find(v.begin(), v.end(), loader) == v.end())
      {
        // 如果不在向量中，将其添加到向量中 (If not in the vector, add it to the vector)
        v.push_back(loader);
      }
    }

    /**
     * @brief 移除拥有的类加载器 (Remove owning class loader)
     *
     * @param loader 要移除的类加载器指针 (Pointer to the class loader to remove)
     */
    void AbstractMetaObjectBase::removeOwningClassLoader(const ClassLoader *loader)
    {
      // 获取实现对象中存储的关联类加载器向量的引用 (Get reference to the associated class loaders vector stored in the implementation object)
      ClassLoaderVector &v = impl_->associated_class_loaders_;

      // 查找要移除的类加载器在向量中的位置 (Find the position of the class loader to remove in the vector)
      ClassLoaderVector::iterator itr = std::find(v.begin(), v.end(), loader);

      // 如果找到了要移除的类加载器 (If the class loader to remove is found)
      if (itr != v.end())
      {
        // 从向量中移除该类加载器 (Remove the class loader from the vector)
        v.erase(itr);
      }
    }

    /**
     * @brief 检查是否由指定的类加载器拥有 (Check if owned by the specified class loader)
     *
     * @param loader 要检查的类加载器指针 (Pointer to the class loader to check)
     * @return 是否由指定的类加载器拥有 (Whether it is owned by the specified class loader)
     */
    bool AbstractMetaObjectBase::isOwnedBy(const ClassLoader *loader) const
    {
      // 获取实现对象中存储的关联类加载器向量的常量引用 (Get constant reference to the associated class loaders vector stored in the implementation object)
      const ClassLoaderVector &v = impl_->associated_class_loaders_;

      // 查找指定的类加载器在向量中的位置 (Find the position of the specified class loader in the vector)
      auto it = std::find(v.begin(), v.end(), loader);

      // 返回是否找到了指定的类加载器 (Return whether the specified class loader is found)
      return it != v.end();
    }

    /**
     * @brief 检查是否由任何类加载器拥有 (Check if owned by anybody)
     *
     * @return 是否由任何类加载器拥有 (Whether it is owned by anybody)
     */
    bool AbstractMetaObjectBase::isOwnedByAnybody() const
    {
      // 返回关联类加载器向量的大小是否大于0 (Return whether the size of the associated class loaders vector is greater than 0)
      return impl_->associated_class_loaders_.size() > 0;
    }

    /**
     * @brief 获取关联类加载器数量 (Get the number of associated class loaders)
     *
     * @return 关联类加载器数量 (Number of associated class loaders)
     */
    size_t AbstractMetaObjectBase::getAssociatedClassLoadersCount() const
    {
      // 返回关联类加载器向量的大小 (Return the size of the associated class loaders vector)
      return impl_->associated_class_loaders_.size();
    }

    /**
     * @brief 获取指定索引处的关联类加载器 (Get the associated class loader at the specified index)
     *
     * @param index 要获取的关联类加载器的索引 (Index of the associated class loader to get)
     * @return 指定索引处的关联类加载器指针 (Pointer to the associated class loader at the specified index)
     */
    ClassLoader *AbstractMetaObjectBase::getAssociatedClassLoader(size_t index) const
    {
      // 返回关联类加载器向量中指定索引处的类加载器 (Return the class loader at the specified index in the associated class loaders vector)
      return impl_->associated_class_loaders_[index];
    }

  } // namespace impl

} // namespace class_loader
