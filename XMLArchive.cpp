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
#include "XMLArchive.h"


namespace ser
{

// ---------------------- XMLArchive::InputIterator ----------------------

ArchiveIterator XMLArchive::InputIterator::Construct(pugi::xml_node container, pugi::xml_node index)
{
    return ArchiveIterator::ConstructR<InputIterator>(container, index);
}

void XMLArchive::InputIterator::Copy(ArchiveIterator& destination) const
{
    ArchiveIterator::Construct<InputIterator>(destination, *this);
}

XMLArchive::InputIterator::InputIterator(pugi::xml_node container)
    : container_(container)
    , index_(container.first_child())
{
}

XMLArchive::InputIterator::InputIterator(pugi::xml_node container, pugi::xml_node index)
    : container_(container)
    , index_(index)
{
}

pugi::xml_node XMLArchive::InputIterator::Current()
{
    if (AtEnd())
        return {};

    return index_;
}

int XMLArchive::InputIterator::Size() const
{
    if (container_.empty())
        return 0;

    return std::distance(container_.begin(), container_.end());
}

ArchiveIterator XMLArchive::InputIterator::Find(const std::string& key)
{
    if (container_.empty())
        return ArchiveIterator{};

    auto node = container_.find_child_by_attribute("value", "key", key.c_str());
    if (node.empty())
        return {};

    return Construct(container_, node);
}

bool XMLArchive::InputIterator::AtEnd() const
{
    return container_.empty() || index_.empty();
}

ArchiveIterator XMLArchive::InputIterator::operator[](int index)
{
    if (container_.empty() || Size() <= index)
        return {};

    auto it = container_.begin();
    for (; index-- > 0; it++);

    return Construct(container_, *it);
}

void XMLArchive::InputIterator::operator++()
{
    index_ = index_.next_sibling();
}

// ---------------------- XMLOutputArchive::XMLOutputIterator ----------------------

ArchiveIterator XMLOutputArchive::OutputIterator::Construct(pugi::xml_node container, pugi::xml_node index)
{
    return ArchiveIterator::ConstructR<OutputIterator>(container, index);
}

void XMLOutputArchive::OutputIterator::Copy(ArchiveIterator& destination) const
{
    ArchiveIterator::Construct<OutputIterator>(destination, *this);
}

XMLOutputArchive::OutputIterator::OutputIterator(pugi::xml_node container)
    : InputIterator(container)
{
}

XMLOutputArchive::OutputIterator::OutputIterator(pugi::xml_node container, pugi::xml_node index)
    : InputIterator(container, index)
{
}

pugi::xml_node XMLOutputArchive::OutputIterator::Current()
{
    // Automatically expanding
    if (index_.empty())
    {
        container_.append_child("value");
        index_ = container_.last_child();
    }

    return index_;
}

bool XMLOutputArchive::OutputIterator::AtEnd() const
{
    return container_.empty();
}

ArchiveIterator XMLOutputArchive::OutputIterator::operator[](int index)
{
    if (container_.empty())
        return {};

    while (index < Size())
        container_.append_child("value");

    return InputIterator::operator[](index);
}

ArchiveIterator XMLOutputArchive::OutputIterator::Find(const std::string& key)
{
    if (container_.empty())
        return ArchiveIterator{};

    auto node = container_.find_child_by_attribute("value", "key", key.c_str());
    if (node.empty())
    {
        node = container_.append_child("value");
        node.append_attribute("key").set_value(key.c_str());
    }

    return Construct(container_, node);
}

// ---------------------- XMLOutputArchive ----------------------

template<typename T>
static bool XMLOutputArchive__SerializeValueHelper(ArchiveIterator& it, T& value)
{
    if (!it)
        return false;

    if (auto current = static_cast<XMLInputArchive::InputIterator*>(it.Get())->Current())   // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
    {
        current.append_child(pugi::node_pcdata).set_value(std::to_string(value).c_str());   // TODO: possibly sucks
        return true;
    }
    return false;
}

static ArchiveIterator XMLOutputArchive__BeginHelper(pugi::xml_node target, Archive::ContainerType type)
{
    return ArchiveIterator::ConstructR<XMLOutputArchive::OutputIterator>(target);
}

XMLOutputArchive::XMLOutputArchive()
{
    root_.append_child("root");
}

ArchiveIterator XMLOutputArchive::Begin(ArchiveIterator&& it, Archive::ContainerType type)
{
    return XMLOutputArchive__BeginHelper(static_cast<InputIterator*>(it.Get())->Current(), type);   // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
}

ArchiveIterator XMLOutputArchive::Begin(Archive::ContainerType type)
{
    return XMLOutputArchive__BeginHelper(root_.root().first_child(), type);
}

std::string XMLOutputArchive::ToString() const
{
    struct xml_string_writer: pugi::xml_writer
    {
        std::string result;

        void write(const void* data, size_t size) override
        {
            result.append(static_cast<const char*>(data), size);
        }
    };

    xml_string_writer xml_writer{};
    root_.save(xml_writer);
    return xml_writer.result;
}

bool XMLOutputArchive::Serialize(ArchiveIterator&& it, std::string& value)
{
    if (!it)
        return false;

    if (auto current = ((InputIterator*)it.Get())->Current())
    {
        current.set_value(value.c_str());
        return true;
    }
    return false;
}

bool XMLOutputArchive::Serialize(ArchiveIterator&& it, bool& value)
{
    return XMLOutputArchive__SerializeValueHelper(it, value);
}

bool XMLOutputArchive::Serialize(ArchiveIterator&& it, int8_t& value)
{
    return XMLOutputArchive__SerializeValueHelper(it, value);
}

bool XMLOutputArchive::Serialize(ArchiveIterator&& it, uint8_t& value)
{
    return XMLOutputArchive__SerializeValueHelper(it, value);
}

bool XMLOutputArchive::Serialize(ArchiveIterator&& it, int16_t& value)
{
    return XMLOutputArchive__SerializeValueHelper(it, value);
}

bool XMLOutputArchive::Serialize(ArchiveIterator&& it, uint16_t& value)
{
    return XMLOutputArchive__SerializeValueHelper(it, value);
}

bool XMLOutputArchive::Serialize(ArchiveIterator&& it, int32_t& value)
{
    return XMLOutputArchive__SerializeValueHelper(it, value);
}

bool XMLOutputArchive::Serialize(ArchiveIterator&& it, uint32_t& value)
{
    return XMLOutputArchive__SerializeValueHelper(it, value);
}

bool XMLOutputArchive::Serialize(ArchiveIterator&& it, int64_t& value)
{
    return XMLOutputArchive__SerializeValueHelper(it, value);
}

bool XMLOutputArchive::Serialize(ArchiveIterator&& it, uint64_t& value)
{
    return XMLOutputArchive__SerializeValueHelper(it, value);
}

bool XMLOutputArchive::Serialize(ArchiveIterator&& it, float& value)
{
    return XMLOutputArchive__SerializeValueHelper(it, value);
}

bool XMLOutputArchive::Serialize(ArchiveIterator&& it, double& value)
{
    return XMLOutputArchive__SerializeValueHelper(it, value);
}

// ---------------------- XMLInputArchive ----------------------

template<typename T>
typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value, bool>::type
XMLInputArchive__SerializeValueHelper(ArchiveIterator& it, T& value)
{
    if (!it)
        return false;

    if (auto current = static_cast<XMLInputArchive::InputIterator*>(it.Get())->Current())   // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
    {
        size_t pos = 0;
        value = (T)std::stoull(current.value(), &pos);
        return pos > 0;
    }
    return false;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value, bool>::type
XMLInputArchive__SerializeValueHelper(ArchiveIterator& it, T& value)
{
    if (!it)
        return false;

    if (auto current = static_cast<XMLInputArchive::InputIterator*>(it.Get())->Current())   // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
    {
        size_t pos = 0;
        value = (T)std::stoll(current.first_child().value(), &pos);
        return pos > 0;
    }
    return false;
}

XMLInputArchive::XMLInputArchive(const std::string& xml_data)
{
    root_.load_string(xml_data.c_str());
}

ArchiveIterator XMLInputArchive::Begin(ArchiveIterator&& it, Archive::ContainerType type)
{
    return ArchiveIterator::ConstructR<InputIterator>(static_cast<InputIterator*>(it.Get())->Current());    // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
}

ArchiveIterator XMLInputArchive::Begin(Archive::ContainerType type)
{
    return ArchiveIterator::ConstructR<InputIterator>(root_.root().first_child());
}

bool XMLInputArchive::Serialize(ArchiveIterator&& it, bool& value)
{
    if (!it)
        return false;

    if (auto current = ((InputIterator*)it.Get())->Current())
    {
        value = strcmp(current.value(), "true") == 0;
        return true;
    }
    return false;
}

bool XMLInputArchive::Serialize(ArchiveIterator&& it, int8_t& value)
{
    return XMLInputArchive__SerializeValueHelper(it, value);
}

bool XMLInputArchive::Serialize(ArchiveIterator&& it, uint8_t& value)
{
    return XMLInputArchive__SerializeValueHelper(it, value);
}

bool XMLInputArchive::Serialize(ArchiveIterator&& it, int16_t& value)
{
    return XMLInputArchive__SerializeValueHelper(it, value);
}

bool XMLInputArchive::Serialize(ArchiveIterator&& it, uint16_t& value)
{
    return XMLInputArchive__SerializeValueHelper(it, value);
}

bool XMLInputArchive::Serialize(ArchiveIterator&& it, int32_t& value)
{
    return XMLInputArchive__SerializeValueHelper(it, value);
}

bool XMLInputArchive::Serialize(ArchiveIterator&& it, uint32_t& value)
{
    return XMLInputArchive__SerializeValueHelper(it, value);
}

bool XMLInputArchive::Serialize(ArchiveIterator&& it, int64_t& value)
{
    return XMLInputArchive__SerializeValueHelper(it, value);
}

bool XMLInputArchive::Serialize(ArchiveIterator&& it, uint64_t& value)
{
    return XMLInputArchive__SerializeValueHelper(it, value);
}

bool XMLInputArchive::Serialize(ArchiveIterator&& it, float& value)
{
    if (!it)
        return false;

    if (auto current = ((InputIterator*)it.Get())->Current())
    {
        size_t pos = 0;
        value = std::stof(current.first_child().value(), &pos);
        return pos > 0;
    }
    return false;
}

bool XMLInputArchive::Serialize(ArchiveIterator&& it, double& value)
{
    if (!it)
        return false;

    if (auto current = ((InputIterator*)it.Get())->Current())
    {
        size_t pos = 0;
        value = std::stod(current.first_child().value(), &pos);
        return pos > 0;
    }
    return false;
}

bool XMLInputArchive::Serialize(ArchiveIterator&& it, std::string& value)
{
    if (!it)
        return false;

    if (auto current = ((InputIterator*)it.Get())->Current())
    {
        value = current.first_child().value();
        return true;
    }
    return false;
}

}
