#pragma once

#include <ClibUtil/string.hpp>
#include <ClibUtil/editorID.hpp>

namespace JSONUtils
{
    // Helper function for extracting forms from a string
    template <typename T>
    struct form_type
    {
        static constexpr RE::FormType value = RE::FormType::None;
    };

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

    constexpr bool supports_edids_without_tweaks(RE::FormType type)
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
        FormatError,      // String is in an invalid format (EditorID while PO3's Tweaks is not present, FormID not hex, etc)
        FileNotFound,     // ESP/ESM/ESL missing
        FormNotInFile,    // Master exists, but form is not present
        WrongFormtype,    // Form exists in given file, but type is wrong.
        NoForm,           // Form simply not found

        MissingPo3Tweaks, // EditorID query that requires PO3's tweaks but PO3's tweaks is not present.
        GenericFailure    // Catchall (might be missing data handler, cosmic ray, etc)
    };

    template <typename T>
    struct QueryData
    {
        T*          value;
        QueryResult status;
    };


    template <typename T>
    QueryData<T> find_form_from_string(std::string_view a_str) {
        constexpr bool supportsEDID = supports_edids_without_tweaks(form_type<T>::value);

        QueryData<T> response({ nullptr, QueryResult::FormatError });

        auto* dh = RE::TESDataHandler::GetSingleton();
        if (!dh) {
            response.status = QueryResult::GenericFailure;
            return response;
        }

        auto* tweaks = REX::W32::GetModuleHandleW(L"po3_Tweaks.dll");
        auto parts = clib_util::string::split(a_str, "|");
       
        if (parts.size() == 1) {
            if constexpr (!supportsEDID) {
                if (!tweaks) {
                    response.status = QueryResult::MissingPo3Tweaks;
                    return response;
                }
            }
            auto* asForm = RE::TESForm::LookupByEditorID(parts[0]);
            if (!asForm) {
                response.status = QueryResult::NoForm;
                return response;
            }
            T* castForm = asForm->As<T>();
            if (!castForm) {
                response.status = QueryResult::WrongFormtype;
                return response;
            }
            response.status = QueryResult::Success;
            response.value = castForm;
        }
        else if (parts.size() == 2) {
            const auto& first = parts[0];
            const auto& second = parts[1];

            RE::TESForm* asForm = nullptr;
            RE::FormID formID;
            if (clib_util::string::is_only_hex(first)) {
                if (!dh->LookupModByName(second)) {
                    response.status = QueryResult::FileNotFound;
                    return response;
                }
                formID = clib_util::string::to_num<RE::FormID>(first);
                asForm = dh->LookupFormID(formID, second);
            }
            else if (clib_util::string::is_only_hex(second)) {
                if (!dh->LookupModByName(first)) {
                    response.status = QueryResult::FileNotFound;
                    return response;
                }
                formID = clib_util::string::to_num<RE::FormID>(second);
                asForm = dh->LookupFormID(formID, first);
            }
            else {
                return response;
            }

            if (!asForm) {
                response.status = QueryResult::FormNotInFile;
                return response;
            }
            T* castForm = asForm->As<T>();
            if (!castForm) {
                response.status = QueryResult::WrongFormtype;
                return response;
            }

            response.status = QueryResult::Success;
            response.value = castForm;
        }

        return response;
    }

    template <typename T>
    struct MassQueryResult
    {
        QueryResult     status;
        std::vector<T*> values{};
    };

    template <typename T>
    MassQueryResult<T> forms_from_strings(const std::vector<std::string_view>& strings)
    {
        MassQueryResult<T> result;
        result.status = QueryResult::Success;
        if (strings.empty()) {
            return result;
        }
        result.values.reserve(strings.size());

        QueryData<T> subResult;
        for (const auto& str : strings) {
            subResult = find_form_from_string<T>(str);
            if (subResult.value) {
                result.values.emplace_back(subResult.value);
                continue;
            }
            result.status = std::max(subResult.status, result.status);
        }
        return result;
    }
}