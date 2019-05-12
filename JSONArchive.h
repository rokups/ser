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

#include "rapidjson/document.h"

#include "Archive.h"

namespace ser
{

class JSONArchive : public Archive
{
public:
    /// Implements a validating iterator for reading.
    class InputIterator : public detail::IArchiveIterator
    {
    protected:
        rapidjson::Value* container_ = nullptr;
        size_t index_ = 0;
        rapidjson::Document::AllocatorType& allocator_;

        virtual ArchiveIterator Construct(rapidjson::Value* container, rapidjson::Document::AllocatorType& allocator, size_t index);
        void Copy(ArchiveIterator& destination) const override;

    public:
        explicit InputIterator(rapidjson::Value* container, rapidjson::Document::AllocatorType& allocator);
        explicit InputIterator(rapidjson::Value* container, rapidjson::Document::AllocatorType& allocator, size_t index);
        InputIterator(const InputIterator& other) = default;

        virtual rapidjson::Value* Current();
        int Size() const override;
        ArchiveIterator Find(const std::string& key) override;
        bool AtEnd() const override;

    protected:
        // These operators are accessed through wrapper operators of ArchiveIterator.
        ArchiveIterator operator[](int index) override;
        void operator++() override;
    };

    rapidjson::Document root_;
};

class JSONOutputArchive : public JSONArchive
{

public:
    /// Implements a converting/appending iterator for writing.
    class OutputIterator : public InputIterator
    {
    protected:
        ArchiveIterator Construct(rapidjson::Value* container, rapidjson::Document::AllocatorType& allocator, size_t index) override;
        void Copy(ArchiveIterator& destination) const override;
    public:
        explicit OutputIterator(rapidjson::Value* container, rapidjson::Document::AllocatorType& allocator);
        explicit OutputIterator(rapidjson::Value* container, rapidjson::Document::AllocatorType& allocator, size_t index);

        rapidjson::Value* Current() override;
        ArchiveIterator operator[](int index) override;
        void operator++() override;
        ArchiveIterator Find(const std::string& key) override;
        bool AtEnd() const override;
    };
    static_assert(sizeof(OutputIterator) <= ArchiveIterator::StorageSize, "ArchiveIterator::storage_ is too small.");

    using Iterator = OutputIterator;
private:
    SER_USER_CONTAINER(JSONOutputArchive);
public:

    JSONOutputArchive() = default;

    /// Begin writing to container of specified type. Container pointed by specified iterator will be converted to specified type.
    ArchiveIterator Begin(ArchiveIterator&& it, ContainerType type) override;
    /// Begin writing to container of specified type. Root container will be converted to specified type.
    ArchiveIterator Begin(ContainerType type) override;
    /// Return serialized JSON result.
    std::string ToString() const;

    bool Serialize(ArchiveIterator&& it, bool& value) override;
    bool Serialize(ArchiveIterator&& it, int8_t& value) override;
    bool Serialize(ArchiveIterator&& it, uint8_t& value) override;
    bool Serialize(ArchiveIterator&& it, int16_t& value) override;
    bool Serialize(ArchiveIterator&& it, uint16_t& value) override;
    bool Serialize(ArchiveIterator&& it, int32_t& value) override;
    bool Serialize(ArchiveIterator&& it, uint32_t& value) override;
    bool Serialize(ArchiveIterator&& it, int64_t& value) override;
    bool Serialize(ArchiveIterator&& it, uint64_t& value) override;
    bool Serialize(ArchiveIterator&& it, float& value) override;
    bool Serialize(ArchiveIterator&& it, double& value) override;
    bool Serialize(ArchiveIterator&& it, std::string& value) override;
};

class JSONInputArchive : public JSONArchive
{
public:
    using Iterator = InputIterator;
private:
    SER_USER_CONTAINER(JSONInputArchive);
public:
    /// Construct input archive that will read specified json.
    explicit JSONInputArchive(const std::string& json_data);
    /// Begin iterating container of specified type at specified iterator.
    ArchiveIterator Begin(ArchiveIterator&& it, ContainerType type) override;
    /// Begin iterating container of specified type at archive root.
    ArchiveIterator Begin(ContainerType type) override;

    bool Serialize(ArchiveIterator&& it, bool& value) override;
    bool Serialize(ArchiveIterator&& it, int8_t& value) override;
    bool Serialize(ArchiveIterator&& it, uint8_t& value) override;
    bool Serialize(ArchiveIterator&& it, int16_t& value) override;
    bool Serialize(ArchiveIterator&& it, uint16_t& value) override;
    bool Serialize(ArchiveIterator&& it, int32_t& value) override;
    bool Serialize(ArchiveIterator&& it, uint32_t& value) override;
    bool Serialize(ArchiveIterator&& it, int64_t& value) override;
    bool Serialize(ArchiveIterator&& it, uint64_t& value) override;
    bool Serialize(ArchiveIterator&& it, float& value) override;
    bool Serialize(ArchiveIterator&& it, double& value) override;
    bool Serialize(ArchiveIterator&& it, std::string& value) override;
};

}   // namespace ser
