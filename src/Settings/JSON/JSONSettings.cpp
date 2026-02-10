#include "JSONSettings.h"

namespace Settings::JSON
{
	bool Read() {
		return true;
	}

	Error LoadFormStrings(const Json::Value& a_value, std::vector<std::string>& a_result)
	{
		if (a_value.isString()) {
			a_result.push_back(a_value.asString());
			return Error::None;
		}
		else if (a_value.isArray()) {
			const auto size = a_value.size();
			a_result.reserve(size);

			for (const auto& value : a_value) {
				if (!value.isString()) {
					a_result.clear();
					return Error::NonHomogenousArray;
				}
				a_result.push_back(value.asString());
			}
		}
		return Error::NotStringOrArray;
	}
}