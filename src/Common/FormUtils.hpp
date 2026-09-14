#pragma once

#include <ClibUtil/string.hpp>
#include <ClibUtil/editorID.hpp>

namespace FormUtils
{
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

    enum class ToNumError
    {
        StringEmpty,       // Example: to_num<size_t>("") || to_num<size_t>("0x")

        NotANumber,        // Provided string is not a number.
        InvalidNumber,     // Number is out of bounds for the value.
        FloatFromHex,      // Asked to convert a hex to a float.

        IllegalConversion  // Catch-all for illegal conversions. Likely impossible to reach.
    };

    enum class QueryError
    {
        MissingDataHandler, // Unlikely error - the DataHandler is initialized before SKSE plugins can be.
        PO3TweaksMissing,   // Queries via EDID are supported, but some forms are not cached without PO3 tweaks.
        InvalidFormID,      // Simple typo on the query. Banana crash, essentially.
        FormWrongType,      // Form exists, but is not of the specified type. Almost always a hard error.

        StringEmpty         // Technically an error, very unlikely.
    };

    std::string_view to_string(ToNumError flag)
    {
        switch (flag) {
        case ToNumError::StringEmpty: return "StringEmpty";
        case ToNumError::NotANumber: return "NotANumber";
        case ToNumError::InvalidNumber: return "InvalidNumber";
        case ToNumError::FloatFromHex: return "FloatFromHex";
        case ToNumError::IllegalConversion: return "IllegalConversion";
        default: return "Unknown";
        }
    }

    std::string_view to_string(QueryError flag) {
        switch (flag) {
        case QueryError::MissingDataHandler: return "MissingDataHandler";
        case QueryError::PO3TweaksMissing: return "PO3TweaksMissing";
        case QueryError::InvalidFormID: return "InvalidFormID";
        case QueryError::FormWrongType: return "FormWrongType";
        case QueryError::StringEmpty: return "StringEmpty";
        default: return "Unknown";
        }
    }

    template <typename T>
    concept Integral =
        std::is_integral_v<T>;

    template <typename T>
    concept FloatingPoint =
        std::is_floating_point_v<T>;

    template <typename T>
    concept NumericallyRepresentable =
        Integral<T> || FloatingPoint<T>;
    
    template <typename T>
    requires NumericallyRepresentable<T>
    inline std::expected<T, ToNumError> to_num(const std::string& str)
    {
        if (str.empty()) {
            return std::unexpected(ToNumError::StringEmpty);
        }

        const auto firstTwo = str.substr(0, 2);
        const bool hasHexPrefix = firstTwo == "0x" || firstTwo == "0X";

        auto size = str.size();
        if (hasHexPrefix) {
            if constexpr (std::is_floating_point<T>) {
                return std::unexpected(ToNumError::FloatFromHex);
            }

            size -= 2u;
            if (size == 0u) {
                return std::unexpected(ToNumError::StringEmpty);
            }
        }

        T val{};
        const auto begin = hasHexPrefix ? str.data() + 2 : str.data();
        const auto end = begin + size;
        std::from_chars_result res;

        if constexpr (std::is_integral_v<T>) {
            res = std::from_chars(
                begin,
                end,
                val,
                hasHexPrefix ? 16 : 10);
        }
        else {
            if (hasHexPrefix)
                return std::unexpected(ToNumError::FloatFromHex);

            res = std::from_chars(
                begin,
                end,
                val,
                std::chars_format::general);
        }

        if (res.ec == std::errc::result_out_of_range) {
            return std::unexpected(ToNumError::InvalidNumber);
        }
        else if (res.ec == std::errc::invalid_argument) {
            return std::unexpected(ToNumError::NotANumber);
        }
        else if (res.ptr != begin + size) {
            return std::unexpected(ToNumError::NotANumber);
        }
        return val;
    }

    template <class T>
    std::expected<T*, QueryError> get_form_by_edid(std::string_view edid)
    {
        auto* form = RE::TESForm::LookupByEditorID(edid);
        if (!form) {
            return nullptr;
        }

        if (form->GetFormType() != form_type<T>::value) {
            return std::unexpected(QueryError::FormWrongType);
        }

        return form->As<T>();
    }

    template <class T>
    std::expected<T*, QueryError> get_form_from_string(const std::string& str)
    {
        constexpr bool supportsEDID = supports_edids_without_tweaks(form_type<T>::value);
        if (str.empty()) {
            return std::unexpected(QueryError::StringEmpty);
        }

        auto* dh = RE::TESDataHandler::GetSingleton();
        if (!dh) [[unlikely]] {
            return std::unexpected(QueryError::MissingDataHandler);
        }

        auto end = str.end();
        auto delimiterPos = str.begin();
        for (; delimiterPos != end; ++delimiterPos) {
            if (*delimiterPos == '|') {
                break;
            }
        }

        if (delimiterPos == end) {
            if constexpr (supportsEDID) {
                return get_form_by_edid<T>(str);
            }

            static auto tweaks = REX::W32::GetModuleHandleW(L"po3_Tweaks.dll");
            if (!tweaks) {
                return std::unexpected(QueryError::PO3TweaksMissing);
            }
            return get_form_by_edid<T>(str);
        }

        bool modIspreceding = true;
        const std::string_view preceding = str.substr(0, delimiterPos);
        const std::string_view following = str.substr(delimiterPos + 1);

        auto rawID = to_num<RE::FormID>(following);
        if (raw_id.error()) {
            rawID = to_num<RE::FormID>(preceding);
            if (rawID.error()) {
                return std::unexpected(QueryError::InvalidFormID);
            }
            modIspreceding = false;
        }

        if (!dh->LookupModByName(modIspreceding ? preceding : following)) {
            return nullptr;
        }

        auto id = rawID.value();
        auto* form = dh->LookupForm(id, modIspreceding ? preceding : following);
        if (!form) {
            return nullptr; // form not in file *may* be an error, but mods get updated.
        }
        if (form->GetFormType() != form_type<T>::value) {
            return std::unexpected(QueryError::FormWrongType);
        }
        return form->As<T>();
    }
}