#pragma once

#include <filesystem>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <type_traits>

#include "ParserUtils.h"
#include "XmlData.h"
#include "XmlParsing.h"
#include "GraphPrinting.h"

void graphClassKit(const fs::path& outputRoot, JobCode jobCode)
{
	auto jobIndex = jobs.find(jobCode);

	if (jobIndex == jobs.end())
		return;

	JobData& job = jobIndex->second;

	fs::create_directories(outputRoot);

	std::string jobName = job.Name;

	for (int i = 0; i < jobName.size(); ++i)
		if (jobName[i] == ' ')
			jobName[i] = '_';

	fs::path outputPath = outputRoot;
	outputPath += jobName;
	outputPath += ".digraph";

	std::ofstream outFile(outputPath, std::ofstream::out);

	GraphData graphData { outFile, jobName, job.Name };

	outFile << "digraph " << jobName << "_Kit {\n";

	graphData.PrintRoot(job);

	graphData.PrintLinked();

	outFile << "}" << std::endl;

	outFile.close();
}

void graphSetBonus(const fs::path& outputRoot, int setId, const SetBonusData& setData)
{
	fs::create_directories(outputRoot);

	std::string setName = Desanitize(setData.Name);
	std::string setVarName = setName;

	for (int i = 0; i < setVarName.size(); ++i)
		if (setVarName[i] == ' ' || setVarName[i] == '(' || setVarName[i] == ')' || setVarName[i] == '\'')
			setVarName[i] = '_';

	char idString[16] = { 0 };

	_itoa_s(setId, idString, 16, 10);

	fs::path outputPath = outputRoot;
	outputPath += idString;
	outputPath += "_" + setVarName;
	outputPath += ".digraph";

	std::ofstream outFile(outputPath, std::ofstream::out);

	GraphData graphData{ outFile, setVarName, setName };

	outFile << "digraph " << setVarName << "_Kit {\n";

	graphData.PrintRoot(setData);
	graphData.PrintLinked();

	outFile << "}" << std::endl;

	outFile.close();
}

//template <class ParentClass>
//class DerivedFrom : public ParentClass
//{
//public:
//	typedef ParentClass Parent;
//};
//
//template <typename T>
//constexpr bool IsUpdateable()
//{
//	return &T::Update != &T::Parent::Update;
//}
//
//template <>
//constexpr bool IsUpdateable<class Object>()
//{
//	return true;
//}
//
//template <typename T>
//struct ParentOf
//{
//	typedef T::Parent Type;
//};
//
//class Object
//{
//public:
//	template <class Self>
//	void UpdateCore(float delta)
//	{
//		InvokeUpdate<Self>(delta);
//	}
//
//	template <>
//	void UpdateCore<Object>(float delta)
//	{
//		InvokeUpdate<Object>(delta);
//		Object::Update(delta);
//	}
//
//	template <class Self> requires(IsUpdateable<Self>())
//	void InvokeUpdate(float delta)
//	{
//		typedef void (Self::* Callback)(float);
//
//		Callback callback = &Self::Update;
//
//	    (this->*callback)(delta);
//	}
//
//	template <class Self> requires(!IsUpdateable<Self>())
//	void InvokeUpdate(float delta) {}
//
//	void Update(float delta)
//	{
//		// do thing
//	}
//};
//class Transform : public DerivedFrom<Object>
//{
//public:
//	void Update(float delta) {}
//};

int main(int argc, char** argv)
{
	//Object a;
	//Transform b;
	//
	//b.UpdateCore<Transform>(0);


	fs::path xmlRootPath = "B:/Documents/MapleServer2/GameDataParser/Resources/Xml/";
	fs::path outputRootPath = "B:/Documents/Ms2DependencyGraph/output/";

	fs::path tableRootPath = xmlRootPath;
	tableRootPath += "table/";

	if (!loadFeatures(tableRootPath, "NA", "Live"))
		return -1;

	fs::path effectRootPath = xmlRootPath;
	effectRootPath += "additionaleffect/";

	fs::path skillRootPath = xmlRootPath;
	skillRootPath += "skill/";

	fs::path stringRootPath = xmlRootPath;
	stringRootPath += "string/en/";

	fs::path magicPath = xmlRootPath;
	magicPath += "table/magicpath.xml";

	fs::path jobPath = xmlRootPath;
	jobPath += "table/job.xml";

	fs::path jobNamePath = xmlRootPath;
	jobNamePath += "string/en/jobname.xml";

	fs::path classKitPath = outputRootPath;
	classKitPath += "classKits/";

	fs::path setItemInfoPath = xmlRootPath;
	setItemInfoPath += "table/setiteminfo.xml";

	fs::path setItemOptionPath = xmlRootPath;
	setItemOptionPath += "table/setitemoption.xml";

	fs::path setItemNamePath = xmlRootPath;
	setItemNamePath += "string/en/setitemname.xml";

	fs::path setBonusPath = outputRootPath;
	setBonusPath += "setBonuses/";

	fs::path itemRootPath = xmlRootPath;
	itemRootPath += "item/";

	fs::path itemStringPath = xmlRootPath;
	itemStringPath += "string/en/itemname.xml";

	fs::path itemDescPath = xmlRootPath;
	itemDescPath += "string/en/koritemdescription.xml";

	ParseMagicPaths(magicPath);
	forEachFile(effectRootPath, true, &ParseAdditionalEffect);
	forEachFile(skillRootPath, true, &ParseSkill);
	forEachFile(stringRootPath, true, &ParseStrings);
	ParseJobs(jobPath);
	forEachFile(itemRootPath, true, &ParseItems);
	ParseJobStrings(jobNamePath);
	ParseItemStrings(itemStringPath);
	ParseItemDescriptionStrings(itemDescPath);
	ParseSetBonusOptions(setItemOptionPath);
	ParseSetBonuses(setItemInfoPath);
	ParseSetBonusStrings(setItemNamePath);

	JobCode jobs[] = {
		JobCode::Beginner,
		JobCode::Knight,
		JobCode::Berserker,
		JobCode::Wizard,
		JobCode::Priest,
		JobCode::Archer,
		JobCode::HeavyGunner,
		JobCode::Thief,
		JobCode::Assassin,
		JobCode::Runeblade,
		JobCode::Striker,
		JobCode::SoulBinder,
		JobCode::GameMaster
	};

	for (int i = 0; i < sizeof(jobs) / sizeof(JobCode); ++i)
		graphClassKit(classKitPath, jobs[i]);

	for (const std::pair<int, SetBonusData>& setBonus : setBonuses)
		graphSetBonus(setBonusPath, setBonus.first, setBonus.second);
}