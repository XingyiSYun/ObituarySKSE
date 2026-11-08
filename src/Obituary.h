#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include <deque>
#include <string>
#include <unordered_set>
#include <vector>

#include "Config.h"
#include "Localization.h"

namespace obit
{
	struct LogEntry
	{
		std::string entry;    // "<名字> (<死亡时间>)"
		std::string details;  // %1..%8 模板
	};

	// 整个模组的运行时状态。所有回调都在主线程（AddTask 保证）。
	class Manager final : public RE::BSTEventSink<RE::TESDeathEvent>
	{
	public:
		static Manager& GetSingleton();

		// kDataLoaded 之后调用：加载设置/字符串，缓存关键字，注册事件
		void Initialize();
		void Shutdown();

		// ---- 数据访问（菜单用） ----
		const std::deque<LogEntry>& Entries() const { return _entries; }
		void                        ClearLog();

		const std::unordered_set<RE::FormID>& Blacklist() const { return _blacklist; }
		bool                                  IsBlacklisted(RE::TESNPC* a_base) const;
		void                                  AddBlacklist(RE::TESNPC* a_base);
		void                                  RemoveBlacklist(RE::TESNPC* a_base);
		void                                  ClearBlacklist();

		// MCM 改完 ini 后调用：重读设置并按字符串重建黑名单
		void ReloadSettingsIfChanged();
		// 把当前黑名单集合写回设置字符串（并落盘），供 MCM 的 input 显示
		void SyncBlacklistToString();

		// 立刻写入用户设置（菜单改完调用）
		void PersistSettings() const { ConfigStore::GetSingleton().Save(); }

		Settings&       GetSettings() { return ConfigStore::GetSingleton().Mutable(); }
		const Settings& GetSettings() const { return ConfigStore::GetSingleton().Get(); }

		// ---- 序列化 ----
		void Save(SKSE::SerializationInterface* a_intfc);
		void Load(SKSE::SerializationInterface* a_intfc);
		void Revert();

		// ---- 供 Notifier 使用 ----
		RE::Actor*              Player() const;
		RE::BGSKeyword*         CreatureKeyword() const { return _creatureKeyword; }
		bool                    IsCreature(RE::Actor* a_actor) const;
		std::string             DisplayName(RE::Actor* a_actor) const;
		std::string             EntryLineFor(RE::Actor* a_actor, std::string_view a_deathTime) const;
		std::string             DetailsFor(RE::Actor* a_actor, RE::Actor* a_killer, std::string_view a_deathTime) const;
		std::string             FormatGameTime() const;
		// 详情页用：把 6 位 FormID 按当前加载顺序补回主文件索引，显示成 8 位
		std::string             DetailsForDisplay(const LogEntry& a_entry) const;

	protected:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>* a_source) override;

	private:
		Manager() = default;
		~Manager() override = default;

		void HandleDeath(RE::Actor* a_victim, RE::Actor* a_killer);

		void QueueNotification(RE::Actor* a_actor);
		void QueueLogEntry(LogEntry a_entry);

		void ScheduleProcess();
		void ProcessQueues();

		void ProcessNotifications();
		void ProcessLogEntries();

		void WriteEntryToFile(const LogEntry& a_entry);

		// 用设置里的 sBlacklist 字符串重建黑名单集合
		void RebuildBlacklistFromString();

		int  RelationshipRank(RE::Actor* a_victim) const;

		// 状态
		std::deque<RE::Actor*>  _notifQueue;
		std::deque<LogEntry>    _logQueue;
		std::deque<LogEntry>    _entries;
		std::unordered_set<RE::FormID> _blacklist;

		RE::BGSKeyword* _creatureKeyword{ nullptr };
		std::filesystem::file_time_type _blacklistFileTime{};
		bool            _registered{ false };
		bool            _processing{ false };
	};
}