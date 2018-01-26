#pragma once
#include <stdint.h>
#include <type_traits>
#include <string.h>
#include <limits>
#include <malloc.h>
#include <utility>

// Probably a good idea to replace this!
#define ASSERT(a) do { if (!(a)) { int* x = nullptr; *x = 5; } } while (0)

template<typename T>
struct DefaultAllocatorT
{
    static T* Allocate(uint32_t numItems) { return static_cast<T*>(malloc(sizeof(T)*numItems)); }
    static void Free(T* data) { free(data); }
};

template<typename CountType, typename ObjectType, typename Allocator, bool IsCopyable = std::is_copy_constructible<ObjectType>::value>
class BaseArray
{
public:
    typedef ObjectType* iterator;
    typedef const ObjectType* const_iterator;
    typedef ObjectType* Iterator;
    typedef const ObjectType* ConstIterator;

    BaseArray()
        : m_Data(nullptr)
        , m_Size(0)
        , m_Capacity(0)
        , m_OwnsData(true)
        , m_PreserveOrder(false)
    {
    }
    explicit BaseArray(CountType capacity)
        : BaseArray()
    {
        Reserve(capacity);
    }
    BaseArray(const BaseArray& other)
        : BaseArray()
    {
        Reallocate(other.m_Size);
        CopyFrom(other.m_Data, other.m_Size);
        m_Size = other.m_Size;
        m_PreserveOrder = other.m_PreserveOrder;
    }

    BaseArray(BaseArray&& other)
        : BaseArray()
    {
        if (other.m_OwnsData)
        {
            std::swap(m_Data, other.m_Data);
            std::swap(m_Capacity, other.m_Capacity);
            std::swap(m_Size, other.m_Size);
            std::swap(m_Flags, other.m_Flags);
        }
        else
        {
            Reallocate(other.m_Size);
            MoveFrom(other.m_Data, other.m_Size);
            m_PreserveOrder = other.m_PreserveOrder;
        }
    }

    BaseArray(ObjectType* data, CountType numElements, CountType maxCapacity, bool ownsData)
        : BaseArray()
    {
        m_Data = data;
        m_Size = numElements;
        m_Capacity = maxCapacity;
        m_OwnsData = ownsData;
    }

    BaseArray(std::initializer_list<ObjectType>&& data)
        : BaseArray()
    {
        Reserve(static_cast<CountType>(data.size()));
        for (auto& object : data)
        {
            Push(std::move(object));
        }
    }

    void Push(std::initializer_list<ObjectType>&& data)
    {
        Reserve(static_cast<CountType>(data.size()) + m_Size);
        for (auto& obj : data)
        {
            Push(std::move(obj));
        }
    }

    ~BaseArray()
    {
        Clear();
        if (m_OwnsData)
        {
            Allocator::Free(m_Data);
        }
    }

    BaseArray& operator=(const BaseArray& other)
    {
        Clear();
        Reserve(other.m_Size);
        CopyFrom(other.m_Data, other.m_Size);
        m_Size = other.m_Size;
        return *this;
    }

    BaseArray& operator=(BaseArray&& other)
    {
        Clear();
        if (other.m_OwnsData)
        {
            std::swap(m_Flags, other.m_Flags);
            std::swap(m_Data, other.m_Data);
            std::swap(m_Capacity, other.m_Capacity);
            std::swap(m_Size, other.m_Size);
        }
        else
        {
            Reserve(other.m_Size);
            for (CountType i = 0; i < other.m_Size; ++i)
            {
                new (&m_Data[i]) ObjectType(std::move(other.m_Data[i]));
            }
            m_Size = other.m_Size;
        }
        return *this;
    }

    Iterator begin() { return m_Data; }
    Iterator end() { return m_Data + m_Size; }
    ConstIterator begin() const { return m_Data; }
    ConstIterator end() const { return m_Data + m_Size; }

    CountType Size() const { return m_Size; }
    CountType Capacity() const { return m_Capacity; }
    void Reserve(CountType capacity)
    {
        if (m_Capacity < capacity)
            Reallocate(capacity);
    }
    template<typename U = ObjectType>
    typename std::enable_if<std::is_same<U, ObjectType>::value && std::is_pod<U>::value, void>::type
        Clear() { m_Size = 0; }

    template<typename U = ObjectType>
    typename std::enable_if<std::is_same<U, ObjectType>::value && !std::is_pod<U>::value, void>::type
        Clear()
    {
        for (CountType i = 0; i < m_Size; ++i)
        {
            auto& obj = m_Data[i];
            obj.~ObjectType();
        }
        m_Size = 0;
    }

    template<typename U = ObjectType>
    typename std::enable_if<!std::is_pod<U>::value, void>::type Resize(CountType newSize)
    {
        if (newSize < m_Size)
        {
            for (CountType i = newSize; i < m_Size; ++i)
            {
                auto& obj = m_Data[i];
                obj.~ObjectType();
            }
        }
        else if (newSize > m_Size)
        {
            auto oldSize = m_Size;
            Reserve(newSize);
            for (CountType i = oldSize; i < newSize; ++i)
            {
                auto& obj = m_Data[i];
                new (&obj) ObjectType();
            }
        }
        m_Size = newSize;
    }

    template<typename U = ObjectType>
    typename std::enable_if<std::is_pod<U>::value, void>::type Resize(CountType newSize)
    {
        Reserve(newSize);
        m_Size = newSize;
    }

    ObjectType& operator[](CountType index) { ASSERT(index < m_Size); return m_Data[index]; }
    const ObjectType& operator[](CountType index) const { ASSERT(index < m_Size); return m_Data[index]; }
    const ObjectType* GetBuffer() const { return m_Data; }
    ObjectType* GetBuffer() { return m_Data; }

    template<typename U = ObjectType>
    typename std::enable_if<std::is_pod<U>::value, void>::type Push(const ObjectType& object)
    {
        if (m_Size + 1 > m_Capacity)
            Reallocate(m_Size + 1);
        memcpy(&m_Data[m_Size], &object, sizeof(ObjectType));
        ++m_Size;
    }

    template<typename U = ObjectType>
    typename std::enable_if<!std::is_pod<U>::value, void>::type Push(const ObjectType& object)
    {
        if (m_Size + 1 > m_Capacity)
            Reallocate(m_Size + 1);
        new (&m_Data[m_Size]) ObjectType(object);
        ++m_Size;
    }

    template<typename U = ObjectType>
    typename std::enable_if<!std::is_pod<U>::value, void>::type Push(ObjectType&& object)
    {
        if (m_Size + 1 > m_Capacity)
            Reallocate(m_Size + 1);
        new (&m_Data[m_Size]) ObjectType(std::move(object));
        ++m_Size;
    }

    template<typename U = ObjectType>
    typename std::enable_if<std::is_pod<U>::value, void>::type Pop()
    {
        ASSERT(m_Size > 0);
        --m_Size;
    }

    template<typename U = ObjectType>
    typename std::enable_if<!std::is_pod<U>::value, void>::type Pop()
    {
        ASSERT(m_Size > 0);
        m_Data[m_Size - 1].~ObjectType();
        --m_Size;
    }

    ObjectType& First()
    {
        ASSERT(m_Size > 0);
        return m_Data[0];
    }

    const ObjectType First() const
    {
        ASSERT(m_Size > 0);
        return m_Data[0];
    }

    ObjectType& Last()
    {
        ASSERT(m_Size > 0);
        return m_Data[m_Size - 1];
    }

    const ObjectType& Last() const
    {
        ASSERT(m_Size > 0);
        return m_Data[m_Size - 1];
    }

    ObjectType& Grow()
    {
        if (m_Size + 1 > m_Capacity)
            Reallocate(m_Size + 1);
        new (&m_Data[m_Size]) ObjectType();
        return m_Data[m_Size++];
    }

    bool Remove(const ObjectType& obj)
    {
        for (CountType i = 0; i < m_Size; ++i)
        {
            if (m_Data[i] == obj)
            {
                RemoveAt(i);
                return true;
            }
        }
        return false;
    }

    void RemoveAt(CountType index)
    {
        ASSERT(index < m_Size);
        if (!m_PreserveOrder)
        {
            std::swap(m_Data[index], m_Data[--m_Size]);
        }
        else
        {
            m_Data[index].~ObjectType();
            for (CountType i = index; i < m_Size - 1; ++i)
            {
                m_Data[i] = std::move(m_Data[i + 1]);
            }
            --m_Size;
        }
    }

    template<typename U = ObjectType>
    typename std::enable_if<!std::is_pod<U>::value>::type
        Insert(CountType index, const ObjectType& obj)
    {
        ASSERT(index <= m_Size);
        if (m_Size + 1 > m_Capacity)
            Reallocate(m_Size + 1);

        if (index < m_Size)
        {
            new (&m_Data[m_Size]) ObjectType{ std::move(m_Data[m_Size - 1]) };
            for (CountType i = 1; i < m_Size - index; ++i)
            {
                m_Data[m_Size - i] = std::move(m_Data[m_Size - i - 1]);
            }
            m_Data[index] = obj;
        }
        else
        {
            new (&m_Data[m_Size]) ObjectType{ obj };
        }
        ++m_Size;
    }

    template<typename U = ObjectType>
    typename std::enable_if<!std::is_pod<U>::value>::type
        Insert(CountType index, ObjectType&& obj)
    {
        ASSERT(index <= m_Size);
        if (m_Size + 1 > m_Capacity)
            Reallocate(m_Size + 1);

        if (index < m_Size)
        {
            new (&m_Data[m_Size]) ObjectType{ std::move(m_Data[m_Size - 1]) };
            for (CountType i = 1; i < m_Size - index; ++i)
            {
                m_Data[m_Size - i] = std::move(m_Data[m_Size - i - 1]);
            }
            m_Data[index] = std::move(obj);
        }
        else
        {
            new (&m_Data[m_Size]) ObjectType{ std::move(obj) };
        }
        ++m_Size;
    }

    template<typename U = ObjectType>
    typename std::enable_if<std::is_pod<U>::value>::type
        Insert(CountType index, const ObjectType& obj)
    {
        ASSERT(index <= m_Size);
        if (m_Size + 1 > m_Capacity)
            Reallocate(m_Size + 1);

        if (index < m_Size)
        {
            memmove(m_Data + index + 1, m_Data + index, (m_Size - index) * sizeof(ObjectType));
        }
        m_Data[index] = obj;
        ++m_Size;
    }

    bool GetPreserveOrder() const { return m_PreserveOrder; }
    void SetPreserveOrder(bool preserve) { m_PreserveOrder = preserve; }

    bool Empty() const
    {
        return m_Size == 0;
    }
private:
    template<typename U = ObjectType>
    typename std::enable_if<std::is_same<U, ObjectType>::value && std::is_trivially_move_constructible<U>::value>::type
        Reallocate(CountType newSize)
    {
        if (newSize <= m_Capacity)
            return;
        ASSERT(newSize < std::numeric_limits<CountType>::max());
        auto newData = Allocator::Allocate(newSize);
        if (m_Data != nullptr)
        {
            memcpy(newData, m_Data, m_Size * sizeof(ObjectType));
            if (m_OwnsData)
                Allocator::Free(m_Data);
        }
        m_Data = newData;
        m_OwnsData = 1;
        m_Capacity = newSize;
    }

    template<typename U = ObjectType>
    typename std::enable_if<std::is_same<U, ObjectType>::value && !std::is_trivially_move_constructible<U>::value>::type
        Reallocate(CountType newSize)
    {
        if (newSize <= m_Capacity)
            return;
        ASSERT(newSize < std::numeric_limits<CountType>::max());
        auto newData = Allocator::Allocate(newSize);
        if (m_Data != nullptr)
        {
            for (CountType i = 0; i < m_Size; ++i)
            {
                new (&newData[i]) ObjectType(std::move(m_Data[i]));
            }
            if (m_OwnsData)
            {
                for (CountType i = 0; i < m_Size; ++i)
                {
                    m_Data[i].~ObjectType();
                }
                Allocator::Free(m_Data);
            }

        }
        m_Data = newData;
        m_OwnsData = 1;
        m_Capacity = newSize;
    }

    void MoveFrom(ObjectType* values, CountType numItems)
    {
        for (CountType i = 0; i < numItems; ++i)
        {
            new (&m_Data[i]) ObjectType(std::move(values[i]));
        }
    }

    template<typename U = ObjectType>
    typename std::enable_if<std::is_same<U, ObjectType>::value && std::is_pod<U>::value && std::is_copy_constructible<U>::value, void>::type
        CopyFrom(const U* values, CountType numItems)
    {
        memcpy(m_Data, values, numItems * sizeof(U));
    }

    template<typename U = ObjectType>
    typename std::enable_if<std::is_same<U, ObjectType>::value && !std::is_pod<U>::value && std::is_copy_constructible<U>::value, void>::type
        CopyFrom(const U* values, CountType numItems)
    {
        for (CountType i = 0; i < numItems; ++i)
        {
            new (&m_Data[i]) U(values[i]);
        }
    }

    template<typename U = ObjectType>
    typename std::enable_if<std::is_same<U, ObjectType>::value && std::is_pod<U>::value, void>::type
        MoveFrom(U* values, CountType numItems)
    {
        memcpy(m_Data, values, numItems * sizeof(U));
    }

    template<typename U = ObjectType>
    typename std::enable_if<std::is_same<U, ObjectType>::value && !std::is_pod<U>::value, void>::type
        MoveFrom(U* values, CountType numItems)
    {
        for (CountType i = 0; i < numItems; ++i)
        {
            new (&m_Data[i]) U(std::move(values[i]));
        }
    }


    ObjectType* m_Data;
    CountType m_Size;
    CountType m_Capacity;
    union
    {
        uint8_t m_Flags;
        struct
        {
            uint8_t m_OwnsData : 1;
            uint8_t m_PreserveOrder : 1;
        };
    };
};

template<typename CountType, typename ObjectType, typename Allocator>
class BaseArray<CountType, ObjectType, Allocator, false> : public BaseArray<CountType, ObjectType, Allocator, true>
{
    using BaseArray<CountType, ObjectType, Allocator, true>::BaseArray;
public:
    BaseArray() = default;
    BaseArray(const BaseArray&) = delete;
    BaseArray(BaseArray&&) = default;
    BaseArray& operator=(const BaseArray&) = delete;
    BaseArray& operator=(BaseArray&&) = default;
};

template<typename ObjectType, typename Allocator = DefaultAllocatorT<ObjectType>>
using Array = BaseArray<uint16_t, ObjectType, Allocator>;

template<typename ObjectType, typename Allocator = DefaultAllocatorT<ObjectType>>
using BigArray = BaseArray<uint32_t, ObjectType, Allocator>;

template<typename ObjectType, uint16_t FIXED_SIZE, typename Allocator = DefaultAllocatorT<ObjectType>>
class InplaceArray : public BaseArray<uint16_t, ObjectType, Allocator>
{
    typedef BaseArray<uint16_t, ObjectType, Allocator> super;
public:
    InplaceArray() : super(reinterpret_cast<ObjectType*>(&m_FixedBuffer), 0, FIXED_SIZE, false) {}
    InplaceArray(const InplaceArray&) = default;
    InplaceArray(InplaceArray&& other) = default;
    ~InplaceArray() = default;
    InplaceArray(const BaseArray<uint16_t, ObjectType, Allocator>& other)
        : super(other)
    {
    }
    InplaceArray& operator=(const InplaceArray&) = default;
    InplaceArray& operator=(InplaceArray&& other) = default;
private:
    typename std::aligned_storage<sizeof(ObjectType)*FIXED_SIZE, alignof(ObjectType)>::type m_FixedBuffer;
};

template<typename ObjectType, typename CountType1, typename CountType2, typename Allocator1, typename Allocator2>
bool operator==(const BaseArray<CountType1, ObjectType, Allocator1>& a1, const BaseArray<CountType2, ObjectType, Allocator2>& a2)
{
    if (a1.Size() != a2.Size())
        return false;
    typedef typename std::conditional<sizeof(CountType1) >= sizeof(CountType2), CountType1, CountType2>::type count_t;
    for (count_t i = 0; i < static_cast<count_t>(a1.Size()); ++i)
    {
        if (a1[static_cast<CountType1>(i)] != a2[static_cast<CountType2>(i)])
            return false;
    }
    return true;
}

template<typename ObjectType, typename CountType1, typename CountType2, typename Allocator1, typename Allocator2>
bool operator!=(const BaseArray<CountType1, ObjectType, Allocator1>& a1, const BaseArray<CountType2, ObjectType, Allocator2>& a2)
{
    return !(a1 == a2);
}
