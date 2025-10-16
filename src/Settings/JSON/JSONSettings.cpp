#include "JSONSettings.h"

namespace Settings::JSON
{
	static const char* TypeName(Json::ValueType t)
	{
		switch (t) {
		case Json::nullValue: return "null";
		case Json::intValue: return "int";
		case Json::uintValue: return "uint";
		case Json::realValue: return "real";
		case Json::stringValue: return "string";
		case Json::booleanValue: return "bool";
		case Json::arrayValue: return "array";
		case Json::objectValue: return "object";
		default: return "unknown";
		}
	}

	void Reader::RemoveDuplicates(Json::Value& arr)
	{
		if (!arr.isArray())
			return;

		if (arr.empty() || arr.begin()->type() == Json::objectValue) {
			return;
		}

		std::set<std::string> seen;
		Json::Value unique(Json::arrayValue);
		for (const auto& el : arr) {
			auto s = Json::writeString(Json::StreamWriterBuilder(), el);
			if (seen.insert(s).second)
				unique.append(el);
		}
		arr.swap(unique);
	}

	void Reader::CommitMerge(Json::Value& dest, const Json::Value& src)
	{
		for (const auto& key : src.getMemberNames()) {
			const auto& newVal = src[key];
			auto& existing = dest[key];

			if (existing.isNull()) {
				existing = newVal;
				continue;
			}

			if (existing.isArray() && newVal.isArray()) {
				for (const auto& el : newVal)
					existing.append(el);
				RemoveDuplicates(existing);
			}
			else if (existing.isArray()) {
				existing.append(newVal);
				RemoveDuplicates(existing);
			}
			else if (newVal.isArray()) {
				Json::Value arr(Json::arrayValue);
				arr.append(existing);
				for (const auto& el : newVal)
					arr.append(el);
				RemoveDuplicates(arr);
				existing = std::move(arr);
			}
			else if (existing.isObject() && newVal.isObject()) {
				Json::Value arr(Json::arrayValue);
				arr.append(existing);
				for (const auto& el : newVal)
					arr.append(el);
				existing = std::move(arr);
			}
			else {
				Json::Value arr(Json::arrayValue);
				arr.append(existing);
				arr.append(newVal);
				RemoveDuplicates(arr);
				existing = std::move(arr);
			}
		}
	}

	bool Reader::ValidateMerge(const Json::Value& current, const Json::Value& incoming)
	{
		for (const auto& key : incoming.getMemberNames()) {
			const auto& newVal = incoming[key];
			const auto& oldVal = current[key];

			if (oldVal.isNull())
				continue; // new field, fine

			Json::ValueType aType = oldVal.type();
			Json::ValueType bType = newVal.type();

			if (aType == Json::arrayValue && !oldVal.empty())
				aType = oldVal[0].type();
			if (bType == Json::arrayValue && !newVal.empty())
				bType = newVal[0].type();

			if (aType != bType) {
				logger::warn("      >Field '{}' type mismatch across configs ({} vs {})."sv,
					key, TypeName(aType), TypeName(bType));
				return false;
			}
		}
		return true;
	}

	bool Reader::BuildMergedObject(Json::Value& result, const Json::Value& source)
	{
		for (const auto& key : source.getMemberNames()) {
			const auto& member = source[key];
			if (member.isNull())
				continue;

			if (!result.isMember(key)) {
				if (member.isObject()) {
					Json::Value arr(Json::arrayValue);
					arr.append(member);
					result[key] = std::move(arr);
					continue;
				}
				result[key] = member;
				continue;
			}

			auto& existing = result[key];
			auto existingType = existing.type();
			auto memberType = member.type();

			if (existingType == Json::arrayValue && !existing.empty())
				existingType = existing[0].type();

			if (existingType != memberType &&
				!(existing.isArray() && member.isArray())) {
				logger::warn("      >Member '{}' type mismatch in same config."sv, key);
				return false;
			}
			if (existingType == Json::objectValue && memberType == Json::objectValue) {
				if (existing.isArray()) {
					existing.append(member);
				}
				else {
					Json::Value arr(Json::arrayValue);
					arr.append(existing);
					arr.append(member);
					existing = std::move(arr);
				}
				RemoveDuplicates(existing);
				continue;
			}

			if (existing.isArray() && member.isArray()) {
				for (const auto& el : member)
					existing.append(el);
				RemoveDuplicates(existing);
				continue;
			}

			if (existingType == memberType) {
				Json::Value arr(Json::arrayValue);
				arr.append(existing);
				arr.append(member);
				RemoveDuplicates(arr);
				existing = std::move(arr);
			}
		}
		return true;
	}

	bool Read() {
		logger::info("Reading JSON settings..."sv);
		auto* reader = Reader::GetSingleton();
		if (!reader) {
			logger::critical("  >Failed to fetch the reader singleton."sv);
			return false;
		}
		return reader->Read();
	}

	bool Reader::Read()
	{
		std::string jsonFolder = fmt::format(R"(.\Data\SKSE\Plugins\{})"sv, Plugin::NAME);
		logger::info("  >Settings folder: {}."sv, jsonFolder);
		if (!std::filesystem::exists(jsonFolder)) {
			logger::info("    >No settings folder found."sv);
			return true;
		}

		std::vector<std::string> paths{};
		try {
			for (const auto& entry : std::filesystem::directory_iterator(jsonFolder)) {
				if (entry.is_regular_file() && entry.path().extension() == ".json") {
					paths.push_back(entry.path().string());
				}
			}

			std::sort(paths.begin(), paths.end());
			logger::info("    >Found {} configuration files."sv, std::to_string(paths.size()));
		}
		catch (const std::exception& e) {
			logger::warn("Caught {} while reading files."sv, e.what());
			return false;
		}

		if (paths.empty()) {
			logger::info("    >No settings found"sv);
			return true;
		}

		for (const auto& path : paths) {
			auto configName = path.substr(jsonFolder.size() + 1, path.size() - 1);
			logger::info("    >Reading config {}..."sv, configName);
			Json::CharReaderBuilder builder;
			builder["collectComments"] = false;

			try {
				std::ifstream rawJSON(path);
				std::string errs;
				Json::Value JSONFile;
				if (!Json::parseFromStream(builder, rawJSON, &JSONFile, &errs)) {
					logger::warn("      >Failed to parse {}: {}", path, errs);
					continue;
				}

				if (!ReadConfig(JSONFile)) {
					logger::warn("      >Config treated as invalid, skipping."sv);
					continue;
				}
			}
			catch (const Json::Exception& e) {
				logger::warn("Caught {} while reading files.", e.what());
				continue;
			}
			catch (const std::exception& e) {
				logger::error("Caught unhandled exception {} while reading files.", e.what());
				continue;
			}
		}

		logger::info("Finished reading all settings."sv);

#ifndef NDEBUG
		LOG_DEBUG("Dumping data..."sv);
		DumpMergedConfig(fmt::format("{}\\merged.json", jsonFolder));
#endif

		Clear();
		return true;
	}

	bool Reader::ReadConfig(const Json::Value& a_json) {
		if (!a_json.isObject()) {
			logger::warn("      >Config's top level is NOT an object, this isn't supported."sv);
			return false;
		}

		const auto& minVersionField = a_json[MINIMUM_VERSION_FIELD];
		if (minVersionField) {
			if (!minVersionField.isInt()) {
				logger::warn("      >Config has {} specified, but it is not an integer."sv, MINIMUM_VERSION_FIELD);
				return false;
			}
			auto requiredVer = minVersionField.asInt();
			if (requiredVer < 1 || requiredVer > std::numeric_limits<uint8_t>::max()) {
				logger::warn("      >Config has {} specified, but its required version is either too large or too small."sv, MINIMUM_VERSION_FIELD);
				return false;
			}

			auto sanitizedVer = static_cast<uint8_t>(requiredVer);
			if (PARSER_VERSION < sanitizedVer) {
				logger::warn("      >Config requires parser version {}, but the current parser is version {}."sv, sanitizedVer, PARSER_VERSION);
				return false;
			}
		}

		// Copy is intentional.
		Json::Value staged = settings;
		Json::Value result = Json::Value();

		if (!BuildMergedObject(result, a_json)) {
			logger::warn("      >Failed to build merged object for config."sv);
			return false;
		}

		if (!ValidateMerge(staged, result)) {
			logger::warn("      >Config fields are not homogeneous with previous declarations."sv);
			return false;
		}

		CommitMerge(staged, result);
		settings = std::move(staged);

		return true;
	}

	bool Reader::Clear()
	{
		settings.clear();
		reloaded = false;
		return true;
	}

	void Reader::DumpMergedConfig(const std::string& outPath) const {
		std::ofstream out(outPath);
		Json::StreamWriterBuilder builder;
		builder["indentation"] = "  ";
		std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
		writer->write(settings, &out);
	}

	bool Reader::Reload()
	{
		if (reloaded) {
			return false;
		}
		reloaded = true;
		return Read();
	}
}