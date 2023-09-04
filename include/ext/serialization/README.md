# Serialization for objects

Implementation of base classes to simplify serialization / deserialization of data
Essentially - an attempt to create reflection in C++

Usage example:
    #include <ext/serialization/iserializable.h>
    
	using namespace ext::serializable;
	
	struct ChannelSettings : public SerializableObject<ChannelSettings>
	{
		DECLARE_SERIALIZABLE_FIELD(std::string, ChannelName);
		DECLARE_SERIALIZABLE_FIELD(float CriticalValue, 5);
		DECLARE_SERIALIZABLE_FIELD(float, WarningValue, 4);
		DECLARE_SERIALIZABLE_FIELD((std::pair<int, float>), PairValue);
	};

	class ApplicationSettings : public SerializableObject<ApplicationSettings>
	{
	public:
		ApplicationSettings();
		~ApplicationSettings();

	public:
		DECLARE_SERIALIZABLE(bool, m_openAfterGeneration, false);
		DECLARE_SERIALIZABLE(std::list<std::shared_ptr<ChannelSettings>>, m_reportionChannels);
	};
	
	using namespace ext::serializable::serializer;
	ApplicationSettings::ApplicationSettings()
	{
		if (!Executor::DeserializeObject(Factory::XMLDeserializer("settings.xml"), this))
			...
	}

	ApplicationSettings::~ApplicationSettings()
	{
		if (!Executor::SerializeObject(Factory::XMLSerializer("settings.xml"), this))
			...
	}

For serializing to XML I am using PugiXml, so if you need to serialize to xml you need to set it and predefine USE_PUGI_XML and PUGIXML_WCHAR_MODE. Otherwise, you can serialize to string

More examples you can find in [Test project](https://github.com/Pennywise007/ext_test/blob/main/ext_test/Tests/Serialization.cpp) 

