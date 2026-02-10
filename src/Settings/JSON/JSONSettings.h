#pragma once

#include <json/json.h>

namespace Settings
{
	namespace JSON
	{
		bool Read();

		enum class Error
		{
			None, // All good.

			NotStringOrArray,  // Provided Json::Value is neither a string nor an array.
			NonHomogenousArray // Provided Json::Value is an array, but its elements are of different types.
		};

		/// <summary>
		/// Extracts all individual strings from a given Json::Value into a given string vector.
		/// </summary>
		/// <param name="a_value">The Json::Value to retrieve the strings from. If it isn't a String object OR a homogeneous Array of Strings object, returns an empty vector + an error.</param>
		/// <param name="a_result">The vector to place the results in. Note that the vector is modified.</param>
		/// <returns>An error code. None means success.</returns>
		Error LoadFormStrings(const Json::Value& a_value,
			std::vector<std::string>& a_result);

		inline static constexpr int PLUGIN_INDEX = 0;
		inline static constexpr int FORMID_INDEX = 1;
		static_assert(PLUGIN_INDEX >= 0, "PLUGIN_INDEX must be greater than or equal to 0.");
		static_assert(FORMID_INDEX >= 0, "FORMID_INDEX must be greater than or equal to 0.");
		static_assert(FORMID_INDEX != PLUGIN_INDEX, "PLUGIN_INDEX must be different from FORMID_INDEX.");

		// Used for form validation. Certain form types do NOT need to be retrieved through PO3's tweaks.
		template <typename T>
		struct form_type { static constexpr RE::FormType value = RE::FormType::None; };

		template <>
		struct form_type<RE::BGSKeyword> { static constexpr RE::FormType value = RE::FormType::Keyword; };
		template <>
		struct form_type<RE::BGSLocationRefType> { static constexpr RE::FormType value = RE::FormType::LocationRefType; };
		template<>
		struct form_type<RE::TESGlobal> { static constexpr RE::FormType value = RE::FormType::Global; };
		template<>
		struct form_type<RE::TESRace> { static constexpr RE::FormType value = RE::FormType::Race; };
		template<>
		struct form_type<RE::TESSound> { static constexpr RE::FormType value = RE::FormType::Sound; };
		template<>
		struct form_type<RE::TESObjectCELL> { static constexpr RE::FormType value = RE::FormType::Cell; };
		template<>
		struct form_type<RE::TESWorldSpace> { static constexpr RE::FormType value = RE::FormType::WorldSpace; };
		template<>
		struct form_type<RE::TESQuest> { static constexpr RE::FormType value = RE::FormType::Quest; };
		template<>
		struct form_type<RE::TESIdleForm> { static constexpr RE::FormType value = RE::FormType::Idle; };
		template<>
		struct form_type<RE::TESObjectANIO> { static constexpr RE::FormType value = RE::FormType::AnimatedObject; };
		template<>
		struct form_type<RE::TESImageSpaceModifier> { static constexpr RE::FormType value = RE::FormType::ImageAdapter; };
		template<>
		struct form_type<RE::BGSVoiceType> { static constexpr RE::FormType value = RE::FormType::VoiceType; };
		template<>
		struct form_type<RE::BGSMusicType> { static constexpr RE::FormType value = RE::FormType::MusicType; };
		template<>
		struct form_type<RE::BGSSoundDescriptorForm> { static constexpr RE::FormType value = RE::FormType::SoundRecord; };

		constexpr bool SupportsEDIDWithoutTweaks(RE::FormType type)
		{
			switch (type) {
			case RE::FormType::Keyword:
			case RE::FormType::LocationRefType:
			case RE::FormType::Global:
			case RE::FormType::Race:
			case RE::FormType::Sound:
			case RE::FormType::Cell:
			case RE::FormType::WorldSpace:
			case RE::FormType::Quest:
			case RE::FormType::Idle:
			case RE::FormType::AnimatedObject:
			case RE::FormType::ImageAdapter:
			case RE::FormType::VoiceType:
			case RE::FormType::MusicType:
			case RE::FormType::SoundRecord:
				return true;
			default:
				return false;
			}
		}

		enum class QueryResult
		{
			Success,          // Success
			FormatError,      //String is in an invalid format (EditorID while PO3's Tweaks is not present, FormID not hex, etc)
			FileNotFound,     // ESP/ESM/ESL missing
			FormNotInFile,    // Master exists, but form is not present
			WrongFormtype,    // Form exists in given file, but type is wrong.

			MissingPo3Tweaks, // EditorID query that requires PO3's tweaks but PO3's tweaks is not present.
			GenericFailure    // Catchall (might be missing data handler, cosmic ray, etc)
		};

		template <typename T>
		struct QueryData
		{
			std::optional<T*> value{ std::nullopt };
			QueryResult status{ QueryResult::Success };
		};


		template <typename T>
		QueryData<T> GetFormFromString(const std::string& a_str) {
			constexpr bool supportsEDID = SupportsEDIDWithoutTweaks(form_type<T>::value);

			auto response = QueryData<T>{};

			static auto* dh = RE::TESDataHandler::GetSingleton();
			if (!dh) {
				response.status = QueryResult::GenericFailure;
				return response;
			}

			static auto* tweaks = REX::W32::GetModuleHandleW(L"po3_Tweaks.dll");

			auto parts = clib_util::string::split(a_str, "|");
			RE::TESFile* mod = nullptr;
			RE::FormID formID = 0;
			RE::TESForm* form = nullptr;
			T* castForm = nullptr;

			switch (parts.size()) {
			case 1:
				// EDID
				if constexpr (!supportsEDID) {
					if (!tweaks) {
						response.status = QueryResult::MissingPo3Tweaks;
						return response;
					}
				}
				form = RE::TESForm::LookupByEditorID<RE::TESForm>(a_str);
				if (!form) {
					response.value = nullptr;
					return response;
				}
				castForm = form->As<T>();
				if (!castForm) {
					response.status = QueryResult::WrongFormtype;
					return response;
				}
				response.value = castForm;
				return response;
			case 2:
				// FormID
				if (!clib_util::string::is_only_hex(parts[FORMID_INDEX], false)) {
					response.status = QueryResult::FormatError;
					return response;
				}
				formID = clib_util::string::to_num<RE::FormID>(parts[FORMID_INDEX], true);

				mod = dh->LookupModByName(parts[PLUGIN_INDEX]);
				if (!mod) {
					response.status = QueryResult::FileNotFound;
					return response;
				}
				form = dh->LookupForm<RE::TESForm>(formID, mod);
				if (!form) {
					response.status = QueryResult::FormNotInFile;
					return response;
				}
				castForm = form->As<T>();
				if (!castForm) {
					response.status = QueryResult::WrongFormtype;
					return response;
				}
				response.value = castForm;
				return response;
			default:
				response.status = QueryResult::FormatError;
				break;
			}
			return response;
		}
	}
}