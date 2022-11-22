#pragma once

#include <unordered_map>
#include <string>
#include <filesystem>
#include <vector>

#include "tinyxml2.h"

namespace fs = std::filesystem;

template <typename T>
void forEachFile(const fs::path& path, bool recursiveSearch, const T& callback)
{
	for (auto& file : fs::directory_iterator(path))
	{
		fs::file_status status = file.symlink_status();

		if (fs::is_directory(status))
		{
			if (recursiveSearch)
				forEachFile(file.path(), true, callback);

			continue;
		}

		callback(file.path());
	}
}

enum class SupportLevel
{
	None,
	Default,
	Override
};

struct SupportSettings
{
	std::string Name;
	SupportLevel Level = SupportLevel::Default;

	bool OverriddenBy(SupportLevel support)
	{
		return Level != SupportLevel::Override || support == SupportLevel::Override;
	}
};

extern std::unordered_map<std::string, int> features;
extern std::string locale;

SupportLevel featureIsActive(const char* feature);

SupportLevel matchesLocale(const char* nodeLocale);

SupportSettings featureIsActive(tinyxml2::XMLElement* node, SupportSettings& settings);

SupportSettings matchesLocale(tinyxml2::XMLElement* node, SupportSettings& settings);

bool isNodeEnabled(tinyxml2::XMLElement* node, SupportSettings* feature, SupportSettings* locale);

template <typename Type>
Type readValue(const tinyxml2::XMLAttribute* attribute);

template <>
int readValue<int>(const tinyxml2::XMLAttribute* attribute);

template <>
unsigned int readValue<unsigned int>(const tinyxml2::XMLAttribute* attribute);

template <>
short readValue<short>(const tinyxml2::XMLAttribute* attribute);

template <>
unsigned short readValue<unsigned short>(const tinyxml2::XMLAttribute* attribute);

template <>
unsigned char readValue<unsigned char>(const tinyxml2::XMLAttribute* attribute);

template <>
signed char readValue<signed char>(const tinyxml2::XMLAttribute* attribute);

template <>
bool readValue<bool>(const tinyxml2::XMLAttribute* attribute);

template <>
long long readValue<long long>(const tinyxml2::XMLAttribute* attribute);

template <>
unsigned long long readValue<unsigned long long>(const tinyxml2::XMLAttribute* attribute);

template <>
float readValue<float>(const tinyxml2::XMLAttribute* attribute);

template <>
double readValue<double>(const tinyxml2::XMLAttribute* attribute);

template <typename Type>
Type readAttribute(tinyxml2::XMLElement* node, const char* name, const Type& defaultValue)
{
	const tinyxml2::XMLAttribute* attribute = node->FindAttribute(name);

	if (attribute == nullptr)
		return defaultValue;

	return readValue<Type>(attribute);
}

template <typename Type>
Type readValue(const char* value)
{
	return atoi(value);
}

template <>
float readValue<float>(const char* value);

template <>
double readValue<double>(const char* value);

template <>
long long readValue<long long>(const char* value);

template <>
unsigned long long readValue<unsigned long long>(const char* value);

template <typename Type>
void readAttribute(tinyxml2::XMLElement* node, const char* name, std::vector<Type>& vector)
{
	const tinyxml2::XMLAttribute* attribute = node->FindAttribute(name);

	if (attribute == nullptr)
		return;

	const char* value = attribute->Value();

	if (strcmp(value, "") == 0)
		return;

	int i = 0;

	while (value[i])
	{
		vector.push_back(readValue<Type>(value + i));

		while (value[i] && value[i] >= '0' && value[i] <= '9')
			++i;

		while (value[i] && (value[i] < '0' || value[i] > '9'))
			++i;
	}
}

bool loadFeatures(const fs::path& tableRoot, const char* locale, const char* env);