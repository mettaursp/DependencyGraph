#pragma once

#include <filesystem>
#include <iostream>
#include <fstream>
#include <unordered_set>

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

	for (int i = 0; i < job.Skills.size(); ++i)
		graphData.PrintRoot(job.Skills[i]);

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
		if (setVarName[i] == ' ' || setVarName[i] == '(' || setVarName[i] == ')')
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

int main(int argc, char** argv)
{
	fs::path xmlRootPath = "B:/Documents/MapleServer2/GameDataParser/Resources/Xml/";

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

	fs::path jobPath = xmlRootPath;
	jobPath += "table/job.xml";

	fs::path jobNamePath = xmlRootPath;
	jobNamePath += "string/en/jobname.xml";

	fs::path outputRootPath = "B:/Documents/Ms2DependencyGraph/output/";

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

	forEachFile(effectRootPath, true, &ParseAdditionalEffect);
	forEachFile(skillRootPath, true, &ParseSkill);
	forEachFile(stringRootPath, true, &ParseStrings);
	ParseJobs(jobPath);
	ParseJobStrings(jobNamePath);
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