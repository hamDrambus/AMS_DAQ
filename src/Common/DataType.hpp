/**
 * Copyright (C) 2019-2021 CERN
 *
 * DAQling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DAQling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with DAQling. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#include "DataFormat.hpp"
#include "Utils/Ers.hpp"
#include <vector>

ERS_DECLARE_ISSUE(daqling, InternalAllocationIssue,
                  "Internal allocation issue, message from exception: " << message,
                  ((const char *)message))
ERS_DECLARE_ISSUE(daqling, UninitializedData, "Internal data was nullptr.", ERS_EMPTY)
using freeptr = void (*)(void *, void *);
/**
 * DataType
 * Description: Abstract base class for DAQling DataTypes.
 * Primary purpose is abstraction for sending datastructures with the DAQling connections.
 * Date: July 2021
 */
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class DataType {
public:
  /**
   * @brief Clears inner data.
   */
  virtual void clear_inner_data() = 0;
  /**
   * @brief Reconstructs inner data.
   * @param size size of the memory region to reconstruct.
   * @param data pointer to memory region.
   */
  virtual void reconstruct(const void *data, size_t size) = 0;
  /**
   * @brief Detaches ownership of inner data.
   */
  virtual DataType *detach() {
    detach_data();
    return this;
  }; // data() pointing to memory region with data, and size() delimiting the area. (destructor of
     // DataType should no longer destruct data.)
  virtual ~DataType() = default;
  /**
   * @brief returns ptr to free function, to be used with zmq.
   */
  virtual freeptr free() = 0;
  /**
   * @brief returns hint ptr, to pass to freeptr function
   */
  virtual void *hint() { return nullptr; };
  /**
   * @brief returns size of memory region.
   */
  virtual size_t size() const = 0;
  /**
   * @brief returns pointer to memory region
   */
  virtual void *data() = 0;
  /**
   * @brief returns const pointer to memory region
   */
  virtual const void *data() const = 0;

protected:
  /**
   * @brief subclass implementation of detaching data.
   */
  virtual void detach_data() = 0;
};
template <class T> class DataFragment;
/*
 * SharedDataType
 * Description: Subclass of DataType, holding an inner datatype in a shared pointer.
 * This allows to share the ownership of the inner data, while still providing the detach
 * functionality. Date: July 2021
 */
template <class T> class SharedDataType : public DataType {
  friend class DataFragment<T>;

private:
  /**
   * @brief hint pointer to pass to free function. Holds a pointer to a shared pointer storing the
   * inner data. This makes sure that the inner data is not deallocated before the free function is
   * called.
   */
  std::shared_ptr<T> *hint_ptr;
  /**
   * @brief Shared pointer storing the inner data.
   */
  std::shared_ptr<T> data_ptr;
  /**
   * @brief boolean signifying whether the inner data can be shared by copies of this object.
   */
  bool m_shared = false;

public:
  ~SharedDataType() override = default;
  /**
   * @brief Default constructor creating empty inner data.
   **/
  SharedDataType() : DataType(), data_ptr(std::make_shared<T>()){};
  /**
   * @brief Constructor creating inner data from the parameters.
   * @param data pointer to data to reconstruct.
   * @param size size of data to reconstruct.
   **/
  SharedDataType(const void *data, const size_t size) { reconstruct(data, size); }
  /**
   * @brief Move constructor.
   **/
  SharedDataType(SharedDataType<T> &&rhs) noexcept : DataType() {
    data_ptr = rhs.data_ptr;
    rhs.data_ptr = nullptr;
  }
  /**
   * @brief Move construct from DataFragment<T>.
   **/
  SharedDataType(DataFragment<T> &&rhs) noexcept {
    rhs.detach();
    data_ptr = std::shared_ptr<T>(rhs.data_ptr);
  }
  /**
   * @brief Move assignment.
   **/
  SharedDataType &operator=(SharedDataType<T> &&rhs) noexcept {
    if (this == &rhs) {
      return *this;
    }
    data_ptr = rhs.data_ptr;
    rhs.data_ptr = nullptr;
    return *this;
  }
  /**
   * @brief Move assignment from DataFragment<T>.
   **/
  SharedDataType &operator=(DataFragment<T> &&rhs) noexcept {
    if (this == &rhs) {
      return *this;
    }
    rhs.detach();
    data_ptr = std::shared_ptr<T>(rhs.data_ptr);
    return *this;
  }
  /**
   * @brief Copy constructor.
   **/
  SharedDataType(const SharedDataType<T> &rhs) : DataType() {
    if (rhs.m_shared) {
      data_ptr = rhs.data_ptr;
    } else if (rhs.data_ptr != nullptr) {
      data_ptr = std::make_shared<T>(rhs.data(), rhs.size());
    } else {
      data_ptr = nullptr;
    }
  }
  /**
   * @brief Copy construct from DataFragment<T>.
   **/
  SharedDataType(const DataFragment<T> &rhs) noexcept {
    data_ptr = std::make_shared<T>(rhs.data(), rhs.size());
  }
  /**
   * @brief Copy assignemnt.
   **/
  SharedDataType<T> &operator=(const SharedDataType<T> &rhs) {
    if (this == &rhs) {
      return *this;
    }
    if (rhs.m_shared) {
      data_ptr = rhs.data_ptr;
    } else if (rhs.data_ptr != nullptr) {
      data_ptr = std::make_shared<T>(rhs.data(), rhs.size());
    } else {
      data_ptr = nullptr;
    }
    return *this;
  }
  /**
   * @brief Copy assignment from DataFragment<T>.
   **/
  SharedDataType<T> &operator=(const DataFragment<T> &rhs) {
    if (this == &rhs) {
      return *this;
    }
    if (rhs.data() != nullptr) {
      data_ptr = std::make_shared<T>(rhs.data(), rhs.size());
    } else {
      data_ptr = nullptr;
    }
    return *this;
  }
  /**
   * @brief Reconstruct inner data.
   * @param data pointer to data.
   * @param size size of data.
   **/
  void reconstruct(const void *data, const size_t size) override {
    data_ptr = std::make_shared<T>(data, size);
  }

  /**
   * @brief Compound assignment - addition of inner data.
   * @param rhs SharedDataType storing inner data to be added to this inner data.
   **/
  SharedDataType<T> &operator+=(SharedDataType<T> &rhs) {
    if (data_ptr != nullptr && rhs.data_ptr != nullptr) {
      *(data_ptr.get()) += *(rhs.data_ptr.get());
    } else if (rhs.data_ptr != nullptr) {
      data_ptr = std::make_shared<T>(rhs.data_ptr->data(), rhs.data_ptr->size());
    }
    return *this;
  }
  bool operator==(SharedDataType<T> &rhs) { return data_ptr == rhs.data_ptr; }
  bool operator!=(SharedDataType<T> &rhs) { return data_ptr != rhs.data_ptr; }
  bool operator<(SharedDataType<T> &rhs) { return data_ptr < rhs.data_ptr; }
  bool operator>(SharedDataType<T> &rhs) { return data_ptr > rhs.data_ptr; }
  bool operator<=(SharedDataType<T> &rhs) { return data_ptr <= rhs.data_ptr; }
  bool operator>=(SharedDataType<T> &rhs) { return data_ptr >= rhs.data_ptr; }
  /**
   * @brief Make this object shared, allowing for lighter copy operations.
   **/
  void make_shared() { m_shared = true; }
  /**
   * @brief pointer semantics to access inner data.
   */
  T *operator->() { return data_ptr.get(); }
  /**
   * @brief get size of inner data.
   * @return size of inner data.
   */
  inline size_t size() const override {
    if (data_ptr == nullptr) {
      return 0;
    }
    return data_ptr->size();
  }

  /**
   * @brief Get pointer to inner data.
   * @return pointer to inner data.
   **/
  inline void *data() override {
    if (data_ptr == nullptr) {
      return nullptr;
    }
    return data_ptr->data();
  }
  /**
   * @brief Get const pointer to inner data.
   * @return const pointer to inner data.
   **/
  inline const void *data() const override {
    if (data_ptr == nullptr) {
      return nullptr;
    }
    return data_ptr->data();
  }
  /**
   * @brief Get template type pointer to inner data.
   * @return template type pointer to inner data.
   **/
  template <typename U = void *> U data() {
    static_assert(std::is_pointer<U>(), "Type parameter must be a pointer type");
    if (data_ptr == nullptr) {
      return static_cast<U>(nullptr);
    }
    return static_cast<U>(data_ptr->data());
  }
  /**
   * @brief Get const template type pointer to inner data.
   * @return const template type pointer to inner data.
   **/
  template <typename U = void *> const U data() const {
    static_assert(std::is_pointer<U>(), "Type parameter must be a pointer type");
    if (data_ptr == nullptr) {
      return static_cast<U>(nullptr);
    }
    return static_cast<U>(data_ptr->data());
  }

  /**
   * @brief Get free function pointer.
   * @return pointer to three function.
   **/
  freeptr free() override {
    return [](void *, void *hint) {
      auto *ptr = static_cast<std::shared_ptr<T> *>(hint);
      delete ptr;
    };
  }
  /**
   * @brief Clear the inner data type.
   **/
  void clear_inner_data() override { data_ptr = nullptr; }
  /**
   * @brief hint to be used by free function.
   * @return hint pointer.
   **/
  void *hint() override { return static_cast<void *>(hint_ptr); }
  /**
   * @brief get pointer to inner data.
   * @return pointer to inner data.
   **/
  T *get() { return data_ptr.get(); }

protected:
  /**
   * @brief Detach the inner data.
   * The detachment is done by creating a new shared pointer as a copy of the data_ptr.
   * The pointer to this shared pointer is stored in the hint pointer deleted in the free function.
   * Thus ensuring that the inner data is kept alive until the free method is called by whomever has
   *taken ownership of the inner data.
   **/
  void detach_data() override { hint_ptr = new std::shared_ptr<T>(data_ptr); }
};

/*
 * DataFragment
 * Description: Subclass of DataType, holding an inner datatype in a pointer.
 * Date: July 2021
 */
template <class T> class DataFragment : public DataType {
  friend class SharedDataType<T>;

private:
  /**
   * @brief Raw pointer to inner data.
   **/
  T *data_ptr;
  /**
   * @brief true if the inner data has been detached from this object.
   **/
  bool m_detached{false};

public:
  ~DataFragment() override {
    if (!m_detached) {
      delete data_ptr;
    }
  };

  /**
   * @brief Default constructor creating object with no inner data.
   **/
  DataFragment() : data_ptr(nullptr){};
  /**
   * @brief Constructor creating inner data object as a copy of the memory passed as parameters.
   * @param data pointer to data.
   * @param size size of data.
   **/
  DataFragment(const void *data, const size_t size) : data_ptr(nullptr) { reconstruct(data, size); }
  /**
   * @brief Constructor setting the pointer to the inner data.
   * @param ptr pointer to inner data.
   **/
  DataFragment(T *ptr) : data_ptr(ptr){};
  /**
   * @brief Move constructor
   **/
  DataFragment(DataFragment<T> &&rhs) noexcept {
    m_detached = rhs.m_detached;
    data_ptr = rhs.data_ptr;
    rhs.data_ptr = nullptr;
  }
  /**
   * @brief Move assignment
   **/
  DataFragment &operator=(DataFragment<T> &&rhs) noexcept {
    if (this == &rhs) {
      return *this;
    }
    if (!m_detached) {
      delete data_ptr;
    }
    data_ptr = rhs.data_ptr;
    rhs.data_ptr = nullptr;
    m_detached = rhs.m_detached;
    return *this;
  }
  /**
   * @brief Copy constructor
   **/
  DataFragment(const DataFragment<T> &rhs) {
    m_detached = false;
    if (rhs.data_ptr != nullptr) {
      data_ptr = new T(rhs.data(), rhs.size());
    } else {
      data_ptr = nullptr;
    }
  }
  /**
   * @brief Copy assignment
   **/
  DataFragment<T> &operator=(const DataFragment<T> &rhs) {
    if (this == &rhs) {
      return *this;
    }
    if (!m_detached) {
      delete data_ptr;
    }
    m_detached = false;
    if (rhs.data_ptr != nullptr) {
      data_ptr = new T(rhs.data(), rhs.size());
    } else {
      data_ptr = nullptr;
    }
    return *this;
  }
  /**
   * @brief Copy assignment of SharedDataType<T>
   **/
  DataFragment<T> &operator=(const SharedDataType<T> &rhs) {
    if (this == &rhs) {
      return *this;
    }
    if (!m_detached) {
      delete data_ptr;
    }
    m_detached = false;
    if (rhs.data_ptr != nullptr) {
      data_ptr = new T(rhs.data(), rhs.size());
    } else {
      data_ptr = nullptr;
    }
    return *this;
  }
  /**
   * @brief reconstruction of inner data.
   * @param data pointer to memory chunk to reconstruct
   * @param size size of memory to reconstruct.
   **/
  void reconstruct(const void *data, const size_t size) override {
    delete data_ptr;
    data_ptr = new T(data, size);
  }
  /**
   * @brief Compound assignment - addition of inner data.
   * @param rhs DataFragment storing inner data to be added to this inner data.
   **/
  DataFragment<T> &operator+=(DataFragment<T> &rhs) {
    if (data_ptr != nullptr && rhs.data_ptr != nullptr) {
      *data_ptr += *rhs.data_ptr;
    } else if (rhs.data_ptr != nullptr) {
      data_ptr = new T(rhs.data_ptr->data(), rhs.data_ptr->size());
    }
    return *this;
  }

  bool operator==(DataFragment<T> &rhs) { return *data_ptr == *rhs.data_ptr; }
  bool operator!=(DataFragment<T> &rhs) { return *data_ptr != *rhs.data_ptr; }
  bool operator<(DataFragment<T> &rhs) { return *data_ptr < *rhs.data_ptr; }
  bool operator>(DataFragment<T> &rhs) { return *data_ptr > *rhs.data_ptr; }
  bool operator<=(DataFragment<T> &rhs) { return *data_ptr <= *rhs.data_ptr; }
  bool operator>=(DataFragment<T> &rhs) { return *data_ptr >= *rhs.data_ptr; }

  /**
   * @brief pointer semantics to access inner data.
   */
  T *operator->() { return data_ptr; }
  /**
   * @brief get size of inner data.
   * @return size of inner data.
   */
  size_t size() const override {
    if (data_ptr == nullptr) {
      return 0;
    }
    return data_ptr->size();
  }
  /**
   * @brief Get pointer to inner data.
   * @return pointer to inner data.
   **/
  void *data() override {
    if (data_ptr == nullptr) {
      return data_ptr;
    }
    return data_ptr->data();
  }
  /**
   * @brief Get const pointer to inner data.
   * @return const pointer to inner data.
   **/
  const void *data() const override {
    if (data_ptr == nullptr) {
      return data_ptr;
    }
    return data_ptr->data();
  }
  /**
   * @brief Get template type pointer to inner data.
   * @return pointer to inner data.
   **/
  template <typename U = void *> U data() {
    static_assert(std::is_pointer<U>(), "Type parameter must be a pointer type");
    if (data_ptr == nullptr) {
      return static_cast<U>(nullptr);
    }
    return static_cast<U>(data_ptr->data());
  }
  /**
   * @brief Get const template type pointer to inner data.
   * @return const pointer to inner data.
   **/
  template <typename U = void *> const U data() const {
    static_assert(std::is_pointer<U>(), "Type parameter must be a pointer type");
    if (data_ptr == nullptr) {
      return static_cast<U>(nullptr);
    }
    return static_cast<U>(data_ptr->data());
  }
  /**
   * @brief Get free function pointer.
   * @return pointer to three function.
   **/
  freeptr free() override {
    return [](void *, void *hint) { delete static_cast<T *>(hint); };
  }

  /**
   * @brief Get hint pointer used by free function
   * @return hint pointer.
   **/
  void *hint() override { return static_cast<void *>(data_ptr); }
  /**
   * @brief Clear the inner data type.
   **/
  void clear_inner_data() override {
    if (!m_detached) {
      delete data_ptr;
    }
    data_ptr = nullptr;
  }
  /**
   * @brief get pointer to inner data.
   * @return pointer to inner data.
   **/
  T *get() { return data_ptr; }

protected:
  /**
   * @brief detach ownership of inner data from this object. Destructor will no longer delete the
   *inner object.
   **/
  void detach_data() override { m_detached = true; }
};

/**
 * DataTypeWrapper
 * Description: Wrapper class for daqling DataTypes.
 * Used in the daqling connections and queues, to erase the specific datatypes, similar to std::any.
 * However, the DataTypeWrapper allows to interract with the concrete DataType via a pointer to the
 * abstract DataType class. Thus both allowing storage of an arbitrary datatype object in queues +
 * manipulation of this object via. it's base class methods. Date: July 2021
 */
class DataTypeWrapper {
public:
  /**
   * @brief Copy constructor - deleted.
   **/
  DataTypeWrapper(const DataTypeWrapper &) = delete;
  /**
   * @brief Move constructor.
   **/
  DataTypeWrapper(DataTypeWrapper &&rhs) noexcept
      : m_datatype(rhs.m_datatype), m_data(rhs.m_data), m_size(rhs.m_size),
        m_dealloc(rhs.m_dealloc) {
    rhs.m_dealloc = false;
  }
  /**
   * @brief Default constructor.
   **/
  DataTypeWrapper() = default;
  ~DataTypeWrapper() {
    if (m_dealloc) {
      delete m_datatype;
      // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
      free(m_data);
    }
  }
  /**
   * @brief Copy assignment - deleted.
   **/
  DataTypeWrapper &operator=(const DataTypeWrapper &) = delete;
  /**
   * @brief Move assignment.
   **/
  DataTypeWrapper &operator=(DataTypeWrapper &&rhs) noexcept {
    if (&rhs == this) {
      return *this;
      // If rhs side type is already determined
    }
    if (rhs.m_datatype != nullptr) {
      std::swap(m_datatype, rhs.m_datatype);
      std::swap(m_dealloc, rhs.m_dealloc);
      // If this already knows its type
    } else if (this->m_datatype != nullptr && rhs.m_data != nullptr) {
      m_datatype->reconstruct(rhs.m_data, rhs.m_size);
      if (rhs.m_dealloc) {
        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
        free(rhs.m_data);
      }
      rhs.m_data = nullptr;
    } else {
      std::swap(m_data, rhs.m_data);
      std::swap(m_size, rhs.m_size);
      std::swap(m_dealloc, rhs.m_dealloc);
    }
    return *this;
  };
  /**
   * @brief Construct with Datatype reference.
   * @param ptr DataType reference. Inner data should always be empty, to avoid large copy
   *operations.
   **/
  template <class T> DataTypeWrapper(T &ptr) {
    static_assert(std::is_base_of<DataType, T>::value);
    m_datatype = new T(ptr);
    m_dealloc = true;
  }
  /**
   * @brief Construct with Datatype rhs reference.
   * @param ptr DataType reference. Inner data should always be empty, to avoid large copy
   *operations.
   **/
  // NOLINTNEXTLINE(misc-forwarding-reference-overload)
  template <class T> DataTypeWrapper(T &&ptr) {
    static_assert(std::is_base_of<DataType, T>::value);
    m_datatype = new T(std::forward<T>(ptr));
    m_dealloc = true;
  }
  /**
   * @brief Get pointer to stored datatype
   * @return pointer to stored datatype.
   **/
  DataType *getDataTypePtr() { return m_datatype; }
  /**
   * @brief Reconstructs datatype object, if type is known, otherwise stores raw memory chunk.
   * @param data pointer to memory.
   * @param size size of memory.
   **/
  void reconstruct_or_store(void *data, size_t size) {
    if (m_datatype != nullptr) {
      m_datatype->reconstruct(data, size);
    } else {
      if (m_dealloc) {
        // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
        free(m_data);
      }
      // NOLINTNEXTLINENEXTLINE(cppcoreguidelines-no-malloc)
      m_data = malloc(size);
      memcpy(m_data, data, size);
      m_size = size;
    }
    m_dealloc = true;
  }
  /**
   * @brief Transfer the stored datatype/memory chunk into the datatype object passed as parameter.
   * @param ref DataType object to transfer into.
   **/
  template <typename T> void transfer_into(T &ref) {
    static_assert(std::is_base_of<DataType, T>::value);
    if (m_datatype && m_dealloc) {
      ref = std::move(*(static_cast<T *>(m_datatype)));
      delete m_datatype;
      m_datatype = nullptr;
    } else if (m_data) {
      ref.reconstruct(m_data, m_size);
    } else {
      ERS_WARNING("Nothing to transfer!!!");
    }
  }

private:
  DataType *m_datatype{nullptr};
  void *m_data{nullptr};
  size_t m_size{0};
  bool m_dealloc = false;
};