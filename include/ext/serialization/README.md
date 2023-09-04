# Serialization of objects

Implementation of base classes to simplify serialization / deserialization of data
Essentially - an attempt to create reflection in C++

Usage example:
```c++
#include <ext/serialization/iserializable.h>

using namespace ext::serializable;

struct Settings : SerializableObject<Settings>
{
    struct User : SerializableObject<User>
    {
        DECLARE_SERIALIZABLE_FIELD(std::int64_t, id);
        DECLARE_SERIALIZABLE_FIELD(std::string, firstName);
        DECLARE_SERIALIZABLE_FIELD(std::string, userName);

        bool operator==(const User& other) const
        {
            return id == other.id && firstName == other.firstName && userName == other.userName;
        }
    };
    
    DECLARE_SERIALIZABLE_FIELD(std::wstring, token);
    DECLARE_SERIALIZABLE_FIELD(std::wstring, password);
    DECLARE_SERIALIZABLE_FIELD(std::list<User>, registeredUsers);

	Settings(){
		if (!Executor::DeserializeObject(Factory::XMLDeserializer("settings.xml"), this))
			...
	}
	~Settings() {
		if (!Executor::SerializeObject(Factory::XMLDeserializer("settings.xml"), this))
			...
	}
};

```
For serializing to XML I am using PugiXml, so if you need to serialize to xml you need to set it and predefine USE_PUGI_XML and PUGIXML_WCHAR_MODE. Otherwise, you can serialize to string

More examples you can find in [Test folder](https://github.com/Pennywise007/ext/blob/main/test/serialization/serialization_test.cpp) 

