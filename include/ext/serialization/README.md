# Serialization of objects

Implementation of base classes to simplify serialization / deserialization of data
Essentially - an attempt to create reflection in C++

Usage example:
```c++
#include <ext/serialization/iserializable.h>

using namespace ext::serializable;
using namespace ext::serializer;

#if C++20
struct Settings
{
    struct User
    {
        std::int64_t id;
        std::string firstName;
        std::string userName;
    };
    
    std::wstring password;
    std::list<User> registeredUsers;
};

Settings settings;

std::wstring text;
if (!DeserializeObject(Factory::TextDeserializer(text), settings))
    ...
if (!SerializeObject(Factory::TextSerializer(text), settings))
    ...
#endif // C++20
struct Settings
{
    struct User
    {
        REGISTER_SERIALIZABLE_OBJECT();

        DECLARE_SERIALIZABLE_FIELD(std::int64_t, id);
        DECLARE_SERIALIZABLE_FIELD(std::string, firstName);
        DECLARE_SERIALIZABLE_FIELD(std::string, userName);
    };
    
    REGISTER_SERIALIZABLE_OBJECT_N("My settings");
    DECLARE_SERIALIZABLE_FIELD(std::wstring, token);
    DECLARE_SERIALIZABLE_FIELD(std::wstring, password);
    DECLARE_SERIALIZABLE_FIELD(std::list<User>, registeredUsers);

	Settings(){
        std::wstring text;
		if (!DeserializeObject(Factory::TextDeserializer(text), *this))
			...
	}
	~Settings() {
        std::wstring text;
		if (!SerializeObject(Factory::TextSerializer(text), *this))
			...
	}
};

```
For serializing to XML I am using PugiXml, so if you need to serialize to xml you need to set it and predefine USE_PUGI_XML and PUGIXML_WCHAR_MODE. Otherwise, you can serialize to string

More examples you can find in [Test folder](https://github.com/Pennywise007/ext/blob/main/test/serialization/serialization_test.cpp) 

