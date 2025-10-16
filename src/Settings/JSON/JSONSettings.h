#pragma once

#include <json/json.h>

namespace Settings
{
	namespace JSON
	{
		bool Read();

		class Reader : public REX::Singleton<Reader>
		{
		private:
			void RemoveDuplicates(Json::Value& arr);

			void CommitMerge(Json::Value& dest, const Json::Value& src);

			bool ValidateMerge(const Json::Value& current, const Json::Value& incoming);

			bool BuildMergedObject(Json::Value& result, const Json::Value& source);

			bool ReadConfig(const Json::Value& a_entry);

			bool                reloaded{ false };
			Json::Value         settings{};

			inline static constexpr std::uint8_t PARSER_VERSION = 1;
			inline static constexpr const char* MINIMUM_VERSION_FIELD = "MinimumVersion";
		public:
			bool Reload();
			bool Read();
			bool Clear();
			
			void DumpMergedConfig(const std::string& outPath) const;
		};
	}
}