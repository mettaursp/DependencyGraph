#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>

#include "tinyxml2.h"
#include "ParserUtils.h"
#include "XmlData.h"

extern std::unordered_map<int, MagicPathData> magicPaths;
extern std::unordered_map<int, AdditionalEffectData> effects;
extern std::unordered_map<int, SkillData> skills;
extern std::unordered_map<JobCode, JobData> jobs;
extern std::unordered_map<int, SetBonusOptionData> setBonusOptions;
extern std::unordered_map<int, SetBonusData> setBonuses;
extern std::unordered_map<int, ItemData> items;

void ParseMagicPaths(const fs::path& filePath);

void ParseBeginCondition(tinyxml2::XMLElement* node, BeginCondition& condition);
void ParseConditionSkill(tinyxml2::XMLElement* node, ConditionSkill& conditionSkill);

void ParseAdditionalEffect(const fs::path& filePath);
void ParseSkill(const fs::path& filePath);
void ParseStrings(const fs::path& filePath);
void ParseItems(const fs::path& filePath);
void ParseItemStrings(const fs::path& filePath);
void ParseItemDescriptionStrings(const fs::path& filePath);
void ParseJobs(const fs::path& filePath);
void ParseJobStrings(const fs::path& filePath);
void ParseSetBonusOptions(const fs::path& filePath);
void ParseSetBonuses(const fs::path& filePath);
void ParseSetBonusStrings(const fs::path& filePath);
std::string Desanitize(const std::string& text);
std::string Sanitize(const std::string& text, bool isTooltip = false);