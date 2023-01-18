#include "ParserUtils.h"

std::unordered_map<std::string, int> features;
std::string locale;

template <>
int readValue<int>(const tinyxml2::XMLAttribute* attribute)
{
	return attribute->IntValue();
}

template <>
unsigned int readValue<unsigned int>(const tinyxml2::XMLAttribute* attribute)
{
	return attribute->IntValue();
}

template <>
short readValue<short>(const tinyxml2::XMLAttribute* attribute)
{
	return attribute->IntValue();
}

template <>
unsigned short readValue<unsigned short>(const tinyxml2::XMLAttribute* attribute)
{
	return attribute->IntValue();
}

template <>
unsigned char readValue<unsigned char>(const tinyxml2::XMLAttribute* attribute)
{
	return attribute->IntValue();
}

template <>
signed char readValue<signed char>(const tinyxml2::XMLAttribute* attribute)
{
	return attribute->IntValue();
}

template <>
bool readValue<bool>(const tinyxml2::XMLAttribute* attribute)
{
	return attribute->IntValue();
}

template <>
long long readValue<long long>(const tinyxml2::XMLAttribute* attribute)
{
	return attribute->Int64Value();
}

template <>
unsigned long long readValue<unsigned long long>(const tinyxml2::XMLAttribute* attribute)
{
	return attribute->Int64Value();
}

template <>
float readValue<float>(const tinyxml2::XMLAttribute* attribute)
{
	return attribute->FloatValue();
}

template <>
double readValue<double>(const tinyxml2::XMLAttribute* attribute)
{
	return attribute->DoubleValue();
}

template <>
float readValue<float>(const char* value)
{
	return (float)atof(value);
}

template <>
double readValue<double>(const char* value)
{
	return atof(value);
}

template <>
long long readValue<long long>(const char* value)
{
	return atoll(value);
}

template <>
unsigned long long readValue<unsigned long long>(const char* value)
{
	return atoll(value);
}

int featureIsActive(const char* feature)
{
	if (strcmp(feature, "") == 0)
		return 1;

	const auto& support = features.find(feature);

	return support != features.end() ? support->second : -1;
}

SupportLevel matchesLocale(const char* nodeLocale)
{
	if (strcmp(nodeLocale, "") == 0)
		return SupportLevel::Default;

	return locale == nodeLocale ? SupportLevel::Override : SupportLevel::None;
}

SupportSettings featureIsActive(tinyxml2::XMLElement* node, SupportSettings& settings)
{
	const tinyxml2::XMLAttribute* attribute = node->FindAttribute("feature");

	int support = -1;
	const char* feature = "";

	if (attribute != nullptr)
	{
		feature = attribute->Value();
		support = featureIsActive(feature);
	}

	if (settings.OverriddenBy(support))
		return SupportSettings{ feature, SupportLevel::Default, support };

	return SupportSettings{ "", SupportLevel::None };
}

SupportSettings matchesLocale(tinyxml2::XMLElement* node, SupportSettings& settings)
{
	const tinyxml2::XMLAttribute* attribute = node->FindAttribute("locale");

	SupportLevel support = SupportLevel::Default;
	const char* locale = "";

	if (attribute != nullptr)
	{
		locale = attribute->Value();
		support = matchesLocale(locale);
	}

	if (settings.OverriddenBy(support))
		return SupportSettings{ locale, support };

	return SupportSettings{ "", SupportLevel::None };
}

bool isNodeEnabled(tinyxml2::XMLElement* node, SupportSettings* feature, SupportSettings* locale)
{
	SupportSettings featureFound;
	SupportSettings localeFound;

	if (feature != nullptr)
		featureFound = featureIsActive(node, *feature);

	if (locale != nullptr)
		localeFound = matchesLocale(node, *locale);

	if (featureFound.Version == -1 || featureFound.Version == 99 || featureFound.Version < feature->Version || localeFound.Level == SupportLevel::None)
		return false;

	if (feature != nullptr)
		*feature = featureFound;

	if (locale != nullptr)
		*locale = localeFound;

	return true;
}

bool loadFeatures(const fs::path& tableRoot, const char* locale, const char* env)
{
	fs::path featureSettingPath = tableRoot;
	featureSettingPath += "feature_setting.xml";

	fs::path featurePath = tableRoot;
	featurePath += "feature.xml";

	if (!fs::exists(featurePath) || !fs::exists(featureSettingPath))
		return false;

	int featureLevel = -1;

	{
		tinyxml2::XMLDocument document;

		document.LoadFile(featureSettingPath.string().c_str());

		tinyxml2::XMLElement* rootElement = document.RootElement();

		for (tinyxml2::XMLElement* settingElement = rootElement->FirstChildElement(); settingElement; settingElement = settingElement->NextSiblingElement())
		{
			const tinyxml2::XMLAttribute* typeAttribute = settingElement->FindAttribute("type");

			if (typeAttribute == nullptr || strcmp(typeAttribute->Value(), env) != 0)
				continue;

			const tinyxml2::XMLAttribute* featureLevelAttribute = settingElement->FindAttribute(locale);

			if (featureLevelAttribute != nullptr)
				featureLevel = featureLevelAttribute->IntValue();

			break;
		}
	}

	if (featureLevel == -1)
		return false;

	tinyxml2::XMLDocument document;

	document.LoadFile(featurePath.string().c_str());

	tinyxml2::XMLElement* rootElement = document.RootElement();

	for (tinyxml2::XMLElement* featureElement = rootElement->FirstChildElement(); featureElement; featureElement = featureElement->NextSiblingElement())
	{
		const tinyxml2::XMLAttribute* featureLevelAttribute = featureElement->FindAttribute(locale);

		if (featureLevelAttribute == nullptr)
			continue;

		const tinyxml2::XMLAttribute* featureNameAttribute = featureElement->FindAttribute("name");

		if (featureNameAttribute == nullptr)
			continue;

		int level = featureLevelAttribute->IntValue();

		if (level <= featureLevel)
			features[featureNameAttribute->Value()] = level;
	}

	return true;
}