#include "GraphPrinting.h"

#include <iostream>

#include "XmlParsing.h"

const ReferenceData& GraphData::Dereference(const ReferenceData& reference, bool isSplash)
{
	int id = reference.Id;

	ReferenceData data = reference;

	if (reference.Type == ReferenceType::Skill)
	{
		if (isSplash && !ReferencedSplashSkills.contains(id))
			ReferencedSplashSkills.insert(id);

		if (skills[id].ScalingLevels)
			data.Level = -1;

		References& refs = ReferencedSkills[id];

		refs.Reference = data;
		
		if (refs.LevelReferences.contains(data.Level))
			return refs.Reference;

		QueuedSkills.push_back(data);
		refs.LevelReferences[data.Level] = std::vector<ReferenceData>();

		return QueuedSkills.back();
	}

	if (effects[id].ScalingLevels)
		data.Level = -1;

	References& refs = ReferencedEffects[id];
	refs.Reference = data;

	if (refs.LevelReferences.contains(data.Level))
		return refs.Reference;

	QueuedEffects.push_back(data);
	refs.LevelReferences[data.Level] = std::vector<ReferenceData>();

	return QueuedEffects.back();
}

void GraphData::Print(const ReferenceData& caller, const ConditionSkill& trigger, const std::string& style, int index1, int index2, const SkillAttack* attack)
{
	std::vector<ReferenceData>& references = caller.Type == ReferenceType::Skill ? ReferencedSkills[caller.Id].LevelReferences[caller.Level] : ReferencedEffects[caller.Id].LevelReferences[caller.Level];

	std::stringstream outRefStream;

	outRefStream << caller;

	std::string outRef;
	bool printTarget = Settings.PrintTarget && trigger.SkillTarget != SkillTarget::SkillTarget;

	if (index1 != -1 && trigger.Condition.EventCondition != EventCondition::None)
	{
		outRefStream << "_" << index1;

		if (index2 != -1)
			outRefStream << "_" << index2;

		outRef = outRefStream.str();

		OutFile << "\t" << caller << " -> " << outRef;

		if (printTarget)
		{
			OutFile << " [label=\"" << SkillTargetNames[(int)trigger.SkillTarget] << "\"]";
		}

		printTarget = false;

		OutFile << "\n";

		OutFile << "\t" << outRef << " [label=\"" << SkillTargetNames[(int)trigger.Condition.EventTarget] << "\\n" << EventConditionNames[(int)trigger.Condition.EventCondition] << "\" shape=component]\n";
	}
	else
		outRef = outRefStream.str();

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

			OutFile << "\t" << outRef << " -> " << Dereference(cast, trigger.IsSplash) << " [color=\"orange\"";
			
			if (printTarget)
				OutFile << " label=\"" << SkillTargetNames[(int)trigger.SkillTarget] << "\"";
			
			OutFile << "]\n";
		}

		return;
	}

	for (int i = 0; i < references.size(); ++i)
		if (references[i].Type == trigger.Reference.Type && references[i].Id == trigger.Reference.Id)
			return;

	references.push_back(trigger.Reference);

	OutFile << "\t" << outRef << " -> " << Dereference(trigger.Reference, trigger.IsSplash);
	
	//if (printTarget)
	//	OutFile << " label=\"" << SkillTargetNames[(int)trigger.SkillTarget] << "\"";

	bool printAttackMaterial = attack != nullptr && Settings.PrintAttackMaterial;
	bool printNonTargetAttack = attack != nullptr && Settings.PrintNonTargetAttack && trigger.IsSplash;
	bool printApplyTarget = attack != nullptr && Settings.PrintApplyTarget;

	if (style != "" || printTarget || printAttackMaterial || printNonTargetAttack || printApplyTarget)
		OutFile << " [" << style; 

	if (printTarget || printAttackMaterial || printNonTargetAttack || printApplyTarget)
	{
		OutFile << " label=\"";

		if (printTarget)
			OutFile << SkillTargetNames[(int)trigger.SkillTarget] << (printAttackMaterial || printNonTargetAttack || printApplyTarget ? "\\n" : "");

		if (printAttackMaterial)
			OutFile << "AttackMaterial: " << (int)attack->AttackMaterial << (printNonTargetAttack || printApplyTarget ? "\\n" : "");

		if (printApplyTarget)
			OutFile << "ApplyTarget: " << ApplyTargetNames[(int)attack->ApplyTarget] << "\\nCastTarget: " << ApplyTargetNames[(int)attack->CastTarget] << (printNonTargetAttack ? "\\n" : "");

		bool targetingNone = printNonTargetAttack && (((trigger.NonTargetActive || attack->CubeMagicPathId > 0) && attack->MagicPathId == 0) || attack->ApplyTarget == ApplyTarget::None);

		if (printNonTargetAttack)
			OutFile << "No Target: " << targetingNone;

		OutFile << "\" ";
	}

	if (style != "" || printTarget || printAttackMaterial || printNonTargetAttack || printApplyTarget)
		OutFile << "]";

	OutFile << "\n";
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
		OutFile << "\t" << Dereference(jobSkill.Skill) << " -> " << Dereference(jobSkill.SubSkills[j]) << " [dir=none color=\"blue\"]\n";

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

		OutFile << "\t" << Dereference(combo) << " -> " << Dereference(nextCombo) << " [color=\"green\"]\n";

		combo = nextCombo;
		comboSkill = &comboSkillIndex->second;
		comboSkillLevel = &comboSkill->Levels.begin()->second;
	}

	for (int i = 0; i < skillLevel.ChangeSkillReferences.size(); ++i)
	{
		const ChangeSkillReference& changeSkill = skillLevel.ChangeSkillReferences[i];

		if (changeSkill.Skill.Id == 0)
			continue;

		OutFile << "\t" << Dereference(jobSkill.Skill) << " -> " << Dereference(changeSkill.Skill) << " [color=\"red\"]\n";
	}
}

void GraphData::PrintRoot(const JobData& jobData)
{
	OutFile << "\t" << RootName << " [label=\"" << RootLabel << "\"]\n";

	for (int i = 0; i < jobData.Skills.size(); ++i)
		PrintRoot(jobData.Skills[i]);

	if (jobData.Lapenshards.size() == 0)
		return;

	std::unordered_set<int> skillReferences;
	std::unordered_set<int> effectReferences;

	for (int i = 0; i < jobData.Lapenshards.size(); ++i)
	{
		const ItemData& item = *jobData.Lapenshards[i];

		int uniqueReferences = 0;

		for (int j = 0; j < item.AdditionalEffects.size(); ++j)
			if (!effectReferences.contains(item.AdditionalEffects[j].Id))
				++uniqueReferences;

		for (int j = 0; j < item.Skills.size() && j < 1; ++j)
			if (!skillReferences.contains(item.Skills[j].Id))
				++uniqueReferences;

		if (uniqueReferences == 0)
			continue;

		OutFile << "\tLapenshards -> " << "item_" << item.Id << "\n";
		OutFile << "\titem_" << item.Id << " [label=\"Item " << item.Id << "\\n\t" << Sanitize(item.Name) << "\\n\tLapenshard";

		if (item.Description != "")
			OutFile << "\" tooltip=\"" << Sanitize(item.Description, true);

		OutFile <<  "\" shape=house]\n";

		for (int j = 0; j < item.AdditionalEffects.size(); ++j)
		{
			const ReferenceData& ref = item.AdditionalEffects[j];

			if (!effectReferences.contains(ref.Id))
				effectReferences.insert(ref.Id);

			OutFile << "\titem_" << item.Id << " -> " << Dereference(ref) << "\n";
		}

		for (int j = 0; j < item.Skills.size(); ++j)
		{
			const ReferenceData& ref = item.Skills[j];

			if (!skillReferences.contains(ref.Id))
				skillReferences.insert(ref.Id);

			OutFile << "\titem_" << item.Id << " -> " << Dereference(ref) << "\n";
		}
	}
}

void GraphData::PrintLinked()
{

	int skillIndex = 0;
	int effectIndex = 0;

	int rootSkills = (int)QueuedSkills.size();

	while (QueuedSkills.size() > skillIndex || QueuedEffects.size() > effectIndex)
	{
		while (QueuedSkills.size() > skillIndex)
		{
			ReferenceData skillRef = QueuedSkills[skillIndex];
			int skillId = skillRef.Id;

			auto skillContainerIndex = skills.find(skillId);

			if (skillContainerIndex == skills.end())
			{
				++skillIndex;

				continue;
			}

			SkillData& skill = skillContainerIndex->second;

			bool isProjectile = false;

			if (skill.Levels.size() > 0)
			{
				SkillLevelData& skillLevel = skillRef.Level == -1 ? skill.Levels.begin()->second : skill.Levels[skillRef.Level];

				for (const SkillMotion& motion : skillLevel.Motions)
				{
					for (const SkillAttack& attack : motion.Attacks)
					{
						if (attack.MagicPathId == 0)
							continue;

						auto index = magicPaths.find(attack.MagicPathId);

						if (index == magicPaths.end())
							continue;

						for (const MagicPathMove& move : index->second.Moves)
						{
							isProjectile = move.Velocity > 0;

							if (isProjectile) break;
						}

						if (isProjectile) break;
					}

					if (isProjectile) break;
				}
			}

			bool isSensor = !isProjectile && ReferencedSensorSkills.contains(skillId);
			bool isSplash = ReferencedSplashSkills.contains(skillId);
			const char* typeLabel = "";

			if (isProjectile)
				typeLabel = "Projectile ";
			else if (isSensor)
				typeLabel = "Sensor ";
			else if (isSplash)
				typeLabel = "Splash ";

			OutFile << "\t" << Dereference(skillRef) << "[label=\"" << typeLabel << "Skill " << skillId;

			if (skillRef.Level != -1)
				OutFile << " [" << skillRef.Level << "]";

			if (skill.Name != "")
				OutFile << "\\n" << Sanitize(skill.Name);

			if (Settings.PrintTypes)
				OutFile << "\\nType: " << skill.Type << "\\nSubType: " << skill.SubType;

			if (Settings.PrintImmediateActiveSkill)
				OutFile << "\\nImmediateActive: " << skill.ImmediateActive;

			if (skill.Levels.size() > 0)
			{
				SkillLevelData& skillLevel = skillRef.Level == -1 ? skill.Levels.begin()->second : skill.Levels[skillRef.Level];

				if (skillLevel.TotalMotionsWithPaths > 0 && Settings.PrintPaths)
				{
					OutFile << "\\nHas Path";
				}
				if (skillLevel.TotalMotionsWithCubePaths > 0 && Settings.PrintPaths)
				{
					OutFile << "\\nHas Cube Path";
				}

				if (skillLevel.Motions.size() > 1 && Settings.PrintMotions)
					OutFile << "\\n" << skillLevel.Motions.size() << " Motions";

				if (skillLevel.TotalAttacks > 1 && Settings.PrintAttacks)
					OutFile << "\\n" << skillLevel.TotalAttacks << " Attacks";
			}

			if (isSplash)
			{
				if (skill.Levels.size() > 0)
				{
					SkillLevelData& skillLevel = skillRef.Level == -1 ? skill.Levels.begin()->second : skill.Levels[skillRef.Level];

					if (skillLevel.Motions.size() > 1)
					{
						skillLevel.TotalAttacks += 0;
					}

					if (skillLevel.TotalAttacks > 1)
					{
						skillLevel.TotalAttacks += 0;
					}
				}

				if (isProjectile)
					OutFile << "\",shape=hexagon";
				else if (isSensor)
					OutFile << "\",shape=Mcircle";
				else
					OutFile << "\",shape=box3d";

				if (skill.Levels.size() == 0)
					OutFile << ",color=red";
			}
			else
				OutFile << "\",shape=box";

			if (skill.Levels.size() == 0)
			{
				++skillIndex;

				OutFile << "]\n";

				continue;
			}

			SkillLevelData& skillLevel = skillRef.Level == -1 ? skill.Levels.begin()->second : skill.Levels[skillRef.Level];
			
			if (skillLevel.Description != "")
				OutFile << ",tooltip=\"" << Sanitize(skillLevel.Description, true) << "\"";

			OutFile << "]\n";

			for (int i = 0; i < skillLevel.Passives.size(); ++i)
			{
				Print(skillRef, skillLevel.Passives[i], "color=\"purple\"");
			}

			if (skillIndex > rootSkills && skillLevel.TotalPaths > 1)
				skillIndex += 0;

			for (int m = 0; m < skillLevel.Motions.size(); ++m)
			{
				SkillMotion& motion = skillLevel.Motions[m];

				if (skillIndex > rootSkills && motion.TotalPaths > 1)
					skillIndex += 0;

				for (int a = 0; a < motion.Attacks.size(); ++a)
				{
					SkillAttack& attack = motion.Attacks[a];

					int magicPathMoves = 0;
					int magicPathAligned = 0;

					if (magicPaths.contains(attack.MagicPathId))
					{
						magicPathMoves = (int)magicPaths[attack.MagicPathId].Moves.size();
						magicPathAligned = magicPaths[attack.MagicPathId].Aligned;
					}
					else
						skillIndex += 0;

					if (magicPathMoves > 1 && skillIndex > rootSkills)
						skillIndex += 0;

					if (magicPathAligned > 0)
						skillIndex += 0;

					for (int i = 0; i < attack.Triggers.size(); ++i)
					{
						const ConditionSkill& trigger = attack.Triggers[i];

						Print(skillRef, trigger, "", a, i, &attack);

						if (trigger.IsSplash)
						{
							if (trigger.OnlySensingActive && !ReferencedSensorSkills.contains(trigger.Reference.Id))
								ReferencedSensorSkills.insert(trigger.Reference.Id);

							const auto& ref = skills.find(trigger.Reference.Id);

							if (ref != skills.end())
							{
								SkillData& data = ref->second;

								const auto& ref2 = data.Levels.find(trigger.Reference.Level);

								if (ref2 != data.Levels.end())
								{
									SkillLevelData& levelData = ref2->second;

									for (SkillMotion& motionData : levelData.Motions)
									{
										for (SkillAttack& attackData : motionData.Attacks)
										{
											if (attack.CubeMagicPathId != 0 && attackData.MagicPathId != 0)
											{
												int magicPathMoves = 0;
												int cubeMagicPathMoves = 0;
												int cubeMagicPathAligned = 0;

												if (magicPaths.contains(attackData.MagicPathId))
													magicPathMoves = (int)magicPaths[attackData.MagicPathId].Moves.size();
												else
													skillIndex += 0;

												if (magicPaths.contains(attack.CubeMagicPathId))
												{
													cubeMagicPathMoves = (int)magicPaths[attack.CubeMagicPathId].Moves.size();
													cubeMagicPathAligned = magicPaths[attack.CubeMagicPathId].Aligned;
												}
												else
													skillIndex += 0;

												if (magicPathMoves > 0 && cubeMagicPathMoves > 0)
													skillIndex += 0;

												if (magicPathMoves > 1)
													skillIndex += 0;

												if (cubeMagicPathMoves > 1)
													skillIndex += 0;
												
												if (cubeMagicPathAligned == 0)
													skillIndex += 0;

											}
										}
									}
								}
							}
						}
					}
				}
			}

			++skillIndex;
		}

		while (QueuedEffects.size() > effectIndex)
		{
			ReferenceData effectRef = QueuedEffects[effectIndex];
			int effectId = effectRef.Id;

			auto effectContainerIndex = effects.find(effectId);

			if (effectContainerIndex == effects.end())
			{
				++effectIndex;

				continue;
			}

			AdditionalEffectData& effect = effectContainerIndex->second;

			OutFile << "\t" << Dereference(effectRef) << "[label=\"Effect " << effectId;

			if (effectRef.Level != -1)
				OutFile << " [" << effectRef.Level << "]";

			if (effect.Levels.size() == 0)
			{
				OutFile << "\",color=red]\n";

				++effectIndex;

				continue;
			}

			AdditionalEffectLevelData& effectLevel = effectRef.Level == -1 ? effect.Levels.begin()->second : effect.Levels[effectRef.Level];

			if (effectLevel.Group != 0)
				OutFile << "\\nGroup: " << effectLevel.Group;

			if (effectLevel.Name != "")
				OutFile << "\\n" << Sanitize(effectLevel.Name);

			if (Settings.PrintEffectTypes)
				OutFile << "\\nType: " << effectLevel.Type << "\\nSubType: " << effectLevel.SubType;

			if (Settings.PrintReset)
				OutFile << "\\nResetCondition: " << effectLevel.ResetCondition;

			if (Settings.PrintStacks)
				OutFile << "\\nMax Stacks: " << effectLevel.MaxStacks;

			if (Settings.PrintKeepCondition)
				OutFile << "\\nKeepCondition: " << effectLevel.KeepCondition;

			if (effectLevel.KeepCondition == 99)
				OutFile << "\\nPersistent Effect\"shape=egg";
			else
				OutFile << "\",shape=ellipse";
			
			if (effectLevel.Description != "")
				OutFile << ",tooltip=\"" << Sanitize(effectLevel.Description, true) << "\"";

			OutFile << "]\n";

			if (effectLevel.Group != 0)
			{
				if (effects.contains(effectLevel.Group))
					OutFile << "\t" << Dereference(effectRef) << " -> " << Dereference(ReferenceData{ ReferenceType::Effect, effectLevel.Group, -1 }) << "[constraint=false,style=dashed,arrowhead=dot,color=green]\n";
				else
				{
					OutFile << "\t" << Dereference(effectRef) << " -> effectgroup_" << effectLevel.Group << "[style=dashed,arrowhead=dot,color=green]\n";

					if (!ReferencedEffectGroups.contains(effectLevel.Group))
						ReferencedEffectGroups.insert(effectLevel.Group);
				}
			}

			for (int i = 0; i < effectLevel.Triggers.size(); ++i)
			{
				Print(effectRef, effectLevel.Triggers[i], "", i);
			}

			for (int i = 0; i < effectLevel.Modifications.size(); ++i)
			{
				const ModifyReference& mod = effectLevel.Modifications[i];

				if (mod.ModificationType == ModifyReferenceType::ModifyStacks)
				{
					OutFile << "\t" << Dereference(effectRef) << " -> " << Dereference(mod);
					
					if (mod.Offset > 0)
						OutFile << "[style=dashed,arrowhead=olnormal,color=chartreuse4,label=\"+";
					else
						OutFile << "[style=dashed,arrowhead=ornormal,color=darkred,label=\"";

					OutFile << mod.Offset << "\"]\n";
				}
				else if (mod.ModificationType == ModifyReferenceType::ModifyDuration)
				{

				}
				else if (mod.ModificationType == ModifyReferenceType::Cancel)
				{
					OutFile << "\t" << Dereference(effectRef) << " -> " << Dereference(mod) << " [style=dotted,arrowhead=vee,color=red]\n";
				}
				else if (mod.ModificationType == ModifyReferenceType::Immune)
				{
					OutFile << "\t" << Dereference(effectRef) << " -> " << Dereference(mod) << " [style=dotted,arrowhead=tee,color=darkred]\n";
				}
			}

			if (Settings.PrintRequireSkillCodeConnections)
			{
				std::string constraint = effectLevel.Condition.RequireSkillCodes.size() > 5 ? "constraint=false," : "";

				for (int skillId : effectLevel.Condition.RequireSkillCodes)
				{
					OutFile << "\t" << Dereference(ReferenceData{ ReferenceType::Skill, skillId, 1 }) << " -> " << Dereference(effectRef) << " [" << constraint << "style=dashed,arrowhead=vee,color=cyan]\n";
				}
			}

			++effectIndex;
		}
	}

	for (int group : ReferencedEffectGroups)
		OutFile << "\teffectgroup_" << group << "[label=\"Group " << group << "\",shape=octagon]\n";
}

void GraphData::PrintRoot(const SetBonusData& setData)
{
	OutFile << "\t" << RootName << " [label=\"" << RootLabel << "\"]\n";

	for (int i = 0; i < setData.OptionData->Parts.size(); ++i)
	{
		const SetBonusOptionPartData& part = setData.OptionData->Parts[i];

		OutFile << "\tset_" << part.Count << " [label=\"" << part.Count << " Piece Bonus\" shape=invhouse]\n";
		OutFile << "\t" << RootName << " -> set_" << part.Count << "\n";

		for (int j = 0; j < part.AdditionalEffects.size(); ++j)
			OutFile << "\tset_" << part.Count << " -> " << Dereference(part.AdditionalEffects[j]) << "\n";
	}

	for (int i = 0; i < setData.ItemIds.size(); ++i)
	{
		int itemId = setData.ItemIds[i];

		const auto& itemIndex = items.find(itemId);

		if (itemIndex == items.end())
			continue;

		const ItemData& item = itemIndex->second;

		OutFile << "\titem_" << itemId << " [label=\"Item " << itemId << "\\n" << Sanitize(item.Name) << "\\n" << item.Class;

		if (item.Description != "")
			OutFile << "\" tooltip=\"" << Sanitize(item.Description, true);

		OutFile << "\" shape=house]\n";
		OutFile << "\t" << RootName << " -> " << "item_" << itemId << "\n";
		
		for (int j = 0; j < item.AdditionalEffects.size(); ++j)
			OutFile << "\titem_" << itemId << " -> " << Dereference(item.AdditionalEffects[j]) << "\n";
	}
}