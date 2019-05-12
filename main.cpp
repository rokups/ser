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
#include <typeindex>
#include <iostream>
#include "JSONArchive.h"
#include "XMLArchive.h"


using namespace ser;


class Serializable
{
public:
    virtual void Serialize(Archive* archive) = 0;
};

struct UserType
{
    int userValue = 0;
};

bool SerializeToJSON(JSONOutputArchive& archive, JSONOutputArchive::Iterator& it, UserType& value)
{
    auto* target = it.Current();
    target->SetObject();

    target->AddMember("type", "UserType", archive.root_.GetAllocator());
    target->AddMember("userValue", value.userValue, archive.root_.GetAllocator());

    return true;
}

bool SerializeFromJSON(JSONInputArchive& archive, JSONInputArchive::Iterator& it, UserType& value)
{
    auto* target = it.Current();
    if (target == nullptr)
        return false;

    if (!target->IsObject())
        return false;

    auto it_type = target->FindMember("type");
    auto it_value = target->FindMember("userValue");

    if (it_type == target->MemberEnd() || it_value == target->MemberEnd())
        return false;

    if (strcmp(it_type->value.GetString(), "UserType") != 0)
        return false;

    if (!it_value->value.IsNumber())
        return false;

    value.userValue = it_value->value.GetInt64();

    return true;
}

bool SerializeToXML(XMLOutputArchive& archive, XMLOutputArchive::Iterator& it, UserType& value)
{
    auto target = it.Current();
    if (target.empty())
        return false;

    target.set_name("UserType");
    target.append_child("userValue").append_child(pugi::node_pcdata).set_value(std::to_string(value.userValue).c_str());

    return true;
}

bool SerializeFromXML(XMLInputArchive& archive, XMLInputArchive::Iterator& it, UserType& value)
{
    auto target = it.Current();
    if (target.empty())
        return false;

    if (strcmp(target.name(), "UserType") != 0)
        return false;

    auto userValue = target.child("userValue");
    if (userValue.empty())
        return false;

    size_t pos = 0;
    value.userValue = std::stoi(userValue.first_child().value(), &pos);

    return pos > 0;
}

class SerializableObject : public Serializable
{
public:
    int value11{};
    int value1{};
    int value2{};
    int value3{};
    UserType user;

    /*
     * The whole idea is that each serialization archive may contain objects, arrays or values. We iterate each value
     * using universal non-allocating iterator and serialize data to/from by passing that iterator to serialization
     * function. Code constructs dealing with container iteration for std containers will work here.
     */
    void Serialize(Archive* archive) override
    {
        if (auto it = archive->Begin(Archive::Array))
        {
            if (auto map = archive->Begin(it++, Archive::Map))
                archive->Serialize(map["value11"], value11);

            archive->Serialize(it++, value1);
            archive->Serialize(it++, value2);
            archive->Serialize(it++, value3);
            archive->Serialize(it++, user);
        }

        // This would also work
        // unsigned i = 0;
        // for (auto it = archive->Begin(Archive::Array); !it.AtEnd(); ++it)
        // {
        //    archive->Serialize(it++, arr[i++]);
        //    archive->Serialize(it++, arr[i++]);
        //    archive->Serialize(it++, arr[i++]);
        // }
    }
};


template<typename InputArchive, typename OutputArchive>
void test()
{
    OutputArchive out;
    SerializableObject obj_out;
    obj_out.value11 = 11;
    obj_out.value1 = 1;
    obj_out.value2 = 2;
    obj_out.value3 = 3;
    obj_out.user.userValue = 4;
    obj_out.Serialize(&out);

    auto serialized_data = out.ToString();
    std::cout << serialized_data << std::endl;

    InputArchive in(serialized_data);
    SerializableObject obj_in;
    obj_in.Serialize(&in);

    assert(obj_out.value11 == obj_in.value11);
    assert(obj_out.value1 == obj_in.value1);
    assert(obj_out.value2 == obj_in.value2);
    assert(obj_out.value3 == obj_in.value3);
    assert(obj_out.user.userValue == obj_in.user.userValue);
}

int main()
{
    SER_USER_TYPE_SERIALIZER(JSONOutputArchive, UserType, SerializeToJSON);
    SER_USER_TYPE_SERIALIZER(JSONInputArchive, UserType, SerializeFromJSON);
    SER_USER_TYPE_SERIALIZER(XMLOutputArchive, UserType, SerializeToXML);
    SER_USER_TYPE_SERIALIZER(XMLInputArchive, UserType, SerializeFromXML);

    test<JSONInputArchive, JSONOutputArchive>();
    test<XMLInputArchive, XMLOutputArchive>();
    return 0;
}
