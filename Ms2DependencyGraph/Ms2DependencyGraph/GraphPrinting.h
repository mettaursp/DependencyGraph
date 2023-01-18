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

	struct PrintSettings
	{
		bool PrintTypes = true;
		bool PrintEffectTypes = true;
		bool PrintPaths = false;
		bool PrintReset = true;
		bool PrintKeepCondition = true;
		bool PrintStacks = true;
		bool PrintMotions = false;
		bool PrintAttacks = false;
		bool PrintTargets = true;
		bool PrintImmediateActiveSkill = true;
		bool PrintTarget = true;
		bool PrintRequireSkillCodeConnections = false;
		bool PrintAttackMaterial = true;
		bool PrintNonTargetAttack = true;
		bool PrintApplyTarget = true;
	};

	PrintSettings Settings;

	struct References
	{
		ReferenceData Reference;
		std::unordered_map<int, std::vector<ReferenceData>> LevelReferences;
	};

	std::unordered_map<int, References> ReferencedSkills;
	std::unordered_map<int, References> ReferencedEffects;
	std::unordered_set<int> ReferencedSplashSkills;
	std::unordered_set<int> ReferencedSensorSkills;
	std::unordered_set<int> ReferencedEffectGroups;
	std::vector<ReferenceData> QueuedSkills;
	std::vector<ReferenceData> QueuedEffects;

	const ReferenceData& Dereference(const ReferenceData& reference, bool isSplash = false);
	void Print(const ReferenceData& caller, const ConditionSkill& trigger, const std::string& style = "", int index1 = -1, int index2 = -1, const SkillAttack* attack = nullptr);
	void PrintRoot(const JobSkill& jobSkill);
	void PrintRoot(const JobData& jobData);
	void PrintLinked();
	void PrintRoot(const SetBonusData& setData);
};