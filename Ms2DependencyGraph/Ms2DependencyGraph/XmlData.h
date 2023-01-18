#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <filesystem>

#include "tinyxml2.h"
#include "ParserUtils.h"

enum class ReferenceType : unsigned char
{
	Effect = 0,
	Skill = 1,
	EffectCategory,

};

enum class SkillTarget : unsigned char
{
	SkillTarget = 0,
	Owner = 1,
	Target = 2,
	Caster = 3,
	PetOwner = 4,
	Attacker = 5
};

enum class ApplyTarget : unsigned char
{
	None = 0, // used on skill attacks whos sole purpose is deploying a region skill/self buff
	Enemy = 1,
	Ally = 2,
	Player1 = 3, // Unknown, 
	Player2 = 5, // Unknown, 
	Player3 = 6, // Unknown, Recovery
	Player4 = 7, // Unknown, Debuff (Archeon's ice bombs)

	HungryMobs = 8
};

const std::vector<const char*> SkillTargetNames = { "SkillTarget", "Owner", "Target", "Caster", "PetOwner", "Attacker" };

const std::vector<const char*> ApplyTargetNames = { "None", "Enemy", "Ally", "Player1", "Player2", "Player3", "Player4", "HungryMobs" };

enum class ConditionReferenceType : unsigned char
{
	Require,
	Prevent,
	RequireRange,
	Ignore,
	RequireEvent
};

enum class ModifyReferenceType : unsigned char
{
	Cancel,
	Immune,
	ResetCooldown,
	ModifyDuration,
	ModifyStacks
};

enum class JobCode : unsigned short
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

enum class EventCondition : unsigned char
{
	None = 0,
	OnEvade = 1,
	OnBlock = 2,
	OnAttacked = 4,
	OnOwnerAttackCrit = 5,
	OnOwnerAttackHit = 6,
	OnSkillCasted = 7,
	OnBuffStacksReached = 10,
	OnInvestigate = 11,
	OnBuffTimeExpiring = 13,
	OnSkillCastEnd = 14,
	OnEffectApplied = 16,
	OnEffectRemoved = 17,
	OnLifeSkillGather = 18,
	OnAttackMiss = 19,
	OnKritiasPuzzleEvent = 20,
	OnWizardHurricaneTransform = 102,
	OnStrikerFullCombo = 103
};

enum class AttackMaterial : unsigned short
{
	Unknown0 = 0,
	Unknown1 = 1,
	Unknown2 = 2,
	Unknown3 = 3,
	Unknown4 = 4,
	Unknown5 = 5,
	Unknown6 = 6,
	Unknown7 = 7,
	Unknown8 = 8,
	Unknown9 = 9,
	Unknown11 = 11,
	Unknown100 = 100,
	Unknown101 = 101,
	Unknown102 = 102,
	Unknown103 = 103,
	Unknown104 = 104,
	Unknown105 = 105,
	Unknown106 = 106,
	Unknown1000 = 1000,
	Unknown1103 = 1103
};

//additional effect StatusProperty.compulsionEventTypes
// 1: Crit chance override (can be applied to specific skills)
// 2: Evasion chance
// 3: Block chance

enum class CompulsionEventType : unsigned char
{
	CritChanceOverride = 1,
	EvasionChanceOverride = 2,
	BlockChance = 3
};

enum class CompulsionType : unsigned char
{
	Unknown1 = 1,
	AlwaysCrit = 2,
	Unknown3 = 3
};

const std::vector<const char*> EventConditionNames = ([]()
	{
		std::vector<const char*> names(104);

		names.resize(104);
		
		names[0] = "None";
		names[1] = "OnEvade";
		names[2] = "OnBlock";
		names[4] = "OnAttacked";
		names[5] = "OnOwnerAttackCrit";
		names[6] = "OnOwnerAttackHit";
		names[7] = "OnSkillCasted";
		names[10] = "OnBuffStacksReached";
		names[11] = "OnInvestigate";
		names[13] = "OnBuffTimeExpiring";
		names[14] = "OnSkillCastEnd";
		names[16] = "OnEffectApplied";
		names[17] = "OnEffectRemoved";
		names[18] = "OnLifeSkillGather";
		names[19] = "OnAttackMiss";
		names[20] = "OnKritiasPuzzleEvent";
		names[102] = "OnWizardHurricaneTransform";
		names[103] = "OnStrikerFullCombo";

		for (int i = 0; i < 104; ++i)
			if (names[i] == nullptr)
				names[i] = "Error: Unknown Event";

		return names;
	}
)();

struct MagicPathMove
{
	float Velocity = 0;
};

struct MagicPathData
{
	int Aligned = 0;
	std::vector<MagicPathMove> Moves;
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
	SkillTarget EventTarget = SkillTarget::SkillTarget;
	EventCondition EventCondition = EventCondition::None;
	std::vector<int> RequireSkillCodes;
};

struct ConditionSkill
{
	bool IsSplash = false;
	SkillTarget SkillOwner;
	SkillTarget SkillTarget;
	ReferenceData Reference;
	bool OnlySensingActive = false;
	bool NonTargetActive = false;

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
	bool ScalingLevels = true;
	std::unordered_map<int, AdditionalEffectLevelData> Levels;
};

struct AdditionalEffectLevelData
{
	std::string Name;
	std::string Description;

	SupportSettings Feature;
	SupportSettings Locale;
	
	short Type;
	short SubType;
	int ResetCondition = 0;
	int KeepCondition = 0;
	int MaxStacks = 0;
	int Group = 0;
	BeginCondition Condition;
	std::vector<ConditionSkill> Triggers;
	std::vector<ModifyReference> Modifications;

	AdditionalEffectLevelData() {}

	AdditionalEffectLevelData(const SupportSettings& feature, const SupportSettings& locale) : Feature(feature), Locale(locale) {}
};

struct SkillLevelData;

struct SkillData
{
	std::string Name;

	SupportSettings Feature;
	SupportSettings Locale;

	bool ImmediateActive = false;
	short Type = 0;
	short SubType = 0;
	bool ScalingLevels = true;
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
	int TotalPaths = 0;
	int TotalCubePaths = 0;

	std::vector<SkillAttack> Attacks;
};

struct SkillAttack
{
	int MagicPathId = 0;
	int CubeMagicPathId = 0;
	ApplyTarget CastTarget = ApplyTarget::None;
	ApplyTarget ApplyTarget = ApplyTarget::None;
	unsigned char AttackMaterial = 0;

	std::vector<ConditionSkill> Triggers;
};

struct SkillLevelData
{
	std::string Description;
	int TotalAttacks = 0;
	int TotalPaths = 0;
	int TotalCubePaths = 0;
	int TotalMotionsWithPaths = 0;
	int TotalMotionsWithCubePaths = 0;

	SupportSettings Feature;
	SupportSettings Locale;

	BeginCondition Condition;
	ComboReference Combo;
	std::vector<ConditionSkill> Passives;
	std::vector<ChangeSkillReference> ChangeSkillReferences;
	std::vector<SkillMotion> Motions;

	SkillLevelData() {}

	SkillLevelData(const SupportSettings& feature, const SupportSettings& locale) : Feature(feature), Locale(locale) {}
};

struct JobSkill
{
	ReferenceData Skill;

	std::vector<ReferenceData> SubSkills;
};

struct ItemData;

struct JobData
{
	JobCode Job = JobCode::None;
	std::string Name;
	std::string AwakenedName;

	SupportSettings Feature;
	SupportSettings Locale;

	std::vector<JobSkill> Skills;
	std::vector<ItemData*> Lapenshards;

	JobData() {}

	JobData(const SupportSettings& feature, const SupportSettings& locale) : Feature(feature), Locale(locale) {}
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

	SetBonusData() {}

	SetBonusData(const SupportSettings& feature, const SupportSettings& locale) : Feature(feature), Locale(locale) {}
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

enum class ItemType
{
	None,
	Currency,
	Furnishing,
	Pet,
	Lapenshard,
	Medal,
	Earring = 12,
	Hat = 13,
	Clothes = 14,
	Pants = 15,
	Gloves = 16,
	Shoes = 17,
	Cape = 18,
	Necklace = 19,
	Ring = 20,
	Belt = 21,
	Overall = 22,
	Bludgeon = 30,
	Dagger = 31,
	Longsword = 32,
	Scepter = 33,
	ThrowingStar = 34,
	Spellbook = 40,
	Shield = 41,
	Greatsword = 50,
	Bow = 51,
	Staff = 52,
	Cannon = 53,
	Blade = 54,
	Knuckle = 55,
	Orb = 56
};

struct ItemData
{
	std::string Name;
	std::string Class;
	std::string Description;

	SupportSettings Feature;
	SupportSettings Locale;

	int Id = 0;
	ItemType Type = ItemType::None;
	JobCode JobLimit = JobCode::None;
	std::vector<ReferenceData> AdditionalEffects;
	std::vector<ReferenceData> Skills;

	ItemData() {}

	ItemData(const SupportSettings& feature, const SupportSettings& locale) : Feature(feature), Locale(locale) {}
};

std::ostream& operator<<(std::ostream& out, const ReferenceData& reference);