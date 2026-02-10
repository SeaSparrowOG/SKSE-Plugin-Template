#include "JSONSettings.h"

namespace Settings::JSON
{
	bool Read() {
		return true;
	}

	Error LoadFormStrings(const Json::Value& a_value, std::vector<std::string>& a_result)
	{
		(void)a_value;
		(void)a_result;
		return Error::None;
	}
}