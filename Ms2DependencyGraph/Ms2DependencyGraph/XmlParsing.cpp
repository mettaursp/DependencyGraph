#include "XmlParsing.h"

std::unordered_map<int, MagicPathData> magicPaths;
std::unordered_map<int, AdditionalEffectData> effects;
std::unordered_map<int, SkillData> skills;
std::unordered_map<JobCode, JobData> jobs;
std::unordered_map<int, SetBonusOptionData> setBonusOptions;
std::unordered_map<int, SetBonusData> setBonuses;
std::unordered_map<int, ItemData> items;

void ParseMagicPaths(const fs::path& filePath)
{
	tinyxml2::XMLDocument document;

	document.LoadFile(filePath.string().c_str());

	tinyxml2::XMLElement* rootElement = document.RootElement();

	for (tinyxml2::XMLElement* typeElement = rootElement->FirstChildElement(); typeElement; typeElement = typeElement->NextSiblingElement())
	{
		int id = readAttribute<int>(typeElement, "id", 0);

		MagicPathData& path = magicPaths[id];

		for (tinyxml2::XMLElement* moveElement = typeElement->FirstChildElement(); moveElement; moveElement = moveElement->NextSiblingElement())
		{
			path.Moves.push_back(MagicPathMove{});

			MagicPathMove& move = path.Moves.back();

			bool align = readAttribute<int>(moveElement, "align", 0) == 1;

			if (align)
				++path.Aligned;

			move.Velocity = readAttribute<float>(moveElement, "vel", 0);
		}
	}
}

void ParseBeginCondition(tinyxml2::XMLElement* node, BeginCondition& condition)
{
	for (tinyxml2::XMLElement* conditionElement = node->FirstChildElement(); conditionElement; conditionElement = conditionElement->NextSiblingElement())
	{
		const char* name = conditionElement->Name();

		if (strcmp(name, "requireSkillCodes") == 0)
		{
			int skillId = readAttribute<int>(conditionElement, "code", 0);

			condition.RequireSkillCodes.push_back(skillId);

			continue;
		}

		if (strcmp(name, "owner") != 0 && strcmp(name, "target") != 0 && strcmp(name, "caster") != 0)
			continue;

		SkillTarget target = SkillTarget::Owner;

		if (strcmp(name, "target") != 0)
			target = SkillTarget::Target;

		if (strcmp(name, "caster") != 0)
			target = SkillTarget::Caster;

		int hasBuffID = readAttribute<int>(conditionElement, "hasBuffID", 0);
		EventCondition eventCondition = (EventCondition)readAttribute<int>(conditionElement, "eventCondition", 0);

		if (eventCondition != EventCondition::None)
		{
			condition.EventTarget = target;
			condition.EventCondition = eventCondition;
		}

		if (hasBuffID != 0)
		{
			int hasBuffLevel = readAttribute<int>(conditionElement, "hasBuffLevel", 0);

			condition.References.push_back(TriggerReferenceData{ ReferenceType::Effect, hasBuffID, hasBuffLevel, target, ConditionReferenceType::Require });
		}

		int hasSkillID = readAttribute<int>(conditionElement, "hasSkillID", 0);

		if (hasSkillID != 0)
		{
			int hasSkillLevel = readAttribute<int>(conditionElement, "hasSkillLevel", 0);

			condition.References.push_back(TriggerReferenceData{ ReferenceType::Skill, hasSkillID, hasSkillLevel, target, ConditionReferenceType::Require });
		}

		int hasNotBuffID = readAttribute<int>(conditionElement, "hasNotBuffID", 0);

		if (hasNotBuffID != 0)
			condition.References.push_back(TriggerReferenceData{ ReferenceType::Effect, hasNotBuffID, 0, target, ConditionReferenceType::Prevent });

		int ignoreOwnerEvent = readAttribute<int>(conditionElement, "ignoreOwnerEvent", 0);

		if (ignoreOwnerEvent != 0)
			condition.References.push_back(TriggerReferenceData{ ReferenceType::Effect, ignoreOwnerEvent, 0, target, ConditionReferenceType::Ignore });

		std::vector<int> eventIDs;

		readAttribute(conditionElement, "eventSkillID", eventIDs);

		for (int i = 0; i < eventIDs.size(); ++i)
			condition.References.push_back(TriggerReferenceData{ ReferenceType::Skill, eventIDs[i], 0, target, ConditionReferenceType::RequireEvent });

		int eventIgnoreSkillID = readAttribute<int>(conditionElement, "eventIgnoreSkillID", 0);

		if (eventIgnoreSkillID != 0)
			condition.References.push_back(TriggerReferenceData{ ReferenceType::Skill, eventIgnoreSkillID, 0, target, ConditionReferenceType::Ignore });

		eventIDs.clear();

		readAttribute(conditionElement, "eventEffectID", eventIDs);

		for (int i = 0; i < eventIDs.size(); ++i)
			condition.References.push_back(TriggerReferenceData{ ReferenceType::Effect, eventIDs[i], 0, target, ConditionReferenceType::RequireEvent });
	}

	if (condition.References.size() > 1)
		node = node;
}

void ParseConditionSkill(tinyxml2::XMLElement* node, ConditionSkill& conditionSkill)
{
	conditionSkill.IsSplash = readAttribute<bool>(node, "splash", 0);
	conditionSkill.OnlySensingActive = readAttribute<bool>(node, "onlySensingActive", 0);
	conditionSkill.NonTargetActive = readAttribute<bool>(node, "nonTargetActive", 0);
	conditionSkill.SkillTarget = (SkillTarget)readAttribute<int>(node, "skillTarget", 0);
	conditionSkill.SkillOwner = (SkillTarget)readAttribute<int>(node, "skillOwner", 0);
	conditionSkill.Reference.Type = (ReferenceType)readAttribute<int>(node, "splash", 0);
	conditionSkill.Reference.Id = readAttribute<int>(node, "skillID", 0);
	conditionSkill.Reference.Level = readAttribute<int>(node, "level", 0);

	if (readAttribute<bool>(node, "randomCast", false))
	{
		std::vector<int> randomCasts;

		readAttribute(node, "skillID", randomCasts);

		for (int i = 0; i < randomCasts.size(); ++i)
			conditionSkill.RandomCasts.push_back(ReferenceData{ ReferenceType::Effect, randomCasts[i], 0 });
	}

	int removeDelay = readAttribute<int>(node, "removeDelay", 0);
	int interval = readAttribute<int>(node, "interval", 0);
	int fireCount = readAttribute<int>(node, "fireCount", 0);
	bool immediateActive = readAttribute<bool>(node, "immediateActive", 0);

	ParseBeginCondition(node->FirstChildElement(), conditionSkill.Condition);
}

void ParseAdditionalEffect(const fs::path& filePath)
{
	int effectId = atoi(filePath.stem().string().c_str());

	AdditionalEffectData& effect = effects[effectId];

	tinyxml2::XMLDocument document;

	document.LoadFile(filePath.string().c_str());

	tinyxml2::XMLElement* rootElement = document.RootElement();

	SupportSettings fileFeature;
	SupportSettings fileLocale;

	if (!isNodeEnabled(rootElement, &fileFeature, &fileLocale))
		return;

	for (tinyxml2::XMLElement* levelElement = rootElement->FirstChildElement(); levelElement; levelElement = levelElement->NextSiblingElement())
	{
		tinyxml2::XMLElement* basicPropertyElement = levelElement->FirstChildElement("BasicProperty");

		if (basicPropertyElement == nullptr)
			continue;

		int level = readAttribute<int>(basicPropertyElement, "level", 0);

		AdditionalEffectLevelData& levelData = effect.Levels[level];

		if (!isNodeEnabled(levelElement, &levelData.Feature, &levelData.Locale))
			continue;

		levelData = AdditionalEffectLevelData(levelData.Feature, levelData.Locale);

		levelData.Type = readAttribute<int>(basicPropertyElement, "type", 0);
		levelData.SubType = readAttribute<int>(basicPropertyElement, "subType", 0);
		levelData.KeepCondition = readAttribute<int>(basicPropertyElement, "keepCondition", 0);
		levelData.ResetCondition = readAttribute<int>(basicPropertyElement, "resetCondition", 0);
		levelData.MaxStacks = readAttribute<int>(basicPropertyElement, "maxBuffCount", 0);
		levelData.Group = readAttribute<int>(basicPropertyElement, "group", 0);

		if (levelData.ResetCondition == 1 && levelData.MaxStacks > 1)
		{
			levelData.MaxStacks += 0;
		}
		if (levelData.ResetCondition == 0 && levelData.MaxStacks > 1)
		{
			levelData.MaxStacks += 0;
		}

		for (tinyxml2::XMLElement* propertyElement = levelElement->FirstChildElement(); propertyElement; propertyElement = propertyElement->NextSiblingElement())
		{
			const char* propertyName = propertyElement->Name();

			if (strcmp(propertyName, "splashSkill") == 0 || strcmp(propertyName, "conditionSkill") == 0)
			{
				levelData.Triggers.push_back(ConditionSkill());

				ParseConditionSkill(propertyElement, levelData.Triggers.back());

				continue;
			}

			if (strcmp(propertyName, "beginCondition") == 0)
			{
				ParseBeginCondition(propertyElement, levelData.Condition);

				continue;
			}

			if (strcmp(propertyName, "ModifyOverlapCountProperty") == 0)
			{
				std::vector<int> effectCodes;
				std::vector<int> offsetCounts;

				readAttribute(propertyElement, "effectCodes", effectCodes);
				readAttribute(propertyElement, "offsetCounts", offsetCounts);

				for (int i = 0; i < effectCodes.size(); ++i)
					levelData.Modifications.push_back(ModifyReference{ ReferenceType::Effect, effectCodes[i], 0, ModifyReferenceType::ModifyStacks, offsetCounts.size() > 0 ? offsetCounts[i] : 0 });

				continue;
			}

			if (strcmp(propertyName, "ModifyEffectDurationProperty") == 0)
			{
				std::vector<int> effectCodes;

				readAttribute(propertyElement, "effectCodes", effectCodes);

				for (int i = 0; i < effectCodes.size(); ++i)
					levelData.Modifications.push_back(ModifyReference{ ReferenceType::Effect, effectCodes[i], 0, ModifyReferenceType::ModifyDuration, 0 });

				continue;
			}

			if (strcmp(propertyName, "ResetSkillCoolDownTimeProperty") == 0)
			{
				std::vector<int> skillCodes;

				readAttribute(propertyElement, "skillCodes", skillCodes);

				for (int i = 0; i < skillCodes.size(); ++i)
					levelData.Modifications.push_back(ModifyReference{ ReferenceType::Skill, skillCodes[i], 0, ModifyReferenceType::ResetCooldown, 0 });

				continue;
			}

			if (strcmp(propertyName, "ImmuneEffectProperty") == 0)
			{
				std::vector<int> immuneCodes;

				readAttribute(propertyElement, "immuneEffectCodes", immuneCodes);

				for (int i = 0; i < immuneCodes.size(); ++i)
					levelData.Modifications.push_back(ModifyReference{ ReferenceType::Effect, immuneCodes[i], 0, ModifyReferenceType::Immune, 0 });

				immuneCodes.clear();

				readAttribute(propertyElement, "immuneBuffCategories", immuneCodes);

				for (int i = 0; i < immuneCodes.size(); ++i)
					levelData.Modifications.push_back(ModifyReference{ ReferenceType::EffectCategory, immuneCodes[i], 0, ModifyReferenceType::Immune, 0 });

				continue;
			}

			if (strcmp(propertyName, "CancelEffectProperty") == 0)
			{
				std::vector<int> cancelCodes;

				readAttribute(propertyElement, "cancelEffectCodes", cancelCodes);

				for (int i = 0; i < cancelCodes.size(); ++i)
					levelData.Modifications.push_back(ModifyReference{ ReferenceType::Effect, cancelCodes[i], 0, ModifyReferenceType::Cancel, 0 });

				cancelCodes.clear();

				readAttribute(propertyElement, "cancelBuffCategories", cancelCodes);

				for (int i = 0; i < cancelCodes.size(); ++i)
					levelData.Modifications.push_back(ModifyReference{ ReferenceType::EffectCategory, cancelCodes[i], 0, ModifyReferenceType::Cancel, 0 });

				continue;
			}
		}

		for (const auto& trigger : levelData.Triggers)
		{
			if (trigger.Reference.Id == effectId && trigger.Reference.Level != level && trigger.Reference.Type == ReferenceType::Effect)
				effect.ScalingLevels = false;

			for (const auto& cast : trigger.RandomCasts)
				if (cast.Id == effectId && cast.Level != level && trigger.Reference.Type == ReferenceType::Effect)
					effect.ScalingLevels = false;
		}
	}
}

void ParseSkill(const fs::path& filePath)
{
	int skillId = atoi(filePath.stem().string().c_str());

	SkillData& skill = skills[skillId];

	tinyxml2::XMLDocument document;

	document.LoadFile(filePath.string().c_str());

	tinyxml2::XMLElement* rootElement = document.RootElement();

	SupportSettings fileFeature;
	SupportSettings fileLocale;

	if (!isNodeEnabled(rootElement, &fileFeature, &fileLocale))
		return;

	for (tinyxml2::XMLElement* childElement = rootElement->FirstChildElement(); childElement; childElement = childElement->NextSiblingElement())
	{
		const char* childName = childElement->Name();

		if (strcmp(childName, "basic") == 0)
		{
			if (!isNodeEnabled(childElement, &skill.Feature, &skill.Locale))
				continue;

			tinyxml2::XMLElement* kindsElement = childElement->FirstChildElement("kinds");

			if (kindsElement == nullptr)
				continue;

			skill.Type = readAttribute<int>(kindsElement, "type", 0);
			skill.SubType = readAttribute<int>(kindsElement, "subType", 0);
			skill.ImmediateActive = readAttribute<bool>(kindsElement, "immediateActive", false);

			continue;
		}

		if (strcmp(childName, "level") != 0)
			continue;

		int level = readAttribute<int>(childElement, "value", 0);

		SupportSettings levelFeature;
		SupportSettings levelLocale;

		if (!isNodeEnabled(childElement, &levelFeature, &levelLocale))
			continue;

		SkillLevelData& levelData = skill.Levels[level];

		if (!isNodeEnabled(childElement, &levelData.Feature, &levelData.Locale))
			continue;

		levelData = SkillLevelData(levelData.Feature, levelData.Locale);

		int splashLife = -1;
		int splashCooldown = -1;

		for (tinyxml2::XMLElement* propertyElement = childElement->FirstChildElement(); propertyElement; propertyElement = propertyElement->NextSiblingElement())
		{
			const char* propertyName = propertyElement->Name();

			if (strcmp(propertyName, "beginCondition") == 0)
			{
				ParseBeginCondition(propertyElement, levelData.Condition);

				continue;
			}

			if (strcmp(propertyName, "changeSkill") == 0)
			{
				std::vector<int> effectID;
				std::vector<int> effectLevel;
				std::vector<int> effectStacks;

				readAttribute(propertyElement, "changeSkillCheckEffectID", effectID);
				readAttribute(propertyElement, "changeSkillCheckEffectLevel", effectLevel);
				readAttribute(propertyElement, "changeSkillCheckEffectOverlapCount", effectStacks);

				levelData.ChangeSkillReferences.resize(effectID.size());

				for (int i = 0; i < effectID.size(); ++i)
					levelData.ChangeSkillReferences[i].Effect = EffectReferenceData{ReferenceType::Effect, effectID[i], effectLevel[i], effectStacks[i], 0};

				effectID.clear();
				effectLevel.clear();

				readAttribute(propertyElement, "changeSkillID", effectID);
				readAttribute(propertyElement, "changeSkillLevel", effectLevel);

				for (int i = 0; i < levelData.ChangeSkillReferences.size(); ++i)
					levelData.ChangeSkillReferences[i].Skill = ReferenceData{ReferenceType::Skill, effectID[i], effectLevel[i]};

				int originSkillID = readAttribute<int>(propertyElement, "originSkillID", 0);
				int originSkillLevel = readAttribute<int>(propertyElement, "originSkillLevel", 0);

				for (int i = 0; i < levelData.ChangeSkillReferences.size(); ++i)
					levelData.ChangeSkillReferences[i].OriginSkill = ReferenceData{ ReferenceType::Skill, originSkillID, originSkillLevel };

				continue;
			}

			if (strcmp(propertyName, "combo") == 0)
			{
				levelData.Combo.IsCombo = readAttribute<bool>(propertyElement, "comboSkill", false);
				levelData.Combo.IsCharging = readAttribute<bool>(propertyElement, "chargingSkill", false);
				levelData.Combo.OriginSkill = ReferenceData{ ReferenceType::Skill, readAttribute<int>(propertyElement, "comboOriginSkill", 0), 0 };
				levelData.Combo.InputSkill = ReferenceData{ ReferenceType::Skill, readAttribute<int>(propertyElement, "inputSkill", 0), 0 };
				levelData.Combo.OutputSkill = ReferenceData{ ReferenceType::Skill, readAttribute<int>(propertyElement, "outputSkill", 0), 0 };

				continue;
			}

			if (strcmp(propertyName, "conditionSkill") == 0)
			{
				levelData.Passives.push_back(ConditionSkill());

				ParseConditionSkill(propertyElement, levelData.Passives.back());
			}

			if (strcmp(propertyName, "motion") == 0)
			{
				levelData.Motions.push_back(SkillMotion{});

				SkillMotion& motion = levelData.Motions.back();

				for (tinyxml2::XMLElement* motionNodeElement = propertyElement->FirstChildElement(); motionNodeElement; motionNodeElement = motionNodeElement->NextSiblingElement())
				{
					const char* motionNodeName = motionNodeElement->Name();

					if (strcmp(motionNodeName, "motionProperty") == 0)
					{
						int splashLifeTick = readAttribute<int>(motionNodeElement, "splashLifeTick", 0);
						int splashInvokeCoolTick = readAttribute<int>(motionNodeElement, "splashInvokeCoolTick", 0);

						if (splashLifeTick != 0)
						{
							if (splashLife != -1 && splashLife != splashLifeTick)
								splashLifeTick += 0;

							splashLife = splashLifeTick;

							if (splashCooldown != -1 && splashCooldown != splashInvokeCoolTick)
								splashInvokeCoolTick += 0;

							splashCooldown = splashInvokeCoolTick;
						}

						continue;
					}

					if (strcmp(motionNodeName, "attack") != 0)
						continue;

					motion.Attacks.push_back(SkillAttack{});

					SkillAttack& attack = motion.Attacks.back();

					attack.MagicPathId = readAttribute<int>(motionNodeElement, "magicPathID", 0);
					attack.CubeMagicPathId = readAttribute<int>(motionNodeElement, "cubeMagicPathID", 0);

					++levelData.TotalAttacks;

					bool hasSplash = false;

					for (tinyxml2::XMLElement* childElement = motionNodeElement->FirstChildElement(); childElement; childElement = childElement->NextSiblingElement())
					{
						const char* childName = childElement->Name();

						if (strcmp(childName, "rangeProperty") == 0)
						{
							attack.CastTarget = (ApplyTarget)readAttribute<int>(childElement, "castTarget", 0);
							attack.ApplyTarget = (ApplyTarget)readAttribute<int>(childElement, "applyTarget", 0);

							continue;
						}

						if (strcmp(childName, "damageProperty") == 0)
						{
							attack.AttackMaterial = readAttribute<unsigned char>(childElement, "attackMaterial", 0);

							continue;
						}

						if (strcmp(childName, "conditionSkill") != 0)
							continue;

						attack.Triggers.push_back(ConditionSkill());

						ParseConditionSkill(childElement, attack.Triggers.back());

						hasSplash |= attack.Triggers.back().IsSplash;
					}

					if (attack.CubeMagicPathId != 0 && !hasSplash)
						levelData.TotalAttacks += 0;

					if (attack.MagicPathId != 0 && !hasSplash)
						levelData.TotalAttacks += 0;

				}

				for (int i = 0; i < motion.Attacks.size(); ++i)
				{
					SkillAttack& attack = motion.Attacks[i];

					if (attack.MagicPathId != 0)
						motion.TotalPaths++;

					if (attack.CubeMagicPathId != 0)
						motion.TotalCubePaths++;
				}

				if (motion.TotalPaths > 1)
				{
					motion.TotalPaths += 0;
				}

				continue;
			}
		}

		for (const auto& motion : levelData.Motions)
		{
			for (const auto& attack : motion.Attacks)
			{
				for (const auto& trigger : attack.Triggers)
				{
					if (trigger.Reference.Id == skillId && trigger.Reference.Level != level && trigger.Reference.Type == ReferenceType::Skill)
						skill.ScalingLevels = false;
				}
			}
		}

		for (int i = 0; i < levelData.Motions.size(); ++i)
		{
			SkillMotion& motion = levelData.Motions[i];

			levelData.TotalPaths += motion.TotalPaths;
			levelData.TotalCubePaths += motion.TotalCubePaths;

			if (motion.TotalPaths > 0)
				levelData.TotalMotionsWithPaths++;

			if (motion.TotalCubePaths > 0)
				levelData.TotalMotionsWithCubePaths++;
		}

		if (levelData.Motions.size() > 1)
		{
			level += 0;
		}

		if (levelData.TotalPaths > 1)
		{
			levelData.TotalPaths += 0;
		}

		if (levelData.TotalMotionsWithPaths > 1)
		{
			levelData.TotalMotionsWithPaths += 0;
		}

		for (int i = 0; i < levelData.Motions.size(); ++i)
			if (levelData.Motions[i].TotalPaths > 1)
				level += 0;
	}
}

void ParseStrings(const fs::path& filePath)
{
	std::string fileNameString = filePath.stem().string();
	const char* fileName = fileNameString.c_str();

	const auto strcmp_s = [] <size_t Length> (const char* left, const char(&right)[Length])
	{
		return strncmp(left, right, Length - 1);
	};

	if (strcmp_s(fileName, "korskilldescription") == 0)
	{
		tinyxml2::XMLDocument document;

		document.LoadFile(filePath.string().c_str());

		tinyxml2::XMLElement* rootElement = document.RootElement();

		SupportSettings fileFeature;
		SupportSettings fileLocale;

		if (!isNodeEnabled(rootElement, &fileFeature, &fileLocale))
			return;

		for (tinyxml2::XMLElement* keyElement = rootElement->FirstChildElement(); keyElement; keyElement = keyElement->NextSiblingElement())
		{
			SupportSettings keyFeature;
			SupportSettings keyLocale;

			if (!isNodeEnabled(rootElement, &keyFeature, &keyLocale))
				continue;

			int skillId = readAttribute<int>(keyElement, "id", 0);

			auto skillIndex = skills.find(skillId);

			if (skillIndex == skills.end())
				continue;

			SkillData& skill = skillIndex->second;

			int skillLevel = readAttribute<int>(keyElement, "level", 0);

			auto skillLevelIndex = skill.Levels.find(skillLevel);

			if (skillLevelIndex == skill.Levels.end())
				continue;

			const tinyxml2::XMLAttribute* description = keyElement->FindAttribute("uiDescription");

			if (description == nullptr)
				continue;

			skillLevelIndex->second.Description = description->Value();
		}

		return;
	}

	if (strcmp_s(fileName, "skillname") == 0)
	{
		tinyxml2::XMLDocument document;

		document.LoadFile(filePath.string().c_str());

		tinyxml2::XMLElement* rootElement = document.RootElement();

		SupportSettings fileFeature;
		SupportSettings fileLocale;

		if (!isNodeEnabled(rootElement, &fileFeature, &fileLocale))
			return;

		for (tinyxml2::XMLElement* keyElement = rootElement->FirstChildElement(); keyElement; keyElement = keyElement->NextSiblingElement())
		{
			SupportSettings keyFeature;
			SupportSettings keyLocale;

			if (!isNodeEnabled(rootElement, &keyFeature, &keyLocale))
				continue;

			int skillId = readAttribute<int>(keyElement, "id", 0);

			auto skillIndex = skills.find(skillId);

			if (skillIndex == skills.end())
				continue;

			SkillData& skill = skillIndex->second;

			const tinyxml2::XMLAttribute* name = keyElement->FindAttribute("name");

			if (name == nullptr)
				continue;

			skill.Name = name->Value();
		}

		return;
	}

	if (strcmp_s(fileName, "koradditionaldescription") == 0)
	{
		tinyxml2::XMLDocument document;

		document.LoadFile(filePath.string().c_str());

		tinyxml2::XMLElement* rootElement = document.RootElement();

		SupportSettings fileFeature;
		SupportSettings fileLocale;

		if (!isNodeEnabled(rootElement, &fileFeature, &fileLocale))
			return;

		for (tinyxml2::XMLElement* keyElement = rootElement->FirstChildElement(); keyElement; keyElement = keyElement->NextSiblingElement())
		{
			SupportSettings keyFeature;
			SupportSettings keyLocale;

			if (!isNodeEnabled(rootElement, &keyFeature, &keyLocale))
				continue;

			int effectId = readAttribute<int>(keyElement, "id", 0);

			auto effectIndex = effects.find(effectId);

			if (effectIndex == effects.end())
				continue;

			AdditionalEffectData& effect = effectIndex->second;

			int effectLevel = readAttribute<int>(keyElement, "level", 0);

			auto effectLevelIndex = effect.Levels.find(effectLevel);

			if (effectLevelIndex == effect.Levels.end())
				continue;

			AdditionalEffectLevelData& level = effectLevelIndex->second;

			const tinyxml2::XMLAttribute* name = keyElement->FindAttribute("name");

			if (name != nullptr)
				level.Name = name->Value();

			const tinyxml2::XMLAttribute* description = keyElement->FindAttribute("tooltipDescription");

			if (description != nullptr)
				level.Description = description->Value();
		}

		return;
	}
}

void ParseJobs(const fs::path& filePath)
{
	tinyxml2::XMLDocument document;

	document.LoadFile(filePath.string().c_str());

	tinyxml2::XMLElement* rootElement = document.RootElement();

	SupportSettings fileFeature;
	SupportSettings fileLocale;

	if (!isNodeEnabled(rootElement, &fileFeature, &fileLocale))
		return;

	for (tinyxml2::XMLElement* jobElement = rootElement->FirstChildElement(); jobElement; jobElement = jobElement->NextSiblingElement())
	{
		SupportSettings jobFeature;
		SupportSettings jobLocale;

		if (!isNodeEnabled(jobElement, &jobFeature, &jobLocale))
			continue;

		JobCode jobCode = (JobCode)readAttribute<int>(jobElement, "code", 0);

		if (jobCode == JobCode::None)
			continue;

		JobData& job = jobs[jobCode];

		if (!isNodeEnabled(jobElement, &job.Feature, &job.Locale))
			continue;

		job = JobData(job.Feature, job.Locale);
		job.Job = jobCode;

		tinyxml2::XMLElement* skillsElement = jobElement->FirstChildElement("skills");

		if (skillsElement == nullptr)
			continue;

		for (tinyxml2::XMLElement* skillElement = skillsElement->FirstChildElement(); skillElement; skillElement = skillElement->NextSiblingElement())
		{
			job.Skills.push_back(JobSkill{});

			JobSkill& skill = job.Skills.back();

			int skillId = readAttribute<int>(skillElement, "main", 0);

			skill.Skill = ReferenceData{ ReferenceType::Skill, skillId, 0 };

			std::vector<int> subSkills;

			readAttribute(skillElement, "sub", subSkills);

			for (int i = 0; i < subSkills.size(); ++i)
				skill.SubSkills.push_back(ReferenceData{ ReferenceType::Skill, subSkills[i], 0 });
		}
	}
}

void ParseJobStrings(const fs::path& filePath)
{
	tinyxml2::XMLDocument document;

	document.LoadFile(filePath.string().c_str());

	tinyxml2::XMLElement* rootElement = document.RootElement();

	SupportSettings fileFeature;
	SupportSettings fileLocale;

	if (!isNodeEnabled(rootElement, &fileFeature, &fileLocale))
		return;

	for (tinyxml2::XMLElement* jobElement = rootElement->FirstChildElement(); jobElement; jobElement = jobElement->NextSiblingElement())
	{
		SupportSettings jobFeature;
		SupportSettings jobLocale;

		if (!isNodeEnabled(jobElement, &jobFeature, &jobLocale))
			continue;

		int jobCodeValue = readAttribute<int>(jobElement, "id", 0);
		int jobCodeRawValue = jobCodeValue / 10;
		JobCode jobCode = (JobCode)jobCodeRawValue;

		auto jobIndex = jobs.find(jobCode);

		bool isAwakening = jobCodeValue != 10 * jobCodeRawValue;

		if (jobIndex == jobs.end())
			continue;

		JobData& job = jobIndex->second;

		const tinyxml2::XMLAttribute* nameAttribute = jobElement->FindAttribute("name");

		if (nameAttribute == nullptr)
			continue;

		if (isAwakening)
			job.AwakenedName = nameAttribute->Value();
		else
			job.Name = nameAttribute->Value();
	}
}

void ParseSetBonusOptions(const fs::path& filePath)
{
	tinyxml2::XMLDocument document;

	document.LoadFile(filePath.string().c_str());

	tinyxml2::XMLElement* rootElement = document.RootElement();

	for (tinyxml2::XMLElement* optionElement = rootElement->FirstChildElement(); optionElement; optionElement = optionElement->NextSiblingElement())
	{
		int optionId = readAttribute<int>(optionElement, "id", 0);

		if (optionId == 0)
			continue;

		SetBonusOptionData& optionData = setBonusOptions[optionId];

		for (tinyxml2::XMLElement* partElement = optionElement->FirstChildElement(); partElement; partElement = partElement->NextSiblingElement())
		{
			optionData.Parts.push_back(SetBonusOptionPartData{});

			SetBonusOptionPartData& partData = optionData.Parts.back();

			partData.Count = readAttribute<int>(partElement, "count", 0);

			std::vector<int> effectIds;
			std::vector<int> effectLevels;

			readAttribute(partElement, "additionalEffectID", effectIds);
			readAttribute(partElement, "additionalEffectLevel", effectLevels);

			for (int i = 0; i < effectIds.size(); ++i)
				if (effectIds[i] != 0)
					partData.AdditionalEffects.push_back(ReferenceData{ ReferenceType::Effect, effectIds[i], effectLevels[i] });
		}
	}
}

void ParseSetBonuses(const fs::path& filePath)
{
	tinyxml2::XMLDocument document;

	document.LoadFile(filePath.string().c_str());

	tinyxml2::XMLElement* rootElement = document.RootElement();

	for (tinyxml2::XMLElement* setElement = rootElement->FirstChildElement(); setElement; setElement = setElement->NextSiblingElement())
	{
		SupportSettings setFeature;
		SupportSettings setLocale;

		if (!isNodeEnabled(setElement, &setFeature, &setLocale))
			continue;

		int setId = readAttribute<int>(setElement, "id", 0);
		int optionId = readAttribute<int>(setElement, "optionID", 0);

		if (setId == 0 || optionId == 0)
			continue;

		const auto optionIndex = setBonusOptions.find(optionId);

		if (optionIndex == setBonusOptions.end())
			continue;

		SetBonusData& setData = setBonuses[setId];

		if (!isNodeEnabled(setElement, &setData.Feature, &setData.Locale))
			continue;

		setData = SetBonusData(setData.Feature, setData.Locale);

		setData.OptionId = optionId;
		setData.OptionData = &optionIndex->second;

		readAttribute(setElement, "itemIDs", setData.ItemIds);
	}
}

void ParseSetBonusStrings(const fs::path& filePath)
{
	tinyxml2::XMLDocument document;

	document.LoadFile(filePath.string().c_str());

	tinyxml2::XMLElement* rootElement = document.RootElement();

	for (tinyxml2::XMLElement* keyElement = rootElement->FirstChildElement(); keyElement; keyElement = keyElement->NextSiblingElement())
	{
		int setId = readAttribute<int>(keyElement, "id", 0);

		const auto setIndex = setBonuses.find(setId);

		if (setIndex == setBonuses.end())
			continue;

		SetBonusData& setData = setIndex->second;

		if (!isNodeEnabled(keyElement, &setData.Feature, &setData.Locale))
			continue;

		const tinyxml2::XMLAttribute* nameAttribute = keyElement->FindAttribute("name");

		if (nameAttribute == nullptr)
			continue;

		setData.Name = nameAttribute->Value();
	}
}

ItemType GetItemType(int idDigits)
{
	switch (idDigits)
	{
		case 112: return ItemType::Earring;
		case 113: return ItemType::Hat;
		case 114: return ItemType::Clothes;
		case 115: return ItemType::Pants;
		case 116: return ItemType::Gloves;
		case 117: return ItemType::Shoes;
		case 118: return ItemType::Cape;
		case 119: return ItemType::Necklace;
		case 120: return ItemType::Ring;
		case 121: return ItemType::Belt;
		case 122: return ItemType::Overall;
		case 130: return ItemType::Bludgeon;
		case 131: return ItemType::Dagger;
		case 132: return ItemType::Longsword;
		case 133: return ItemType::Scepter;
		case 134: return ItemType::ThrowingStar;
		case 140: return ItemType::Spellbook;
		case 141: return ItemType::Shield;
		case 150: return ItemType::Greatsword;
		case 151: return ItemType::Bow;
		case 152: return ItemType::Staff;
		case 153: return ItemType::Cannon;
		case 154: return ItemType::Blade;
		case 155: return ItemType::Knuckle;
		case 156: return ItemType::Orb;
		case 209: return ItemType::Medal;
		case 410:
		case 420:
		case 430: return ItemType::Lapenshard;
		case 501:
		case 502:
		case 503:
		case 504:
		case 505: return ItemType::Furnishing;
		case 600: return ItemType::Pet;
		case 900: return ItemType::Currency;
		default: return ItemType::None;
	}
}

void ParseItems(const fs::path& filePath)
{
	int itemId = atoi(filePath.stem().string().c_str());

	ItemData& item = items[itemId];

	tinyxml2::XMLDocument document;

	document.LoadFile(filePath.string().c_str());

	tinyxml2::XMLElement* rootElement = document.RootElement();

	for (tinyxml2::XMLElement* environmentElement = rootElement->FirstChildElement(); environmentElement; environmentElement = environmentElement->NextSiblingElement())
	{
		if (!isNodeEnabled(environmentElement, &item.Feature, &item.Locale))
			continue;

		item = ItemData(item.Feature, item.Locale);

		item.Id = itemId;
		item.Type = GetItemType(itemId / 100000);

		tinyxml2::XMLElement* limit = environmentElement->FirstChildElement("limit");
		tinyxml2::XMLElement* additionalEffects = environmentElement->FirstChildElement("AdditionalEffect");
		tinyxml2::XMLElement* skills = environmentElement->FirstChildElement("skill");

		if (limit != nullptr)
			item.JobLimit = (JobCode)readAttribute<int>(limit, "jobLimit", 0);

		if (item.Type == ItemType::Lapenshard)
		{
			if (item.JobLimit != JobCode::None)
				jobs[item.JobLimit].Lapenshards.push_back(&item);
			else
				for (auto& jobPair : jobs)
					jobPair.second.Lapenshards.push_back(&item);
		}

		std::vector<int> referenceIds;
		std::vector<int> referenceLevels;

		if (additionalEffects != nullptr)
		{
			readAttribute(additionalEffects, "id", referenceIds);
			readAttribute(additionalEffects, "level", referenceLevels);

			for (int i = 0; i < referenceIds.size(); ++i)
			{
				int effectId = referenceIds[i];

				if (effectId == 0)
					continue;

				item.AdditionalEffects.push_back(ReferenceData{ ReferenceType::Effect, effectId, referenceLevels[i] });
			}
		}

		referenceIds.clear();
		referenceLevels.clear();

		if (skills != nullptr)
		{
			readAttribute(skills, "skillID", referenceIds);
			readAttribute(skills, "skillLevel", referenceLevels);

			for (int i = 0; i < referenceIds.size(); ++i)
			{
				int effectId = referenceIds[i];

				if (effectId == 0)
					continue;

				item.Skills.push_back(ReferenceData{ ReferenceType::Skill, effectId, referenceLevels[i] });
			}
		}
	}
}

void ParseItemStrings(const fs::path& filePath)
{
	tinyxml2::XMLDocument document;

	document.LoadFile(filePath.string().c_str());

	tinyxml2::XMLElement* rootElement = document.RootElement();

	for (tinyxml2::XMLElement* keyElement = rootElement->FirstChildElement(); keyElement; keyElement = keyElement->NextSiblingElement())
	{
		int itemId = readAttribute<int>(keyElement, "id", 0);

		if (itemId == 0)
			continue;

		auto itemIndex = items.find(itemId);

		if (itemIndex == items.end())
			continue;

		ItemData& item = itemIndex->second;

		if (!isNodeEnabled(keyElement, &item.Feature, &item.Locale))
			continue;

		const tinyxml2::XMLAttribute* nameAttribute = keyElement->FindAttribute("name");

		if (nameAttribute == nullptr)
			continue;

		item.Name = nameAttribute->Value();

		const tinyxml2::XMLAttribute* classAttribute = keyElement->FindAttribute("class");

		if (classAttribute == nullptr)
			continue;

		item.Class = classAttribute->Value();
	}
}

void ParseItemDescriptionStrings(const fs::path& filePath)
{
	tinyxml2::XMLDocument document;

	document.LoadFile(filePath.string().c_str());

	tinyxml2::XMLElement* rootElement = document.RootElement();

	for (tinyxml2::XMLElement* keyElement = rootElement->FirstChildElement(); keyElement; keyElement = keyElement->NextSiblingElement())
	{
		int itemId = readAttribute<int>(keyElement, "id", 0);

		if (itemId == 0)
			continue;

		auto itemIndex = items.find(itemId);

		if (itemIndex == items.end())
			continue;

		ItemData& item = itemIndex->second;

		if (!isNodeEnabled(keyElement, &item.Feature, &item.Locale))
			continue;

		const tinyxml2::XMLAttribute* tooltipAttribute = keyElement->FindAttribute("tooltipDescription");

		if (tooltipAttribute == nullptr)
			continue;

		item.Description = tooltipAttribute->Value();

		const tinyxml2::XMLAttribute* guideAttribute = keyElement->FindAttribute("guideDescription");

		if (guideAttribute == nullptr)
			continue;

		item.Description += guideAttribute->Value();
	}
}

std::string Sanitize(const std::string& text, bool isTooltip)
{
	int extraCharacters = 0;

	for (int i = 0; i < text.size(); ++i)
	{
		if (text[i] == '"' || text[i] == '\\')
			++extraCharacters;
		else if (text[i] == '\n' || text[i] == '\r')
			extraCharacters += isTooltip ? 5 : 1;
	}

	if (extraCharacters == 0)
		return text;

	std::string cleaned;

	cleaned.reserve(text.size() + extraCharacters);

	for (int i = 0; i < text.size(); ++i)
	{
		if (text[i] == '"' || text[i] == '\\')
			cleaned.push_back('\\');

		if (text[i] != '\n' && text[i] != '\r')
			cleaned.push_back(text[i]);
		else if (!isTooltip)
			cleaned.push_back(text[i] == '\n' ? 'n' : 'r');
		else
		{
			cleaned.push_back('&');
			cleaned.push_back('#');
			cleaned.push_back('0');
			cleaned.push_back('1');
			cleaned.push_back('3');
			cleaned.push_back(';');
		}
	}

	return cleaned;
}

std::string Desanitize(const std::string& text)
{
	int apostrophies = 0;
	int specialCharacterStart = -1;
	int specialCharacterLength = 0;

	for (int i = 0; i < text.size(); ++i)
	{
		if (text[i] == '&')
		{
			specialCharacterStart = i;
			specialCharacterLength = 0;
		}

		if (text[i] == ';' && specialCharacterStart != -1 && specialCharacterLength == 0)
		{
			specialCharacterLength = i - specialCharacterStart;

			if (strncmp(text.c_str() + specialCharacterStart, "&apos;", std::min(specialCharacterLength, 6)) == 0)
				++apostrophies;
		}
	}

	if (apostrophies == 0)
		return text;

	std::string cleaned;

	cleaned.resize(text.size() - 5 * apostrophies);

	int textIndex = 0;

	for (int i = 0; i < text.size(); ++i)
	{
		if (text[i] == '&')
		{
			specialCharacterStart = i;
			specialCharacterLength = 0;
		}

		if (specialCharacterStart == -1)
			cleaned[textIndex++] = text[i];

		if (text[i] == ';' && specialCharacterStart != -1 && specialCharacterLength == 0)
		{
			specialCharacterLength = i - specialCharacterStart;

			if (strncmp(text.c_str() + specialCharacterStart, "&apos;", std::min(specialCharacterLength, 6)) == 0)
				cleaned[textIndex++] = '\'';

			specialCharacterStart = -1;
		}
	}

	return cleaned;
}