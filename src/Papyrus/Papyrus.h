#pragma once

#ifndef NDEBUG
#include "Serialization/Serde.h"
#endif

namespace Papyrus {
#define BIND(a_method, ...) a_vm.RegisterFunction(#a_method##sv, script, a_method __VA_OPT__(, ) __VA_ARGS__)
#define BIND_EVENT(a_method, ...) a_vm.RegisterFunction(#a_method##sv, script, a_method __VA_OPT__(, ) __VA_ARGS__)
#define STATIC_ARGS [[maybe_unused]] VM *a_vm, [[maybe_unused]] StackID a_stackID, RE::StaticFunctionTag *

	using VM = RE::BSScript::Internal::VirtualMachine;
	using StackID = RE::VMStackID;
	inline auto script = fmt::format("SEA_{}"sv, Plugin::NAME);
	bool RegisterFunctions(VM* a_vm);

#ifndef NDEBUG
	class UnitTest_Serialization :
		public REX::Singleton<UnitTest_Serialization>,
		public Serialization::Serializable
	{
	public:
		bool Save(SKSE::SerializationInterface* a_intfc) override;
		bool Load(SKSE::SerializationInterface* a_intfc) override;
		bool Revert([[maybe_unused]] SKSE::SerializationInterface* a_intfc);

		bool ResetWeaponForm();
		bool SetWeaponForm(RE::TESObjectWEAP* a_weap);
		RE::TESObjectWEAP* GetCurrentWeap() const;

	private:
		bool               hasWeapon{ false };
		RE::TESObjectWEAP* weapon{ nullptr };
	};
#endif
}