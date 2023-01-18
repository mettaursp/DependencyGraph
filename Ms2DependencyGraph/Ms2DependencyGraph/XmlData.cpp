#include "XmlData.h"

std::ostream& operator<<(std::ostream& out, const ReferenceData& reference)
{
	if (reference.Type == ReferenceType::Skill)
		out << "skill_";
	else
		out << "effect_";

	out << reference.Id;

	if (reference.Level != -1)
		out << "_" << reference.Level;

	return out;
}