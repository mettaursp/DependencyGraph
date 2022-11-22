#include "GraphPrinting.h"

#include <iostream>

#include "XmlParsing.h"

const ReferenceData& GraphData::Dereference(const ReferenceData& reference)
{
	int id = reference.Id;

	if (reference.Type == ReferenceType::Skill)
	{
		if (ReferencedSkills.contains(id))
			return reference;

		QueuedSkills.push_back(id);
		ReferencedSkills[id] = std::vector<ReferenceData>();

		return reference;
	}

	if (ReferencedEffects.contains(id))
		return reference;

	QueuedEffects.push_back(id);
	ReferencedEffects[id] = std::vector<ReferenceData>();

	return reference;
}

void GraphData::Print(const ReferenceData& caller, const ConditionSkill& trigger, const std::string& style)
{
	std::vector<ReferenceData>& references = caller.Type == ReferenceType::Skill ? ReferencedSkills[caller.Id] : ReferencedEffects[caller.Id];

	if (trigger.RandomCasts.size() > 0)
	{
		for (int i = 0; i < trigger.RandomCasts.size(); ++i)
		{
			const ReferenceData& cast = trigger.RandomCasts[i];

			bool linkReferenced = false;

			for (int i = 0; i < references.size() && !linkReferenced; ++i)
				if (references[i].Type == cast.Type && references[i].Id == cast.Id)
					linkReferenced = true;

			references.push_back(cast);

			OutFile << "\t" << caller << " -> " << Dereference(cast) << " " << style << " [color=\"orange\"]\n";
		}

		return;
	}

	for (int i = 0; i < references.size(); ++i)
		if (references[i].Type == trigger.Reference.Type && references[i].Id == trigger.Reference.Id)
			return;

	references.push_back(trigger.Reference);

	OutFile << "\t" << caller << " -> " << Dereference(trigger.Reference) << " " << style << "\n";
}

void GraphData::PrintRoot(const JobSkill& jobSkill)
{

	auto skillIndex = skills.find(jobSkill.Skill.Id);

	if (skillIndex == skills.end())
		return;

	const SkillData& skill = skillIndex->second;
	const SkillLevelData& skillLevel = skill.Levels.begin()->second;

	OutFile << "\t" << RootName << " -> " << Dereference(jobSkill.Skill) << "\n";

	for (int j = 0; j < jobSkill.SubSkills.size(); ++j)
		OutFile << "\t" << jobSkill.Skill << " -> " << Dereference(jobSkill.SubSkills[j]) << " [dir=none color=\"blue\"]\n";

	ReferenceData combo = jobSkill.Skill;

	const SkillData* comboSkill = &skill;
	const SkillLevelData* comboSkillLevel = &skillLevel;

	std::vector<int> visitedCombos;

	while (combo.Id != 0 && comboSkillLevel != nullptr)
	{
		const ReferenceData& nextCombo = comboSkillLevel->Combo.OutputSkill;

		bool alreadyVisited = false;

		for (int i = 0; i < visitedCombos.size() && !alreadyVisited; ++i)
			if (visitedCombos[i] == nextCombo.Id)
				alreadyVisited = true;

		if (alreadyVisited)
			break;

		auto comboSkillIndex = skills.find(nextCombo.Id);

		if (comboSkillIndex == skills.end())
			break;

		visitedCombos.push_back(combo.Id);

		OutFile << "\t" << combo << " -> " << Dereference(nextCombo) << " [color=\"green\"]\n";

		combo = nextCombo;
		comboSkill = &comboSkillIndex->second;
		comboSkillLevel = &comboSkill->Levels.begin()->second;
	}

	for (int i = 0; i < skillLevel.ChangeSkillReferences.size(); ++i)
	{
		const ChangeSkillReference& changeSkill = skillLevel.ChangeSkillReferences[i];

		if (changeSkill.Skill.Id == 0)
			continue;

		OutFile << "\t" << jobSkill.Skill << " -> " << Dereference(changeSkill.Skill) << " [color=\"red\"]\n";
	}
}

void GraphData::PrintRoot(const JobData& jobData)
{
	OutFile << "\t" << RootName << " [label=\"" << RootLabel << "\"]\n";

	for (int i = 0; i < jobData.Skills.size(); ++i)
		PrintRoot(jobData.Skills[i]);
}

void GraphData::PrintLinked()
{

	int skillIndex = 0;
	int effectIndex = 0;

	while (QueuedSkills.size() > skillIndex || QueuedEffects.size() > effectIndex)
	{
		while (QueuedSkills.size() > skillIndex)
		{
			int skillId = QueuedSkills[skillIndex];

			auto skillContainerIndex = skills.find(skillId);

			if (skillContainerIndex == skills.end())
			{
				++skillIndex;

				continue;
			}

			SkillData& skill = skillContainerIndex->second;

			OutFile << "\t" << ReferenceData{ ReferenceType::Skill, skillId , 0 } << "[label=\"Skill " << skillId;

			if (skill.Name != "")
				OutFile << "\\n" << skill.Name;

			OutFile << "\"shape=box]\n";

			SkillLevelData& skillLevel = skill.Levels.begin()->second;

			for (int i = 0; i < skillLevel.Passives.size(); ++i)
			{
				Print(ReferenceData{ ReferenceType::Skill, skillId , 0 }, skillLevel.Passives[i], "[color=\"purple\"]");
			}

			for (int m = 0; m < skillLevel.Motions.size(); ++m)
			{
				SkillMotion& motion = skillLevel.Motions[m];

				for (int a = 0; a < motion.Attacks.size(); ++a)
				{
					SkillAttack& attack = motion.Attacks[a];

					for (int i = 0; i < attack.Triggers.size(); ++i)
					{
						Print(ReferenceData{ ReferenceType::Skill, skillId , 0 }, attack.Triggers[i]);
					}
				}
			}

			++skillIndex;
		}

		while (QueuedEffects.size() > effectIndex)
		{
			int effectId = QueuedEffects[effectIndex];

			auto effectContainerIndex = effects.find(effectId);

			if (effectContainerIndex == effects.end())
			{
				++effectIndex;

				continue;
			}

			AdditionalEffectData& effect = effectContainerIndex->second;

			OutFile << "\t" << ReferenceData{ ReferenceType::Effect, effectId , 0 } << "[label=\"Effect " << effectId;

			AdditionalEffectLevelData& effectLevel = effect.Levels.begin()->second;

			if (effectLevel.Name != "")
				OutFile << "\\n" << effectLevel.Name;

			OutFile << "\"shape=ellipse]\n";

			for (int i = 0; i < effectLevel.Triggers.size(); ++i)
			{
				Print(ReferenceData{ ReferenceType::Effect, effectId , 0 }, effectLevel.Triggers[i]);
			}

			++effectIndex;
		}
	}
}

void GraphData::PrintRoot(const SetBonusData& setData)
{
	OutFile << "\t" << RootName << " [label=\"" << RootLabel << "\"]\n";

	for (int i = 0; i < setData.OptionData->Parts.size(); ++i)
	{
		const SetBonusOptionPartData& part = setData.OptionData->Parts[i];

		OutFile << "\tset" << part.Count << " [label=\"" << part.Count << " Piece Bonus\"]\n";
		OutFile << "\t" << RootName << " -> set" << part.Count << "\n";

		for (int j = 0; j < part.AdditionalEffects.size(); ++j)
			OutFile << "\tset" << part.Count << " -> " << Dereference(part.AdditionalEffects[j]) << "\n";
	}
}