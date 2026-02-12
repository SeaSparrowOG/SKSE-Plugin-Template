#include "papyrus.h"

namespace Papyrus
{
#ifndef NDEBUG
	RE::BSFixedString UnitTest_Serialization(STATIC_ARGS, RE::BSFixedString a_form) {
		auto* tweaks = REX::W32::GetModuleHandleW(L"po3_tweaks.dll");
		if (!tweaks) {
			return "Couldn't find PO3's Tweaks.";
		}

		auto* utManager = UnitTest_Serialization::GetSingleton();
		if (!utManager) {
			return "Internal Error";
		}

		auto* form = RE::TESForm::LookupByEditorID(a_form);
		if (!form) {
			return RE::BSFixedString(fmt::format("No form found for {}.", a_form.c_str()));
		}
		auto* converted = form->As<RE::TESObjectWEAP>();
		if (!converted) {
			return RE::BSFixedString(fmt::format("{} needs to be a weapon.", a_form.c_str()));
		}

		if (!utManager->SetWeaponForm(converted)) {
			return "Failed to set weapon.";
		}
		return "Success.";
	}

	RE::BSFixedString UnitTest_SerializationReset(STATIC_ARGS){
		auto* tweaks = REX::W32::GetModuleHandleW(L"po3_tweaks.dll");
		if (!tweaks) {
			return "Couldn't find PO3's Tweaks.";
		}
		auto* utManager = UnitTest_Serialization::GetSingleton();
		if (!utManager) {
			return "Internal Error";
		}
		if (!utManager->ResetWeaponForm()) {
			return "Manager refused to reset weapon.";
		}
		return "Success.";
	}

	RE::BSFixedString UnitTest_SerializationState(STATIC_ARGS) {
		auto* tweaks = REX::W32::GetModuleHandleW(L"po3_tweaks.dll");
		if (!tweaks) {
			return "Couldn't find PO3's Tweaks.";
		}
		auto* utManager = UnitTest_Serialization::GetSingleton();
		if (!utManager) {
			return "Internal Error";
		}

		auto* current = utManager->GetCurrentWeap();
		if (!current) {
			return "No current weapon.";
		}
		return clib_util::editorID::get_editorID(current);
	}
#endif

	std::vector<int> GetVersion(STATIC_ARGS) {
		return { Plugin::VERSION[0], Plugin::VERSION[1], Plugin::VERSION[2] };
	}

	void Bind(VM& a_vm) {
		logger::info("  >Binding GetVersion..."sv);
		BIND(GetVersion);

#ifndef NDEBUG
		logger::info("---- Debug Functions ----"sv);
		auto* singleton = UnitTest_Serialization::GetSingleton();
		if (!singleton) {
			SKSE::stl::report_and_fail("{}: Unit test failed - couldn't get internal Serialization singleton."sv);
		}
		singleton->Register('UTSM');
		logger::info("  >Binding UnitTest_Serialization..."sv);
		BIND(UnitTest_Serialization);
		logger::info("  >Binding UnitTest_SerializationState..."sv);
		BIND(UnitTest_SerializationState);
		logger::info("  >Binding UnitTest_SerializationReset..."sv);
		BIND(UnitTest_SerializationReset);
#endif
	}

	bool RegisterFunctions(VM* a_vm) {
		logger::info("Binding papyrus functions in utility script {}..."sv, script);
		Bind(*a_vm);
		logger::info("Finished binding functions."sv);
		return true;
	}

#ifndef NDEBUG
	bool UnitTest_Serialization::Save(SKSE::SerializationInterface* a_intfc) {
		logger::info("  >Reading UnitTest_Serialization..."sv);
		if (!a_intfc->OpenRecord('UTSM', 1u)) {
			logger::critical("    Failed to open UTSM record."sv);
			return false;
		}

		if (!a_intfc->WriteRecordData(hasWeapon)) {
			logger::critical("    Failed to write hasWeapon to the interface."sv);
			return false;
		}
		if (!hasWeapon) {
			return true;
		}
		if (!weapon) {
			logger::critical("    Unit test doesn't have a weapon set when it should."sv);
			return false;
		}
		if (!a_intfc->WriteRecordData(weapon->GetFormID())) {
			logger::critical("    Failed to write {} FormID."sv, clib_util::editorID::get_editorID(weapon));
			return false;
		}
		logger::info("  >Finished writing UnitTest_Serialization."sv);
		return true;
	}

	bool UnitTest_Serialization::Load(SKSE::SerializationInterface* a_intfc) {
		logger::info("  >Reading UnitTest_Serialization..."sv);
		bool hasWeap = false;
		if (!a_intfc->ReadRecordData(hasWeap)) {
			logger::critical("    Failed to read hasWeapon from the interface."sv);
			return false;
		}
		if (!hasWeap) {
			return true;
		}

		auto result = Serialization::GetFormFromInterface<RE::TESObjectWEAP>(a_intfc);
		if (result.status != Serialization::GetFormFromInterfaceResult::Success) {
			logger::critical("    Failed to read weapon from the interface with error: {}"sv,
				Serialization::GetFormFromInterfaceResult_ToString(result.status));
			return false;
		}
		if (!result.form.has_value()) {
			logger::critical("    Serialization didn't produce a weapon, despite success."sv);
			return false;
		}
		SetWeaponForm(result.form.value());
		return true;
	}

	bool UnitTest_Serialization::Revert([[maybe_unused]] SKSE::SerializationInterface* a_intfc) {
		weapon = nullptr;
		hasWeapon = false;
		return true;
	}

	bool UnitTest_Serialization::ResetWeaponForm() {
		hasWeapon = false;
		weapon = nullptr;
		return true;
	}

	bool UnitTest_Serialization::SetWeaponForm(RE::TESObjectWEAP* a_weap) {
		hasWeapon = true;
		weapon = a_weap;
		return true;
	}

	RE::TESObjectWEAP* UnitTest_Serialization::GetCurrentWeap() const {
		return weapon;
	}
#endif
}
