#pragma once

namespace Data
{
	bool PreloadModObjects();

	class ModObjectManager : 
		public REX::Singleton<ModObjectManager>
	{
	public:
		const std::string QuestName = fmt::format("{}_ModObjectsQuest"sv, Plugin::NAME);
		const std::string ScriptName = fmt::format("{}_ModObjectsScript"sv, Plugin::NAME);

		bool PreLoad();

		[[nodiscard]] RE::TESForm* Get(std::string_view a_key) const;
	private:
		util::istring_map<RE::TESForm*> objects;
	};

	template <typename T>
	[[nodiscard]] inline T* ModObject(std::string_view a_key)
	{
		if (const auto object = ModObjectManager::GetSingleton()->Get(a_key))
			return object->As<T>();
		return nullptr;
	}

	inline static constexpr const char* MOD_OBJECT = "FAKE_MOD_OBJECT";
}