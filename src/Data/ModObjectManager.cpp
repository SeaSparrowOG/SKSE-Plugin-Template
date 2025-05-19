#include "ModObjectManager.h"

namespace Data
{
	bool ModObjectManager::PreLoad() {
		logger::info("  >Looking for script {} on quest {}..."sv, ScriptName, QuestName);
		const auto quest = RE::TESForm::LookupByEditorID<RE::TESQuest>(QuestName);
		if (!quest) {
			logger::critical("  >Failed to lookup quest."sv);
			return false;
		}

		const auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		if (!vm) {
			logger::critical("  >Failed to get VM"sv);
			return false;
		}

		const auto bindPolicy = vm->GetObjectBindPolicy();
		const auto handlePolicy = vm->GetObjectHandlePolicy();
		if (!bindPolicy || !handlePolicy) {
			logger::critical("  >Failed to get VM object policies"sv);
			return false;
		}

		const auto handle = handlePolicy->GetHandleForObject(RE::TESQuest::FORMTYPE, quest);
		RE::BSTScrapHashMap<RE::BSFixedString, RE::BSScript::Variable> properties;
		std::uint32_t nonConverted;
		bindPolicy->GetInitialPropertyValues(handle, ScriptName, properties, nonConverted);
		objects.clear();

		for (const auto& [name, var] : properties) {
			if (var.IsObjectArray()) {
				auto varArray = var.GetArray();

				RE::BSScript::Array::size_type index = 0;
				for (const auto& element : *varArray) {
					if (const auto foundForm = element.Unpack<RE::TESForm*>()) {
						auto keyName = fmt::format<std::string>("{}|{}", std::to_string(index), name.c_str());
						objects.emplace(keyName, foundForm);
					}
					else {
						logger::critical("  >Undefined element in array propery {}."sv, name.c_str());
						return false;
					}
					++index;
				}
			}
			else if (var.IsObject()) {
				const auto value = var.Unpack<RE::TESForm*>();
				if (!value) {
					logger::critical("  >Property {} is null."sv, name.c_str());
					return false;
				}
				objects.emplace(name, value);
			}
			else {
				logger::critical("  >Undefined property {}", name.c_str());
				return false;
			}
		}

		vm->ResetAllBoundObjects(handle);
		logger::info("  >Found {} Mod Objects."sv, properties.size());
		logger::info("Done."sv);
		return true;
	}

	RE::TESForm* ModObjectManager::Get(std::string_view a_key) const {
		if (const auto it = objects.find(a_key); it != objects.end()) {
			return it->second;
		}
		SKSE::stl::report_and_fail(fmt::format("Mod Object {} was requested, but was not found."sv, a_key));
	}

	bool PreloadModObjects() {
		SECTION_SEPARATOR;
		logger::info("Preloading Mod Objects..."sv);
		auto* objectManager = ModObjectManager::GetSingleton();
		if (!objectManager) {
			logger::critical("  >Failed to get Mod Object manager."sv);
			return false;
		}
		return objectManager->PreLoad();
	}
}