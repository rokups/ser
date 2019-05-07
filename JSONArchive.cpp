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
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "JSONArchive.h"

namespace ser
{

// ---------------------- JSONArchive::InputIterator ----------------------

ArchiveIterator JSONArchive::InputIterator::Construct(rapidjson::Value* container,
    rapidjson::Document::AllocatorType& allocator, size_t index)
{
    return ArchiveIterator::ConstructR<InputIterator>(container, allocator, index);
}

void JSONArchive::InputIterator::Copy(ArchiveIterator& destination) const
{
    ArchiveIterator::Construct<InputIterator>(destination, *this);
}

JSONArchive::InputIterator::InputIterator(rapidjson::Value* container, rapidjson::Document::AllocatorType& allocator)
    : container_(container)
    , allocator_(allocator)
{
}

JSONArchive::InputIterator::InputIterator(rapidjson::Value* container, rapidjson::Document::AllocatorType& allocator, size_t index)
    : container_(container)
    , index_(index)
    , allocator_(allocator)
{
}

rapidjson::Value* JSONArchive::InputIterator::Current()
{
    if (container_ == nullptr)
        return {};

    if (container_->IsArray())
        return container_->Begin() + index_;

    if (container_->IsObject())
        return &(container_->MemberBegin() + index_)->value;

    return container_;
}

int JSONArchive::InputIterator::Size() const
{
    if (container_ == nullptr)
        return 0;

    if (container_->IsArray())
        return container_->Size();

    if (container_->IsObject())
        return std::distance(container_->MemberBegin(), container_->MemberEnd());

    return 1;
}

ArchiveIterator JSONArchive::InputIterator::Find(const std::string& key)
{
    if (container_ == nullptr || !container_->IsObject())
        return {};

    auto it = container_->FindMember(key.c_str());
    if (it != container_->MemberEnd())
        return Construct(container_, allocator_, std::distance(container_->MemberBegin(), it));

    return {};
}

bool JSONArchive::InputIterator::AtEnd() const
{
    if (container_ == nullptr)
        return true;

    return Size() <= index_;
}

ArchiveIterator JSONArchive::InputIterator::operator[](int index)
{
    if (!container_ || !container_->IsArray() || index >= container_->Size())
        return {};

    return Construct(container_, allocator_, + index);
}

void JSONArchive::InputIterator::operator++()
{
    if (container_ == nullptr)
        return;

    ++index_;
}

// ---------------------- JSONOutputArchive::XMLOutputIterator ----------------------

ArchiveIterator JSONOutputArchive::OutputIterator::Construct(rapidjson::Value* container,
    rapidjson::Document::AllocatorType& allocator, size_t index)
{
    return ArchiveIterator::ConstructR<OutputIterator>(container, allocator_, index);
}

void JSONOutputArchive::OutputIterator::Copy(ArchiveIterator& destination) const
{
    ArchiveIterator::Construct<OutputIterator>(destination, *this);
}

JSONOutputArchive::OutputIterator::OutputIterator(rapidjson::Value* container, rapidjson::Document::AllocatorType& allocator)
    : InputIterator(container, allocator)
{
}

JSONOutputArchive::OutputIterator::OutputIterator(rapidjson::Value* container, rapidjson::Document::AllocatorType& allocator, size_t index)
    : InputIterator(container, allocator, index)
{
}

rapidjson::Value* JSONOutputArchive::OutputIterator::Current()
{
    // Automatically expanding
    if (container_ != nullptr && container_->IsArray())
    {
        while (index_ >= Size())
            container_->PushBack({}, allocator_);
    }

    return InputIterator::Current();
}

ArchiveIterator JSONOutputArchive::OutputIterator::operator[](int index)
{
    if (container_ == nullptr)
        return {};

    // Access converts container to array
    if (!container_->IsArray())
        container_->SetArray();

    while (index < container_->Size())
        container_->GetArray().PushBack(rapidjson::Value(), allocator_);

    return Construct(container_, allocator_, index);
}

void JSONOutputArchive::OutputIterator::operator++()
{
    if (container_ == nullptr)
        return;

    // Access converts container to array
    if (!container_->IsArray())
        container_->SetArray();

    while (index_ < container_->Size())
        container_->PushBack({}, allocator_);

    InputIterator::operator++();
}

ArchiveIterator JSONOutputArchive::OutputIterator::Find(const std::string& key)
{
    if (container_ == nullptr || !container_->IsObject())
        return {};

    auto it = container_->FindMember(key.c_str());
    if (it == container_->MemberEnd())
    {
        rapidjson::Value k;
        k.SetString(key.c_str(), allocator_);
        container_->AddMember(k, rapidjson::Value{}, allocator_);
        it = container_->FindMember(key.c_str());
    }

    return Construct(container_, allocator_, std::distance(container_->MemberBegin(), it));
}

bool JSONOutputArchive::OutputIterator::AtEnd() const
{
    // Output array can always be appended
    return container_ == nullptr;
}

// ---------------------- JSONOutputArchive ----------------------

ArchiveIterator JSONOutputArchive__BeginHelper(rapidjson::Document::AllocatorType& allocator, rapidjson::Value* target, Archive::ContainerType type)
{
    if (type == Archive::Array && !target->IsArray())
        target->SetArray();
    else if (type == Archive::Map && !target->IsObject())
        target->SetObject();

    return ArchiveIterator::ConstructR<JSONOutputArchive::OutputIterator>(target, allocator);
}

ser::ArchiveIterator ser::JSONOutputArchive::Begin(ser::ArchiveIterator&& it, ser::Archive::ContainerType type)
{
    return JSONOutputArchive__BeginHelper(root_.GetAllocator(), static_cast<InputIterator*>(it.Get())->Current(), type);  // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
}

ser::ArchiveIterator ser::JSONOutputArchive::Begin(ser::Archive::ContainerType type)
{
    return JSONOutputArchive__BeginHelper(root_.GetAllocator(), &root_, type);
}

std::string ser::JSONOutputArchive::ToString() const
{
    using StringBuffer = rapidjson::GenericStringBuffer<rapidjson::UTF8<> >;
    StringBuffer buffer;
    rapidjson::PrettyWriter<StringBuffer> writer(buffer);
    writer.SetIndent(' ', 4);
    root_.Accept(writer);
    return buffer.GetString();
}

template<typename T>
bool JSONOutputArchive__SerializeValueHelper(rapidjson::Document::AllocatorType& allocator, ArchiveIterator& it, T value)
{
    if (!it)
        return false;

    if (auto* current = static_cast<JSONArchive::InputIterator*>(it.Get())->Current())
    {
        current->Set(value, allocator);
        return true;
    }
    return false;
}

bool JSONOutputArchive::Serialize(ArchiveIterator&& it, bool& value)
{
    return JSONOutputArchive__SerializeValueHelper(root_.GetAllocator(), it, value);
}

bool JSONOutputArchive::Serialize(ArchiveIterator&& it, int8_t& value)
{
    return JSONOutputArchive__SerializeValueHelper(root_.GetAllocator(), it, (int)value);
}

bool JSONOutputArchive::Serialize(ArchiveIterator&& it, uint8_t& value)
{
    return JSONOutputArchive__SerializeValueHelper(root_.GetAllocator(), it, (unsigned)value);
}

bool JSONOutputArchive::Serialize(ArchiveIterator&& it, int16_t& value)
{
    return JSONOutputArchive__SerializeValueHelper(root_.GetAllocator(), it, (int)value);
}

bool JSONOutputArchive::Serialize(ArchiveIterator&& it, uint16_t& value)
{
    return JSONOutputArchive__SerializeValueHelper(root_.GetAllocator(), it, (unsigned)value);
}

bool JSONOutputArchive::Serialize(ArchiveIterator&& it, int32_t& value)
{
    return JSONOutputArchive__SerializeValueHelper(root_.GetAllocator(), it, value);
}

bool JSONOutputArchive::Serialize(ArchiveIterator&& it, uint32_t& value)
{
    return JSONOutputArchive__SerializeValueHelper(root_.GetAllocator(), it, value);
}

bool JSONOutputArchive::Serialize(ArchiveIterator&& it, int64_t& value)
{
    return JSONOutputArchive__SerializeValueHelper(root_.GetAllocator(), it, value);
}

bool JSONOutputArchive::Serialize(ArchiveIterator&& it, uint64_t& value)
{
    return JSONOutputArchive__SerializeValueHelper(root_.GetAllocator(), it, value);
}

bool JSONOutputArchive::Serialize(ArchiveIterator&& it, float& value)
{
    return JSONOutputArchive__SerializeValueHelper(root_.GetAllocator(), it, value);
}

bool JSONOutputArchive::Serialize(ArchiveIterator&& it, double& value)
{
    return JSONOutputArchive__SerializeValueHelper(root_.GetAllocator(), it, value);
}

bool JSONOutputArchive::Serialize(ArchiveIterator&& it, std::string& value)
{
    if (!it)
        return false;

    if (auto* current = static_cast<JSONArchive::InputIterator*>(it.Get())->Current())
    {
        current->SetString(value.c_str(), root_.GetAllocator());
        return true;
    }
    return false;
}

// ---------------------- JSONInputArchive ----------------------

JSONInputArchive::JSONInputArchive(const std::string& json_data)
{
    root_.Parse(json_data.c_str());
}

static ArchiveIterator JSONInputArchive__BeginHelper(rapidjson::Document::AllocatorType& allocator, rapidjson::Value* target, Archive::ContainerType type)
{
    if (type == Archive::Array && !target->IsArray())
        return {};
    else if (type == Archive::Map && !target->IsObject())
        return {};

    return ArchiveIterator::ConstructR<JSONArchive::InputIterator>(target, allocator);
}

ArchiveIterator JSONInputArchive::Begin(ArchiveIterator&& it, Archive::ContainerType type)
{
    return JSONInputArchive__BeginHelper(root_.GetAllocator(), static_cast<InputIterator*>(it.Get())->Current(), type);   // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
}

ArchiveIterator JSONInputArchive::Begin(Archive::ContainerType type)
{
    return JSONInputArchive__BeginHelper(root_.GetAllocator(), &root_, type);
}

bool JSONInputArchive::Serialize(ArchiveIterator&& it, bool& value)
{
    if (!it)
        return false;

    if (auto* current = ((InputIterator*)it.Get())->Current())
    {
        if (current->IsBool())
        {
            value = current->GetBool();
            return true;
        }
    }
    return false;
}

bool JSONInputArchive::Serialize(ArchiveIterator&& it, std::string& value)
{
    if (!it)
        return false;

    if (auto* current = ((InputIterator*)it.Get())->Current())
    {
        if (current->IsString())
        {
            value = current->GetString();
            return true;
        }
    }
    return false;
}

template<typename T>
static typename std::enable_if<std::is_floating_point<T>::value, bool>::type
JSONInputArchive__SerializeValueHelper(ArchiveIterator& it, T& value)
{
    if (!it)
        return false;

    if (auto* current = static_cast<JSONArchive::InputIterator*>(it.Get())->Current())  // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
    {
        if (current->IsFloat())
        {
            value = current->GetFloat();
            return true;
        }
        else if (current->IsDouble())
        {
            value = current->GetDouble();
            return true;
        }
    }
    return false;
}

template<typename T>
static typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value, bool>::type
JSONInputArchive__SerializeValueHelper(ArchiveIterator& it, T& value)
{
    if (!it)
        return false;

    if (auto* current = static_cast<JSONArchive::InputIterator*>(it.Get())->Current())  // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
    {
        if (current->IsInt())
        {
            value = current->GetInt();
            return true;
        }
        else if (current->IsInt64())
        {
            value = current->GetInt64();
            return true;
        }
    }
    return false;
}

template<typename T>
static typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value, bool>::type
JSONInputArchive__SerializeValueHelper(ArchiveIterator& it, T& value)
{
    if (!it)
        return false;

    if (auto* current = static_cast<JSONArchive::InputIterator*>(it.Get())->Current())  // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
    {
        if (current->IsUint())
        {
            value = current->GetUint();
            return true;
        }
        else if (current->IsUint64())
        {
            value = current->GetUint64();
            return true;
        }
    }
    return false;
}

bool JSONInputArchive::Serialize(ArchiveIterator&& it, int8_t& value)
{
    return JSONInputArchive__SerializeValueHelper(it, value);
}

bool JSONInputArchive::Serialize(ArchiveIterator&& it, uint8_t& value)
{
    return JSONInputArchive__SerializeValueHelper(it, value);
}

bool JSONInputArchive::Serialize(ArchiveIterator&& it, int16_t& value)
{
    return JSONInputArchive__SerializeValueHelper(it, value);
}

bool JSONInputArchive::Serialize(ArchiveIterator&& it, uint16_t& value)
{
    return JSONInputArchive__SerializeValueHelper(it, value);
}

bool JSONInputArchive::Serialize(ArchiveIterator&& it, int32_t& value)
{
    return JSONInputArchive__SerializeValueHelper(it, value);
}

bool JSONInputArchive::Serialize(ArchiveIterator&& it, uint32_t& value)
{
    return JSONInputArchive__SerializeValueHelper(it, value);
}

bool JSONInputArchive::Serialize(ArchiveIterator&& it, int64_t& value)
{
    return JSONInputArchive__SerializeValueHelper(it, value);
}

bool JSONInputArchive::Serialize(ArchiveIterator&& it, uint64_t& value)
{
    return JSONInputArchive__SerializeValueHelper(it, value);
}

bool JSONInputArchive::Serialize(ArchiveIterator&& it, float& value)
{
    return JSONInputArchive__SerializeValueHelper(it, value);
}

bool JSONInputArchive::Serialize(ArchiveIterator&& it, double& value)
{
    return JSONInputArchive__SerializeValueHelper(it, value);
}

}
