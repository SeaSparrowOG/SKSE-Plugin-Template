#include "Data/ModObjectManager.h"
#include "Hooks/Hooks.h"
#include "Papyrus/Papyrus.h"
#include "Settings/INI/INISettings.h"
#include "Settings/JSON/JSONSettings.h"

static void MessageEventCallback(SKSE::MessagingInterface::Message* a_msg)
{
	static auto* jsonHolder = Settings::JSON::Holder::GetSingleton();
	if (!jsonHolder) {
		REX::FAIL("Failed to get internal JSON logger."sv);
	}

	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		if (!Data::PreloadModObjects()) {
			REX::FAIL(
				fmt::format("Failed to preload mod objects. Check the log at Documents/My Games/Skyrim Special Edition/{}.log for more information."sv, Plugin::NAME));
		}
		SECTION_SEPARATOR;
		REX::INFO("Finished startup tasks, enjoy your game!"sv);
		break;
	default:
		break;
	}
}

#ifdef SKYRIM_AE
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []()
	{
		SKSE::PluginVersionData v{};

		v.PluginVersion(Plugin::VERSION);
		v.PluginName(Plugin::NAME);
		v.AuthorName("SeaSparrow"sv);
		v.UsesAddressLibrary();
		v.UsesUpdatedStructs();

		return v;
	}();
#endif

SKSE_PLUGIN_QUERY(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION[0];

	if (a_skse->IsEditor()) {
		REX::CRITICAL("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
#ifdef SKYRIM_AE
	if (ver < SKSE::RUNTIME_SSE_LATEST) {
#else
	if (ver < SKSE::RUNTIME_1_5_39) {
#endif
		REX::CRITICAL("Unsupported runtime version {}", ver.string());
		return false;
	}

	return true;
	}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface * a_skse)
{
	SKSE::InitInfo info;
	info.hook = false;
	info.trampoline = false;
	info.trampolineSize = 0u;
	
	SKSE::Init(a_skse, info);
	REX::INFO("Author: SeaSparrow"sv);
	SECTION_SEPARATOR;

#ifdef SKYRIM_AE
	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_SSE_LATEST) {
		return false;
	}
#endif

	REX::INFO("Performing startup tasks..."sv);

	if (!Settings::INI::Read()) {
		REX::FAIL(
			fmt::format("Failed to load the INI settings. Check the log at (Documents/My Games/Skyrim Special Edition/{}.log for more information."sv, Plugin::NAME));
	}
	SECTION_SEPARATOR;
	if (!Hooks::Install()) {
		REX::FAIL(
			fmt::format("Failed to install the necessary hooks. Check the log at (Documents/My Games/Skyrim Special Edition/{}.log for more information."sv, Plugin::NAME));
	}

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(&MessageEventCallback);

	SECTION_SEPARATOR;
	if (!Settings::JSON::Preload()) {
#ifdef NDEBUG
		REX::FAIL(
			fmt::format("Failed to parse configs. Check the log (Documents/My Games/Skyrim Special Edition/{}.log for more information."sv, Plugin::NAME));
#endif
	}
	SECTION_SEPARATOR;
	if (!Papyrus::RegisterFunctions()) {
		REX::FAIL(
			fmt::format("Failed to register the new Papyrus functions. Check the log at (Documents/My Games/Skyrim Special Edition/{}.log for more information."sv, Plugin::NAME));
	}

	return true;
}