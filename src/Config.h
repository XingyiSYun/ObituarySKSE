#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace obit
{
	// 与原版 _asd_ObituaryQuestScript 的属性一一对应（默认值也照抄）。
	struct Settings
	{
		// General
		bool bModEnabled{ true };

		// Log
		bool bLogEnabled{ true };
		bool bLogUniqueOnly{ true };
		bool bLogCreatures{ true };
		bool bLogPlayerKill{ true };
		bool bLogAllowPlayer{ false };
		bool bLogWriteToFile{ false };
		int  iMaxLogSize{ 1024 };

		// Notification
		bool bNotificationEnabled{ false };
		bool bNotifUniqueOnly{ true };
		bool bNotifCreatures{ true };
		bool bNotifPlayerKill{ false };
		int  iNotifMinRelIndex{ 4 };  // 0 = -4 宿敌 ... 4 = 0 相识 ... 8 = 4 爱人
		int  iNotifMethodIndex{ 0 };  // 0 = 消息框, 1 = 通知
		bool bNotifTeleportOption{ false };
		bool bNotifTeleportResurrectOption{ false };  // 本模组新增：传送并复活（默认关）
		bool bNotifResurrectOption{ false };
		bool bNotifResurrectedKeepsItems{ true };
		int  iNotifResurrectDragonSoulCost{ 0 };

		// 日志浏览页的每页条数（F1 菜单用）
		int iMaxEntriesPerPage{ 16 };

		// 黑名单：设置文件里的 [Blacklist] Entries，插件解析成 FormID 集合
		std::string sBlacklist;  // "0001A67C,Nazeem"

		// 非原版：控制台/日志噪音
		bool bVerboseLog{ false };
	};

	// 设置文件（相对游戏根目录）。以前放在 MCM Helper 的目录下，现在只由本插件与
	// F1 菜单维护，改到插件自己的目录；旧路径仍然兼容读取并自动迁移。
	inline constexpr const char* kModName = "ObituarySKSE";
	inline constexpr const char* kSettingsDir = "Data/SKSE/Plugins/ObituarySKSE";
	inline constexpr const char* kLegacySettingsFile = "Data/MCM/Settings/ObituarySKSE.ini";
	inline constexpr const char* kStringsPath = "Data/SKSE/Plugins/Obituary/Strings.json";
	inline constexpr const char* kLogRoot = "Data/SKSE/Plugins/Obituary/Logs";

	class ConfigStore
	{
	public:
		static ConfigStore& GetSingleton();

		const Settings& Get() const { return _settings; }
		Settings&       Mutable() { return _settings; }

		// 读取 Data/MCM/Settings/ObituarySKSE.ini（不存在就用默认值）
		void Load();
		// 写回同一个 ini，保持与 MCM Helper 兼容的键名
		void Save() const;

		// 重新读取（MCM 修改后由菜单关闭/定时器触发）；返回是否真的重新读了
		bool ReloadIfChanged();

		// ini 里是否有 [Blacklist] Entries（MCM 或本模组写过），决定黑名单以谁为准
		bool HasBlacklistEntries() const { return _hasBlacklist; }

		std::filesystem::path UserIniPath() const;

	private:
		Settings _settings{};
		std::filesystem::file_time_type _lastWrite{};
		bool _hasBlacklist{ false };
	};
}