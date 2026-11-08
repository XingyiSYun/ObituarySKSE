// SKSE Menu Framework 菜单（官方 SKSEMenuFramework.h，external\ 下）。
// 所有界面文本走 Localization（Strings.json），便于本地化。

#include "Menu.h"

#include "Localization.h"
#include "Obituary.h"

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "SKSEMenuFramework.h"

namespace obit::Menu
{
	namespace
	{
		std::string T(std::string_view a_key, std::string_view a_fallback)
		{
			return Localization::GetSingleton().Get(a_key, a_fallback);
		}

		// 悬停提示（对应原版 MCM 的 help 文本，来自 Strings.json 的 *info 键）
		void HelpTip(std::string_view a_text)
		{
			if (a_text.empty() || !ImGuiMCP::IsItemHovered()) {
				return;
			}
			ImGuiMCP::BeginTooltip();
			ImGuiMCP::PushTextWrapPos(420.0f);
			ImGuiMCP::TextUnformatted(std::string(a_text).c_str());
			ImGuiMCP::PopTextWrapPos();
			ImGuiMCP::EndTooltip();
		}

		bool Toggle(const std::string& a_label, bool* a_value, std::string_view a_help)
		{
			const bool changed = ImGuiMCP::Checkbox(a_label.c_str(), a_value);
			HelpTip(a_help);
			return changed;
		}

		bool Slider(const std::string& a_label, int* a_value, int a_min, int a_max, std::string_view a_help)
		{
			const bool changed = ImGuiMCP::SliderInt(a_label.c_str(), a_value, a_min, a_max, "%d");
			HelpTip(a_help);
			return changed;
		}

		void RenderSettings()
		{
			auto& manager = Manager::GetSingleton();
			// 让 MCM/F1 之外改过的值在打开菜单时同步进来（现在只有 F1 会写）
			manager.ReloadSettingsIfChanged();
			auto& s = manager.GetSettings();
			bool changed = false;

			ImGuiMCP::TextUnformatted(T("menutitle", "Obituary").c_str());
			ImGuiMCP::Separator();

			changed |= Toggle(T("modenabled", "Mod Enabled"), &s.bModEnabled, T("modenabledinfo", ""));

			ImGuiMCP::Separator();
			if (ImGuiMCP::CollapsingHeader(T("menuheader_log", "Log Settings").c_str())) {
				changed |= Toggle(T("logenabled", "Log Enabled"), &s.bLogEnabled, T("logenabledinfo", ""));
				changed |= Toggle(T("loguniqueonly", "Unique Only"), &s.bLogUniqueOnly, T("loguniqueonlyinfo", ""));
				changed |= Toggle(T("logcreatures", "Creatures"), &s.bLogCreatures, T("logcreaturesinfo", ""));
				changed |= Toggle(T("logplayerkill", "Player Kill"), &s.bLogPlayerKill, T("logplayerkillinfo", ""));
				changed |= Toggle(T("logallowplayer", "Allow Player"), &s.bLogAllowPlayer, T("logallowplayerinfo", ""));
				changed |= Toggle(T("logwritetofile", "Write to File"), &s.bLogWriteToFile, T("logwritetofileinfo", ""));
				changed |= Slider(T("maxlogsize", "Max Log Size"), &s.iMaxLogSize, 16, 8192, T("maxlogsizeinfo", ""));
				changed |= Slider(T("maxentriesperpage", "Max Entries Per Page"), &s.iMaxEntriesPerPage, 4, 64,
					T("maxentriesperpageinfo", ""));
				const auto clearLog = T("menuclear_log", "Clear Log");
				const bool cleared = ImGuiMCP::Button(clearLog.c_str());
				HelpTip(T("clearloginfo", ""));
				if (cleared) {
					manager.ClearLog();
				}
			}

			ImGuiMCP::Separator();
			if (ImGuiMCP::CollapsingHeader(T("menuheader_notif", "Notification Settings").c_str())) {
				changed |= Toggle(T("notificationenabled", "Notification Enabled"), &s.bNotificationEnabled,
					T("notificationenabledinfo", ""));
				changed |= Toggle(T("notifuniqueonly", "Unique Only"), &s.bNotifUniqueOnly, T("notifuniqueonlyinfo", ""));
				changed |= Toggle(T("notifcreatures", "Creatures"), &s.bNotifCreatures, T("notifcreaturesinfo", ""));
				changed |= Toggle(T("notifplayerkill", "Player Kill"), &s.bNotifPlayerKill, T("notifplayerkillinfo", ""));

				// 界面直接显示关系等级 -4..4（内部仍存原版的 0..8 索引，保持 ini 兼容）
				int relRank = s.iNotifMinRelIndex - 4;
				if (ImGuiMCP::SliderInt(T("menurelabel", "Min. Relationship").c_str(), &relRank, -4, 4, "%d")) {
					s.iNotifMinRelIndex = std::clamp(relRank + 4, 0, 8);
					changed = true;
				}
				HelpTip(T("notifminrelationshipinfo", ""));
				// 每个关系等级都列出来（不用省略号）
				ImGuiMCP::PushTextWrapPos(0.0f);
				ImGuiMCP::TextUnformatted(T("menurelvalues", "").c_str());
				ImGuiMCP::PopTextWrapPos();

				const auto methodLabel = T("menumethodlabel", "Method");
				if (ImGuiMCP::SliderInt(methodLabel.c_str(), &s.iNotifMethodIndex, 0, 1, "%d")) {
					changed = true;
				}
				HelpTip(T("notifmethodinfo", ""));

				changed |= Toggle(T("notifteleportoption", "Teleport Option"), &s.bNotifTeleportOption,
					T("notifteleportoptioninfo", ""));
				changed |= Toggle(T("notifresurrectoption", "Resurrect Option"), &s.bNotifResurrectOption,
					T("notifresurrectoptioninfo", ""));
				changed |= Toggle(T("notifteleportresurrectoption", "Teleport & Resurrect Option"),
					&s.bNotifTeleportResurrectOption, T("notifteleportresurrectoptioninfo", ""));
				changed |= Toggle(T("notifresurrectedkeepsitems", "Resurrected Keeps Items"),
					&s.bNotifResurrectedKeepsItems, T("notifresurrectedkeepsitemsinfo", ""));
				changed |= Slider(T("notifresurrectdragonsoulcost", "Resurrect Dragon Soul Cost"),
					&s.iNotifResurrectDragonSoulCost, 0, 20, T("notifresurrectdragonsoulcostinfo", ""));
			}

			ImGuiMCP::Separator();
			const auto restore = T("restoredefaults", "Restore Defaults");
			const bool doRestore = ImGuiMCP::Button(restore.c_str());
			HelpTip(T("restoredefaultsinfo", ""));
			if (doRestore) {
				s = Settings{};
				manager.PersistSettings();
				changed = false;
			}

			if (changed) {
				manager.PersistSettings();
			}
		}
		int g_logPage = 0;
		int g_selected = -1;

		void RenderLog()
		{
			auto& manager = Manager::GetSingleton();
			const auto& s = manager.GetSettings();
			const auto& entries = manager.Entries();

			const int perPage = std::max(4, s.iMaxEntriesPerPage);
			const int total = static_cast<int>(entries.size());
			const int pages = std::max(1, (total + perPage - 1) / perPage);
			g_logPage = std::clamp(g_logPage, 0, pages - 1);

			char status[256];
			std::snprintf(status, sizeof(status), T("menuentries", "Entries: %d / %d  Page: %d / %d").c_str(),
				total, std::max(16, s.iMaxLogSize), g_logPage + 1, pages);
			ImGuiMCP::TextUnformatted(status);

			const auto first = T("menufirst", "First Page");
			const auto prev = T("menuprev", "Previous Page");
			const auto next = T("menunext", "Next Page");
			const auto clearLog = T("menuclear_log", "Clear Log");

			if (ImGuiMCP::Button(first.c_str())) {
				g_logPage = 0;
			}
			ImGuiMCP::SameLine();
			if (ImGuiMCP::Button(prev.c_str()) && g_logPage > 0) {
				--g_logPage;
			}
			ImGuiMCP::SameLine();
			if (ImGuiMCP::Button(next.c_str()) && g_logPage + 1 < pages) {
				++g_logPage;
			}
			ImGuiMCP::SameLine();
			if (ImGuiMCP::Button(clearLog.c_str())) {
				manager.ClearLog();
				g_selected = -1;
			}

			ImGuiMCP::Separator();

			const int endExclusive = total - g_logPage * perPage;
			const int startInclusive = std::max(0, endExclusive - perPage);
			for (int i = endExclusive - 1; i >= startInclusive; --i) {
				if (i < 0 || i >= total) {
					continue;
				}
				ImGuiMCP::PushID(std::to_string(i).c_str());
				const bool selected = (g_selected == i);
				if (ImGuiMCP::Selectable(entries[static_cast<std::size_t>(i)].entry.c_str(), selected)) {
					g_selected = selected ? -1 : i;
				}
				ImGuiMCP::PopID();
			}

			if (g_selected >= 0 && g_selected < total) {
				ImGuiMCP::Separator();
				const auto details = manager.DetailsForDisplay(entries[static_cast<std::size_t>(g_selected)]);
				ImGuiMCP::TextUnformatted(details.c_str());
			}
		}

		void RenderBlacklist()
		{
			auto& manager = Manager::GetSingleton();

			RE::Actor* target = nullptr;
			if (auto* pick = RE::CrosshairPickData::GetSingleton()) {
				if (auto ref = pick->GetActiveTarget()) {
					target = ref.get() ? ref.get()->As<RE::Actor>() : nullptr;
				}
			}

			ImGuiMCP::TextUnformatted(T("menublacklist", "Blacklist").c_str());
			ImGuiMCP::Separator();

			const bool hasTarget = target && target->GetActorBase();
			if (hasTarget) {
				char line[256];
				std::snprintf(line, sizeof(line), T("menutarget", "Crosshair target: %s").c_str(),
					manager.DisplayName(target).c_str());
				ImGuiMCP::TextUnformatted(line);
			} else {
				ImGuiMCP::TextUnformatted(T("menunotarget", "Crosshair target: none").c_str());
			}

			// 没有准星目标时按钮置灰（与原版 MCM 一致）
			ImGuiMCP::BeginDisabled(!hasTarget);
			const auto add = T("menuadd", "Add to Blacklist");
			const auto remove = T("menuremove", "Remove from Blacklist");
			if (ImGuiMCP::Button(add.c_str()) && hasTarget) {
				manager.AddBlacklist(target->GetActorBase());
			}
			ImGuiMCP::SameLine();
			if (ImGuiMCP::Button(remove.c_str()) && hasTarget) {
				manager.RemoveBlacklist(target->GetActorBase());
			}
			ImGuiMCP::EndDisabled();

			ImGuiMCP::Separator();
			const auto ids = manager.Blacklist();
			char countLine[128];
			std::snprintf(countLine, sizeof(countLine), T("menublacklisted", "Blacklisted: %d").c_str(),
				static_cast<int>(ids.size()));
			ImGuiMCP::TextUnformatted(countLine);

			std::vector<RE::FormID> sorted(ids.begin(), ids.end());
			std::sort(sorted.begin(), sorted.end());
			for (const auto id : sorted) {
				ImGuiMCP::PushID(std::to_string(id).c_str());
				auto* form = RE::TESForm::LookupByID(id);
				std::string label = form ? form->GetName() : std::string{};
				if (label.empty()) {
					char buf[32];
					std::snprintf(buf, sizeof(buf), "%08X", id);
					label = buf;
				}
				label += "  [" + T("menuremove", "Remove") + "]";
				if (ImGuiMCP::Button(label.c_str())) {
					if (auto* npc = form ? form->As<RE::TESNPC>() : nullptr) {
						manager.RemoveBlacklist(npc);
					}
				}
				ImGuiMCP::PopID();
			}

			ImGuiMCP::Separator();
			if (ImGuiMCP::Button(T("menuclear_blacklist", "Clear Blacklist").c_str())) {
				manager.ClearBlacklist();
			}
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			SKSE::log::info("ObituarySKSE: SKSEMenuFramework not installed, ImGui menu disabled");
			return;
		}
		SKSEMenuFramework::SetSection(T("menutitle", "Obituary"));
		SKSEMenuFramework::AddSectionItem(T("menusettings", "Settings"), &RenderSettings);
		SKSEMenuFramework::AddSectionItem(T("menulog", "Log"), &RenderLog);
		SKSEMenuFramework::AddSectionItem(T("menublacklist", "Blacklist"), &RenderBlacklist);
		SKSE::log::info("ObituarySKSE: SKSE Menu Framework pages registered (API v{})",
			SKSEMenuFramework::GetMenuFrameworkAPIVersion());
	}
}