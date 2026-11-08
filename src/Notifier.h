#pragma once

#include <RE/Skyrim.h>

#include <string>
#include <string_view>

namespace obit::Notifier
{
	// 对应原版 Debug.Notification（走 VM 静态调用，保证是游戏原生通知）
	void NotifyHUD(std::string_view a_message);

	// 对应原版 AlertMessageBox.Show() + 传送/复活选项
	void ShowDeathMessageBox(RE::Actor* a_actor);

	void TeleportTo(RE::Actor* a_actor);

	// 新增：传送到目标身边，确认到位后再复活（原版没有）
	void TeleportAndResurrect(RE::Actor* a_actor);

	// 对应原版「复活」（含龙魂消耗与保留物品）
	void ResurrectActor(RE::Actor* a_actor);
}