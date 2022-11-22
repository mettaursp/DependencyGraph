#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>

#include "tinyxml2.h"
#include "ParserUtils.h"

enum class ReferenceType
{
	Effect = 0,
	Skill = 1,
	EffectCategory,

};

enum class SkillTarget
{
	SkillTarget = 0,
	Owner = 1,
	Target = 2,
	Caster = 3,
	PetOwner = 4,
	Attacker = 5
};

enum class ConditionReferenceType
{
	Require,
	Prevent,
	RequireRange,
	Ignore,
	RequireEvent
};

enum class ModifyReferenceType
{
	Cancel,
	Immune,
	ResetCooldown,
	ModifyDuration,
	ModifyStacks
};

enum class JobCode
{
	None = 0,
	Beginner = 1,
	Knight = 10,
	Berserker = 20,
	Wizard = 30,
	Priest = 40,
	Archer = 50,
	HeavyGunner = 60,
	Thief = 70,
	Assassin = 80,
	Runeblade = 90,
	Striker = 100,
	SoulBinder = 110,
	GameMaster = 999
};

struct ReferenceData
{
	ReferenceType Type = ReferenceType::Effect;
	int Id = 0;
	int Level = 0;
};

struct EffectReferenceData : public ReferenceData
{
	int MinStacks = 0;
	int MaxStacks = 0;
};

struct TriggerReferenceData : public ReferenceData
{
	SkillTarget Target = SkillTarget::SkillTarget;
	ConditionReferenceType ConditionType = ConditionReferenceType::Require;
};

struct BeginCondition
{
	std::vector<TriggerReferenceData> References;
};

struct ConditionSkill
{
	ReferenceData Reference;

	std::vector<ReferenceData> RandomCasts;

	BeginCondition Condition;
};

struct ModifyReference : public ReferenceData
{
	ModifyReferenceType ModificationType = ModifyReferenceType::Cancel;
	int Offset = 0;
};

struct AdditionalEffectLevelData;

struct AdditionalEffectData
{
	std::unordered_map<int, AdditionalEffectLevelData> Levels;
};

struct AdditionalEffectLevelData
{
	std::string Name;
	std::string Description;

	SupportSettings Feature;
	SupportSettings Locale;

	BeginCondition Condition;
	std::vector<ConditionSkill> Triggers;
	std::vector<ModifyReference> Modifications;
};

struct SkillLevelData;

struct SkillData
{
	std::string Name;

	SupportSettings Feature;
	SupportSettings Locale;

	std::unordered_map<int, SkillLevelData> Levels;
};

struct ChangeSkillReference
{
	EffectReferenceData Effect;
	ReferenceData Skill;
	ReferenceData OriginSkill;
};

struct ComboReference
{
	bool IsCombo = false;
	bool IsCharging = false;
	ReferenceData OriginSkill;
	ReferenceData InputSkill;
	ReferenceData OutputSkill;
};

struct SkillAttack;

struct SkillMotion
{
	std::vector<SkillAttack> Attacks;
};

struct SkillAttack
{
	std::vector<ConditionSkill> Triggers;
};

struct SkillLevelData
{
	std::string Description;

	SupportSettings Feature;
	SupportSettings Locale;

	BeginCondition Condition;
	ComboReference Combo;
	std::vector<ConditionSkill> Passives;
	std::vector<ChangeSkillReference> ChangeSkillReferences;
	std::vector<SkillMotion> Motions;
};

struct JobSkill
{
	ReferenceData Skill;

	std::vector<ReferenceData> SubSkills;
};

struct JobData
{
	JobCode Job = JobCode::None;
	std::string Name;
	std::string AwakenedName;

	SupportSettings Feature;
	SupportSettings Locale;

	std::vector<JobSkill> Skills;
};

struct SetBonusOptionData;

struct SetBonusData
{
	int OptionId = 0;
	std::string Name;

	const SetBonusOptionData* OptionData = nullptr;

	SupportSettings Feature;
	SupportSettings Locale;

	std::vector<int> ItemIds;
};

struct SetBonusOptionPartData;

struct SetBonusOptionData
{
	std::vector<SetBonusOptionPartData> Parts;
};

struct SetBonusOptionPartData
{
	int Count = 0;

	std::vector<ReferenceData> AdditionalEffects;
};

std::ostream& operator<<(std::ostream& out, const ReferenceData& reference);