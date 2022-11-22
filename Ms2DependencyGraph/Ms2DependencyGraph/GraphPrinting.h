#pragma once

#include <filesystem>
#include <fstream>
#include <unordered_set>

#include "XmlData.h"

struct GraphData
{
	std::ofstream& OutFile;
	std::string RootName;
	std::string RootLabel;

	std::unordered_map<int, std::vector<ReferenceData>> ReferencedSkills;
	std::unordered_map<int, std::vector<ReferenceData>> ReferencedEffects;
	std::vector<int> QueuedSkills;
	std::vector<int> QueuedEffects;

	const ReferenceData& Dereference(const ReferenceData& reference);
	void Print(const ReferenceData& caller, const ConditionSkill& trigger, const std::string& style = "");
	void PrintRoot(const JobSkill& jobSkill);
	void PrintRoot(const JobData& jobData);
	void PrintLinked();
	void PrintRoot(const SetBonusData& setData);
};