#include "Notifier.h"
#include "Obituary.h"

#include <chrono>
#include <thread>
#include <vector>

namespace obit::Notifier
{
	namespace
	{
		// MessageBox 的按钮回调是 C 函数指针，拿不到上下文，这里用静态状态。
		RE::FormID              g_pendingActorId{ 0 };
		std::vector<int>        g_buttonActions;

		void OnMessageBoxPressed(std::uint8_t a_index)
		{
			const auto index = static_cast<std::size_t>(a_index);
			if (index >= g_buttonActions.size()) {
				return;
			}
			auto* actor = RE::TESForm::LookupByID<RE::Actor>(g_pendingActorId);
			g_pendingActorId = 0;

			switch (g_buttonActions[index]) {
			case 1:
				TeleportTo(actor);
				break;
			case 2:
				ResurrectActor(actor);
				break;
			case 3:
				TeleportAndResurrect(actor);
				break;
			default:
				break;
			}
			g_buttonActions.clear();
		}

		std::string LocalizedText(std::string_view a_key, std::string_view a_default)
		{
			return Localization::GetSingleton().Get(a_key, a_default);
		}
	}

	void NotifyHUD(std::string_view a_message)
	{
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		if (!vm) {
			return;
		}

		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
		auto* args = RE::MakeFunctionArguments(std::string(a_message));
		vm->DispatchStaticCall("Debug", "Notification", args, callback);
	}

	void ShowDeathMessageBox(RE::Actor* a_actor)
	{
		if (!a_actor) {
			return;
		}
		auto& manager = Manager::GetSingleton();
		const auto& settings = manager.GetSettings();

		const auto name = manager.DisplayName(a_actor);
		auto text = Localization::Replace(
			LocalizedText(keys::kHasDied, "%s has died."), "%s", name);

		g_buttonActions.clear();
		g_pendingActorId = a_actor->GetFormID();

		// 按钮顺序：关闭 / 传送 / 复活 / 传送并复活（按开关增减）
		std::vector<std::string> labels;
		g_buttonActions.clear();
		labels.push_back(LocalizedText("btnnothing", "Nothing"));
		g_buttonActions.push_back(0);
		if (settings.bNotifTeleportOption) {
			labels.push_back(LocalizedText("btnteleport", "Teleport"));
			g_buttonActions.push_back(1);
		}
		if (settings.bNotifResurrectOption) {
			labels.push_back(LocalizedText("btnresurrect", "Resurrect"));
			g_buttonActions.push_back(2);
		}
		if (settings.bNotifTeleportResurrectOption) {
			labels.push_back(LocalizedText("btnteleportresurrect", "Teleport & Resurrect"));
			g_buttonActions.push_back(3);
		}

		switch (labels.size()) {
		case 4:
			RE::MessageBoxMenu::Create(text.c_str(), &OnMessageBoxPressed, 0, 0, 0,
				labels[0].c_str(), labels[1].c_str(), labels[2].c_str(), labels[3].c_str());
			break;
		case 3:
			RE::MessageBoxMenu::Create(text.c_str(), &OnMessageBoxPressed, 0, 0, 0,
				labels[0].c_str(), labels[1].c_str(), labels[2].c_str());
			break;
		case 2:
			RE::MessageBoxMenu::Create(text.c_str(), &OnMessageBoxPressed, 0, 0, 0,
				labels[0].c_str(), labels[1].c_str());
			break;
		default:
			RE::MessageBoxMenu::Create(text.c_str(), &OnMessageBoxPressed, 0, 0, 0, labels[0].c_str());
			break;
		}
	}

	namespace
	{
		// 传送后等待玩家真正到位再复活：同格 + 距离阈值内，最多等 10 秒
		constexpr float kArrivalRadius = 300.0f;
		constexpr int   kArrivalMaxTries = 40;

		bool PlayerArrivedNear(RE::Actor* a_actor)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player || !a_actor) {
				return false;
			}
			if (player->GetParentCell() != a_actor->GetParentCell()) {
				return false;
			}
			return player->GetPosition().GetDistance(a_actor->GetPosition()) <= kArrivalRadius;
		}

		void ScheduleArrivalCheck(RE::FormID a_id, int a_try)
		{
			std::thread([a_id, a_try]() {
				std::this_thread::sleep_for(std::chrono::milliseconds(250));
				SKSE::GetTaskInterface()->AddTask([a_id, a_try]() {
					auto* actor = RE::TESForm::LookupByID<RE::Actor>(a_id);
					if (!actor || actor->IsDeleted()) {
						return;
					}
					if (PlayerArrivedNear(actor)) {
						ResurrectActor(actor);
						return;
					}
					if (a_try + 1 >= kArrivalMaxTries) {
						NotifyHUD(Localization::GetSingleton().Get("tpnotarrived",
							"Teleport did not complete; resurrect cancelled."));
						return;
					}
					ScheduleArrivalCheck(a_id, a_try + 1);
				});
			}).detach();
		}
	}

	void TeleportAndResurrect(RE::Actor* a_actor)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player || !a_actor) {
			return;
		}
		player->MoveTo(a_actor);
		ScheduleArrivalCheck(a_actor->GetFormID(), 0);
	}

	void TeleportTo(RE::Actor* a_actor)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (player && a_actor) {
			player->MoveTo(a_actor);
		}
	}

	void ResurrectActor(RE::Actor* a_actor)
	{
		if (!a_actor || a_actor->IsDisabled() || a_actor->IsDeleted()) {
			return;
		}

		auto& manager = Manager::GetSingleton();
		const auto& settings = manager.GetSettings();
		auto* player = RE::PlayerCharacter::GetSingleton();

		// 龙魂成本
		if (settings.iNotifResurrectDragonSoulCost > 0) {
			if (!player) {
				return;
			}
			const auto souls = player->GetActorValue(RE::ActorValue::kDragonSouls);
			if (souls < static_cast<float>(settings.iNotifResurrectDragonSoulCost)) {
				auto msg = Localization::Replace(
					LocalizedText(keys::kNotEnoughDragonSouls, "You do not have enough dragon souls to resurrect %s."),
					"%s", manager.DisplayName(a_actor));
				NotifyHUD(msg);
				return;
			}
			player->DamageActorValue(RE::ActorValue::kDragonSouls,
				static_cast<float>(settings.iNotifResurrectDragonSoulCost));
		}

		const bool creature = manager.IsCreature(a_actor);
		const bool keepItems = settings.bNotifResurrectedKeepsItems && !creature;

		// 记录死亡前身上穿的东西，复活后重新穿上（原版靠法术效果脚本做同一件事）
		std::vector<RE::TESBoundObject*> worn;
		if (keepItems) {
			for (const auto& [object, data] : a_actor->GetInventory()) {
				if (!object || !data.second) {
					continue;
				}
				if (data.second->IsWorn()) {
					worn.push_back(object);
				}
			}
		}

		// 左右手单独记下来（武器/盾/法术），复活后单独装回去
		auto* rightHand = keepItems ? a_actor->GetEquippedObject(false) : nullptr;
		auto* leftHand = keepItems ? a_actor->GetEquippedObject(true) : nullptr;

		// resetInventory=false 保留物品栏；true 则是原版「不保留物品」的路径
		a_actor->Resurrect(!keepItems, true);

		for (auto* form : { rightHand, leftHand }) {
			if (auto* object = form ? form->As<RE::TESBoundObject>() : nullptr) {
				if (auto* equipManager = RE::ActorEquipManager::GetSingleton()) {
					equipManager->EquipObject(a_actor, object, nullptr, 1, nullptr, true, true, true, true);
				}
			}
		}

		for (auto* object : worn) {
			if (object) {
				if (auto* equipManager = RE::ActorEquipManager::GetSingleton()) {
					equipManager->EquipObject(a_actor, object, nullptr, 1, nullptr, true, true, true, true);
				}
			}
		}

		auto msg = Localization::Replace(
			LocalizedText(keys::kHasBeenResurrected, "%s has been resurrected."), "%s", manager.DisplayName(a_actor));
		NotifyHUD(msg);
	}
}