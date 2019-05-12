//
// Copyright 2019 Rokas Kupstys
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
#pragma once


#include <string>
#include <unordered_map>


namespace ser
{

class ArchiveIterator;

namespace detail
{

// Interface for archive-specific iterators.
class IArchiveIterator
{
public:
    /// Returns size of array or 0 if this is not an array iterator.
    virtual int Size() const = 0;
    /// Returns iterator pointing to specified key in current map container or null iterator if iterating not a map.
    virtual ArchiveIterator Find(const std::string& key) = 0;
    /// Returns true if iterator has reached end of current container.
    virtual bool AtEnd() const = 0;

protected:
    virtual ~IArchiveIterator() = default;
    /// Copies itself to destination container.
    virtual void Copy(ArchiveIterator& destination) const = 0;
    // These operators are accessed through wrapper operators of ArchiveIterator.
    virtual ArchiveIterator operator[](int index) = 0;
    // This operator is supposed to return owning ArchiveIterator&, but internal iterators implementing this interface
    // are not aware of their owners. For this reason real operator does not return a value and wrapper operator in
    // ArchiveIterator class returns a reference to itself.
    virtual void operator++() = 0;

    friend class ::ser::ArchiveIterator;
};

template<typename T>
static const char* type_name()
{
#ifdef _MSC_VER
    return __FUNCDNAME__;
#else
    return __PRETTY_FUNCTION__;
#endif
}

}   // namespace detail

// Forwarding iterator. Runtime polymorphism without dynamic memory allocation. This will serve as universal iterator
// for both both arrays and objects.
class ArchiveIterator
{
public:
    /// Construct a null iterator. Any iteractions with it will be noop.
    ArchiveIterator() = default;

    /// Copy-construct from another iterator.
    ArchiveIterator(const ArchiveIterator& other)
    {
        // Copy() will invoke Construct(*this, *other.Get()) which will copy-construct internal iterator from other
        // object into this one.
        other.Get()->Copy(*this);
    }

    /// Destruct underlying iterator.
    ~ArchiveIterator()
    {
        if (!is_null_)
            Get()->~IArchiveIterator();
    }

    template<typename T, typename... Args>
    static void Construct(ArchiveIterator& destination, Args&&... args)
    {
        new(destination.storage_) T(args...);
        destination.is_null_ = false;
    }

    template<typename T, typename... Args>
    static ArchiveIterator ConstructR(Args&&... args)
    {
        ArchiveIterator result;
        Construct<T>(result, args...);
        return result;
    }

    /// Returns instance of internal archive iterator.
    detail::IArchiveIterator* Get()                            { return reinterpret_cast<      detail::IArchiveIterator*>(storage_); }
    /// Returns instance of internal archive iterator.
    const detail::IArchiveIterator* Get() const                { return reinterpret_cast<const detail::IArchiveIterator*>(storage_); }
    /// Returns instance of internal archive iterator.
    detail::IArchiveIterator* operator->()                     { return Get(); }
    /// Returns instance of internal archive iterator.
    const detail::IArchiveIterator* operator->() const         { return Get(); }
    /// Returns true if iterator is not null and not at an end.
    operator bool() const                              { return !is_null_ && !Get()->AtEnd(); }                         // NOLINT(google-explicit-constructor)
    /// Returns new archive iterator at specified index. Returns null iterator if this instance is not iterating an array.
    ArchiveIterator operator[](int index)              { return Get()->operator[](index); }
    /// Returns new archive iterator at specified key. Returns null iterator if this instance is not iterating a map.
    ArchiveIterator operator[](const std::string& key) { return Get()->Find(key); }
    /// Returns new archive iterator at specified key. Returns null iterator if this instance is not iterating a map.
    ArchiveIterator operator[](const char* key)        { return Get()->Find(key); }
    /// Increments this iterator in-place and returns reference to itself.
    ArchiveIterator& operator++()                      { Get()->operator++(); return *this; }
    /// Returns a copy of current iterator and increments this instance afterwards.
    ArchiveIterator operator++(int i)                                                                                   // NOLINT(cert-dcl21-cpp)
    {
        ArchiveIterator result;
        Get()->Copy(result);
        operator++();
        return result;
    }

protected:
    /// Flag indicating that
    bool is_null_ = false;
    /// Static storage for dynamic iterator object.
    uint8_t storage_[64]{};

public:
    /// Max size of internal iterator. Use static_assert() to verify object size against this.
    static const unsigned StorageSize = sizeof(storage_);
};

class Archive
{
public:
    /// Type of container that can be iterated.
    enum ContainerType
    {
        Array,
        Map,
    };

    using UserTypeSerializers = std::unordered_map<const char*, bool(*)(Archive*, ArchiveIterator&, void*)>;

    /// Begin iteration of a root container.
    virtual ArchiveIterator Begin(ContainerType type) = 0;
    /// Begin iteration of a subcontainer at specified iterator.
    virtual ArchiveIterator Begin(ArchiveIterator&& it, ContainerType type) = 0;

    virtual bool Serialize(ArchiveIterator&& it, bool& value) = 0;
    virtual bool Serialize(ArchiveIterator&& it, int8_t& value) = 0;
    virtual bool Serialize(ArchiveIterator&& it, uint8_t& value) = 0;
    virtual bool Serialize(ArchiveIterator&& it, int16_t& value) = 0;
    virtual bool Serialize(ArchiveIterator&& it, uint16_t& value) = 0;
    virtual bool Serialize(ArchiveIterator&& it, int32_t& value) = 0;
    virtual bool Serialize(ArchiveIterator&& it, uint32_t& value) = 0;
    virtual bool Serialize(ArchiveIterator&& it, int64_t& value) = 0;
    virtual bool Serialize(ArchiveIterator&& it, uint64_t& value) = 0;
    virtual bool Serialize(ArchiveIterator&& it, float& value) = 0;
    virtual bool Serialize(ArchiveIterator&& it, double& value) = 0;
    virtual bool Serialize(ArchiveIterator&& it, std::string& value) = 0;
    virtual bool Serialize(ArchiveIterator&& it, const char* typeName, void* value) = 0;

    /// Serialize user-defined type. Type must be registered with SER_USER_TYPE_SERIALIZER() macro.
    template<typename T>
    bool Serialize(ArchiveIterator&& it, T& value)
    {
        return Serialize((ArchiveIterator&&)it, detail::type_name<T>(), (void*)&value);
    }
};

// Macro that implements user type serialization in format-specific archives. Simply add this macro to class body.
#define SER_USER_CONTAINER(Type)                                                                      \
public:                                                                                               \
    bool Serialize(ArchiveIterator&& it, const char* typeName, void* value) override                  \
    {                                                                                                 \
        if (!it)                                                                                      \
            return false;                                                                             \
        return GetSerializers()[typeName](this, it, value);                                           \
    }                                                                                                 \
                                                                                                      \
    template<typename T>                                                                              \
    static void RegisterSerializer(bool(*serializer)(Archive*, ArchiveIterator&, void*))              \
    {                                                                                                 \
        GetSerializers()[detail::type_name<T>()] = serializer;                                        \
    }                                                                                                 \
                                                                                                      \
private:                                                                                              \
    static UserTypeSerializers& GetSerializers()                                                      \
    {                                                                                                 \
        static UserTypeSerializers serializers_;                                                      \
        return serializers_;                                                                          \
    }                                                                                                 \
public:                                                                                               \

// Macro that registers a serialization function for custom user type.
#define SER_USER_TYPE_SERIALIZER(SubArchive, Type, Function)                                          \
    SubArchive::RegisterSerializer<Type>([](Archive* archive, ArchiveIterator& it, void* value) {     \
        return Function(*static_cast<SubArchive*>(archive), *static_cast<SubArchive::Iterator*>(it.Get()), *(Type*)value);\
    })                                                                                                \

}   // namespace ser
