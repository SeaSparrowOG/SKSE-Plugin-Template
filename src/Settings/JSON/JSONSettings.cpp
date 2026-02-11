#include "JSONSettings.h"

namespace Settings::JSON
{
	std::string QueryResultToString(QueryResult a_flag) {
		switch (a_flag) {
		case QueryResult::FileNotFound: return "FileNotFound";
		case QueryResult::FormatError: return "FormatError";
		case QueryResult::FormNotInFile: return "FormNotInFile";
		case QueryResult::GenericFailure: return "GenericFailure";
		case QueryResult::MissingPo3Tweaks: return "MissingPo3Tweaks";
		case QueryResult::WrongFormtype: return "WrongFormtype";
		default: return "Success";
		}
	}

	enum class JsonParseResult
	{
		Success,
		NotStringOrArray,
		NonHomogenousArray
	};
	
	std::string JsonParseResultToString(JsonParseResult a_flag) {
		switch (a_flag) {
		case JsonParseResult::NotStringOrArray: return "NotStringOrArray";
		case JsonParseResult::NonHomogenousArray: return "NonHomogenousArray";
		default: return "Success";
		}
	}

	JsonParseResult LoadFormStrings(const Json::Value& a_value, std::vector<std::string>& a_result)
	{
		if (a_value.isString()) {
			a_result.push_back(a_value.asString());
			return JsonParseResult::Success;
		}
		else if (a_value.isArray()) {
			const auto size = a_value.size();
			a_result.reserve(size);

			for (const auto& value : a_value) {
				if (!value.isString()) {
					a_result.clear();
					return JsonParseResult::NonHomogenousArray;
				}
				a_result.push_back(value.asString());
			}
		}
		return JsonParseResult::NotStringOrArray;
	}

	bool Read() {
		return true;
	}
}