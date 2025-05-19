#include "Data/ModObjectManager.h"
#include "Hooks/Hooks.h"
#include "Papyrus/papyrus.h"
#include "Serialization/Serde.h"
#include "Settings/INISettings.h"

namespace
{
	void InitializeLog()
	{
		auto path = logger::log_directory();
		if (!path) {
			util::report_and_fail("Failed to find standard logging directory"sv);
		}

		*path /= fmt::format("{}.log"sv, Plugin::NAME);
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

#ifndef NDEBUG
		const auto level = spdlog::level::debug;
#else 
		const auto level = spdlog::level::info;
#endif

		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
		log->set_level(level);
		log->flush_on(level);
		spdlog::set_default_logger(std::move(log));
	}
}

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

extern "C" DLLEXPORT bool SKSEAPI
SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = Plugin::VERSION[0];
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION[0];

	if (a_skse->IsEditor()) {
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_6_1130) {
		return false;
	}

	return true;
}

static void MessageEventCallback(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		if (!Data::PreloadModObjects()) {
			SKSE::stl::report_and_fail("Failed to preload mod objects. Check the log for more information."sv);
		}
		SECTION_SEPARATOR;
		logger::info("Finished startup tasks, enjoy your game!"sv);
		break;
	default:
		break;
	}
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();
	SECTION_SEPARATOR;
	logger::info("{} v{}"sv, Plugin::NAME, Plugin::VERSION.string());
	logger::info("Author: SeaSparrow"sv);
	SECTION_SEPARATOR;
	SKSE::Init(a_skse);

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_6_1130) {
		return false;
	}

	logger::info("Performing startup tasks..."sv);

	if (!Hooks::Install()) {
		SKSE::stl::report_and_fail("Failed to install hooks."sv);
	}

	SKSE::GetPapyrusInterface()->Register(Papyrus::RegisterFunctions);

	const auto messaging = SKSE::GetMessagingInterface();
	messaging->RegisterListener(&MessageEventCallback);

	SECTION_SEPARATOR;
	logger::info("Setting up serialization system..."sv);
	const auto serialization = SKSE::GetSerializationInterface();
	serialization->SetUniqueID(Serialization::ID);
	serialization->SetSaveCallback(&Serialization::SaveCallback);
	serialization->SetLoadCallback(&Serialization::LoadCallback);
	serialization->SetRevertCallback(&Serialization::RevertCallback);
	logger::info("  >Registered necessary functions."sv);

	return true;
}