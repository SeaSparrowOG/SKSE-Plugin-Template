#pragma once

#include "ClibUtil/string.hpp"

namespace Settings
{
	namespace INI
	{
		[[nodiscard]] bool Read();

		template <typename T>
		concept is_bool =
			std::is_same_v<T, bool>;

		template <typename T>
		concept is_numerical = 
			(std::is_integral_v<T> || std::is_floating_point_v<T>) &&
			!is_bool<T>;
		
		template <typename T>
		concept is_text =
			std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>;

		template <typename T>
		concept is_setting =
			is_bool<T> || is_text<T> || is_numerical<T>;

		class Holder :
			public REX::TSingleton<Holder>
		{
		public:
			void ReadSettings();
			void OverrideSettings();
			void DumpSettings();

			template <typename T> requires is_setting<T>
			T GetSetting(const std::string& setting, 
				const std::string& section, 
				const T defaultValue) const
			{
				const auto composite = std::format("{}|{}", section, setting);
				const auto where = _settings.find(composite);
				if (where == _settings.end()) {
					return defaultValue;
				}

				if constexpr (is_bool<T>) {
					inline static constexpr const std::array<std::string_view> truthy = {
						"1", "true"
					};
					inline static constexpr const std::array<std::string_view> falsy = {
						"0", "false"
					};

					const auto lower = clib_util::string::tolower(where->second);
					if (std::ranges::contains(truthy, lower)) {
						return true;
					}
					else if (std::ranges::contains(falsy, lower)) {
						return false;
					}
					return defaultValue;
				}
				else if constexpr (is_numerical<T>) {
					T value{};
					std::string str = where->second;
					const auto [ptr, ec] = std::from_chars(
						str.data(), str.data() + str.size(), value
					);

					if (ec == std::errc::invalid_argument || 
						ec == std::errc::result_out_of_range ||
						ptr != str.data() + str.size()) 
					{
						return defaultValue;
					}
					return value;
				}
				else if constexpr (is_text<T>) {
					return where->second;
				}
			}

		private:
			std::map<std::string, std::string> _settings;
		};

		template <typename T> requires is_setting<T>
		inline const T GetSetting(const std::string& setting, 
			const std::string& section, 
			const T defaultValue) 
		{
			static const auto* holder = Holder::GetSingleton();
			return holder->GetSetting<T>(setting, section, defaultValue);
		}
	}
}