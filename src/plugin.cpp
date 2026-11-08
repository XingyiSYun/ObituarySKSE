// ObituarySKSE - SKSE plugin entry point.
//
// 目标运行时：Skyrim SE/AE 1.7.104.0（SKSE 2.3.1），CommonLibSSE-NG。

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <SKSE/Logger.h>

#include "Obituary.h"
#include "Menu.h"

#include <type_traits>

using namespace std::literals;

SKSE_EXPORT constinit SKSE::PluginDeclaration SKSEPlugin_Version({
	.Version = REL::Version{ 1, 0, 0, 0 },
	.Name = "ObituarySKSE"sv,
	.Author = "Obituary Remake"sv,
	.SupportEmail = ""sv,
	.StructCompatibility = SKSE::StructCompatibility::Independent,
	.RuntimeCompatibility = {
		REL::Version{ 1, 5, 97, 0 },
		REL::Version{ 1, 6, 1170, 0 },
		REL::Version{ 1, 7, 104, 0 },
	},
	.MinimumSKSEVersion = REL::Version{ 0, 0, 0, 0 },
});

static_assert(std::is_same_v<std::remove_cvref_t<decltype(SKSEPlugin_Version)>,
	SKSE::PluginDeclaration>);

SKSE_EXPORT bool SKSEPlugin_Query(SKSE::QueryInterface*, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = SKSEPlugin_Version.GetName().data();
	a_info->version = static_cast<std::uint32_t>(SKSEPlugin_Version.GetVersion().pack());
	return true;
}

namespace
{
	void OnSKSEMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		if (!a_msg) {
			return;
		}
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kPostLoad:
			SKSE::log::info("ObituarySKSE: kPostLoad (sender='{}')", a_msg->sender);
			break;
		case SKSE::MessagingInterface::kDataLoaded:
			SKSE::log::info("ObituarySKSE: kDataLoaded, initializing");
			obit::Manager::GetSingleton().Initialize();
			obit::Menu::Register();
			break;
		case SKSE::MessagingInterface::kPreLoadGame:
			SKSE::log::info("ObituarySKSE: kPreLoadGame");
			break;
		default:
			break;
		}
	}

	void OnSave(SKSE::SerializationInterface* a_intfc)
	{
		obit::Manager::GetSingleton().Save(a_intfc);
	}

	void OnLoad(SKSE::SerializationInterface* a_intfc)
	{
		obit::Manager::GetSingleton().Load(a_intfc);
	}

	void OnRevert(SKSE::SerializationInterface*)
	{
		obit::Manager::GetSingleton().Revert();
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	SKSE::log::init();

	const auto* declaration = SKSE::PluginDeclaration::GetSingleton();
	const auto  runtime = REL::Module::get().version();
	SKSE::log::info("{} v{} loading (runtime {})",
		declaration->GetName(), declaration->GetVersion().string(), runtime.string());

	if (auto* serialization = SKSE::GetSerializationInterface()) {
		serialization->SetUniqueID('OBIT');
		serialization->SetSaveCallback(OnSave);
		serialization->SetLoadCallback(OnLoad);
		serialization->SetRevertCallback(OnRevert);
	} else {
		SKSE::log::error("ObituarySKSE: serialization interface unavailable");
	}

	if (auto* messaging = SKSE::GetMessagingInterface()) {
		if (!messaging->RegisterListener(OnSKSEMessage)) {
			SKSE::log::error("ObituarySKSE: failed to register messaging listener");
			return false;
		}
	} else {
		SKSE::log::error("ObituarySKSE: messaging interface unavailable");
		return false;
	}

	SKSE::log::info("ObituarySKSE: load finished");
	return true;
}