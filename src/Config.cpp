#include "Config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <string_view>
#include <unordered_map>

#include <spdlog/spdlog.h>

namespace obit
{
	namespace
	{
		std::string Trim(std::string s)
		{
			auto notSpace = [](unsigned char c) { return !std::isspace(c); };
			s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
			s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
			return s;
		}

		bool ParseBool(const std::string& v)
		{
			if (v.empty()) {
				return false;
			}
			const auto c = static_cast<char>(std::tolower(static_cast<unsigned char>(v[0])));
			return c == '1' || c == 't' || c == 'y';
		}

		// 极简 INI 解析：只处理 [Section] 与 key=value，忽略注释。
		std::unordered_map<std::string, std::string> ReadIni(const std::filesystem::path& a_path)
		{
			std::unordered_map<std::string, std::string> out;
			std::ifstream in(a_path);
			if (!in) {
				return out;
			}
			std::string line;
			std::string section;
			while (std::getline(in, line)) {
				if (!line.empty() && line.back() == '\r') {
					line.pop_back();
				}
				const auto first = line.find_first_not_of(" \t");
				if (first == std::string::npos || line[first] == ';' || line[first] == '#') {
					continue;
				}
				if (line[first] == '[') {
					const auto close = line.find(']', first);
					if (close != std::string::npos) {
						section = Trim(line.substr(first + 1, close - first - 1));
					}
					continue;
				}
				const auto eq = line.find('=');
				if (eq == std::string::npos) {
					continue;
				}
				auto key = Trim(line.substr(first, eq - first));
				auto val = Trim(line.substr(eq + 1));
				if (key.empty()) {
					continue;
				}
				out[section.empty() ? key : section + "." + key] = val;
			}
			return out;
		}

		std::string ToBool(bool b) { return b ? "1" : "0"; }
	}

	ConfigStore& ConfigStore::GetSingleton()
	{
		static ConfigStore instance;
		return instance;
	}

	std::filesystem::path ConfigStore::UserIniPath() const
	{
		return std::filesystem::path(kSettingsDir) / (std::string(kModName) + ".ini");
	}

	void ConfigStore::Load()
	{
		auto& s = _settings;
		s = Settings{};  // 回到编译期默认值
		_hasBlacklist = false;

		std::error_code ec;
		auto path = UserIniPath();
		bool legacy = false;
		if (!std::filesystem::exists(path, ec)) {
			const std::filesystem::path old{ kLegacySettingsFile };
			if (std::filesystem::exists(old, ec)) {
				path = old;
				legacy = true;
			}
		}
		_lastWrite = std::filesystem::exists(path, ec) ? std::filesystem::last_write_time(path, ec) :
		                                                 std::filesystem::file_time_type{};

		const auto kv = ReadIni(path);
		if (kv.empty()) {
			spdlog::info("ObituarySKSE: no user settings at {}, using defaults", path.string());
			return;
		}
		if (legacy) {
			spdlog::info("ObituarySKSE: migrating settings from legacy path {}", path.string());
		}

		auto getBool = [&](const char* key, bool& target) {
			if (auto it = kv.find(key); it != kv.end()) {
				target = ParseBool(it->second);
			}
		};
		auto getInt = [&](const char* key, int& target) {
			if (auto it = kv.find(key); it != kv.end()) {
				try {
					target = std::stoi(it->second);
				} catch (...) {
				}
			}
		};

		getBool("General.bModEnabled", s.bModEnabled);
		getBool("Log.bLogEnabled", s.bLogEnabled);
		getBool("Log.bLogUniqueOnly", s.bLogUniqueOnly);
		getBool("Log.bLogCreatures", s.bLogCreatures);
		getBool("Log.bLogPlayerKill", s.bLogPlayerKill);
		getBool("Log.bLogAllowPlayer", s.bLogAllowPlayer);
		getBool("Log.bLogWriteToFile", s.bLogWriteToFile);
		getInt("Log.iMaxLogSize", s.iMaxLogSize);
		getBool("Notification.bNotificationEnabled", s.bNotificationEnabled);
		getBool("Notification.bNotifUniqueOnly", s.bNotifUniqueOnly);
		getBool("Notification.bNotifCreatures", s.bNotifCreatures);
		getBool("Notification.bNotifPlayerKill", s.bNotifPlayerKill);
		getInt("Notification.iNotifMinRelIndex", s.iNotifMinRelIndex);
		getInt("Notification.iNotifMethodIndex", s.iNotifMethodIndex);
		getBool("Notification.bNotifTeleportOption", s.bNotifTeleportOption);
		getBool("Notification.bNotifTeleportResurrectOption", s.bNotifTeleportResurrectOption);
		getBool("Notification.bNotifResurrectOption", s.bNotifResurrectOption);
		getBool("Notification.bNotifResurrectedKeepsItems", s.bNotifResurrectedKeepsItems);
		getInt("Notification.iNotifResurrectDragonSoulCost", s.iNotifResurrectDragonSoulCost);
		getInt("Log.iMaxEntriesPerPage", s.iMaxEntriesPerPage);
		getBool("Debug.bVerboseLog", s.bVerboseLog);

		if (auto it = kv.find("Blacklist.Entries"); it != kv.end()) {
			s.sBlacklist = it->second;
			_hasBlacklist = true;
		}

		s.iMaxLogSize = std::clamp(s.iMaxLogSize, 16, 65535);
		s.iMaxEntriesPerPage = std::clamp(s.iMaxEntriesPerPage, 4, 64);
		s.iNotifMinRelIndex = std::clamp(s.iNotifMinRelIndex, 0, 8);
		s.iNotifMethodIndex = std::clamp(s.iNotifMethodIndex, 0, 1);
		s.iNotifResurrectDragonSoulCost = std::max(0, s.iNotifResurrectDragonSoulCost);

		spdlog::info("ObituarySKSE: settings loaded from {}", path.string());
		if (legacy) {
			Save();
		}
	}

	void ConfigStore::Save() const
	{
		const auto path = UserIniPath();
		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);

		const auto& s = _settings;
		std::ofstream out(path, std::ios::trunc);
		if (!out) {
			spdlog::error("ObituarySKSE: failed to write {}", path.string());
			return;
		}
		out << "; ObituarySKSE - written by the plugin\n";
		out << "[General]\n";
		out << "bModEnabled=" << ToBool(s.bModEnabled) << "\n";
		out << "\n";
		out << "[Log]\n";
		out << "bLogEnabled=" << ToBool(s.bLogEnabled) << "\n";
		out << "bLogUniqueOnly=" << ToBool(s.bLogUniqueOnly) << "\n";
		out << "bLogCreatures=" << ToBool(s.bLogCreatures) << "\n";
		out << "bLogPlayerKill=" << ToBool(s.bLogPlayerKill) << "\n";
		out << "bLogAllowPlayer=" << ToBool(s.bLogAllowPlayer) << "\n";
		out << "bLogWriteToFile=" << ToBool(s.bLogWriteToFile) << "\n";
		out << "iMaxLogSize=" << s.iMaxLogSize << "\n";
		out << "iMaxEntriesPerPage=" << s.iMaxEntriesPerPage << "\n\n";
		out << "[Notification]\n";
		out << "bNotificationEnabled=" << ToBool(s.bNotificationEnabled) << "\n";
		out << "bNotifUniqueOnly=" << ToBool(s.bNotifUniqueOnly) << "\n";
		out << "bNotifCreatures=" << ToBool(s.bNotifCreatures) << "\n";
		out << "bNotifPlayerKill=" << ToBool(s.bNotifPlayerKill) << "\n";
		out << "iNotifMinRelIndex=" << s.iNotifMinRelIndex << "\n";
		out << "iNotifMethodIndex=" << s.iNotifMethodIndex << "\n";
		out << "bNotifTeleportOption=" << ToBool(s.bNotifTeleportOption) << "\n";
		out << "bNotifTeleportResurrectOption=" << ToBool(s.bNotifTeleportResurrectOption) << "\n";
		out << "bNotifResurrectOption=" << ToBool(s.bNotifResurrectOption) << "\n";
		out << "bNotifResurrectedKeepsItems=" << ToBool(s.bNotifResurrectedKeepsItems) << "\n";
		out << "iNotifResurrectDragonSoulCost=" << s.iNotifResurrectDragonSoulCost << "\n\n";
		out << "[Debug]\n";
		out << "bVerboseLog=" << ToBool(s.bVerboseLog) << "\n\n";
		out << "[Blacklist]\n";
		out << "Entries=" << s.sBlacklist << "\n";
	}

	bool ConfigStore::ReloadIfChanged()
	{
		std::error_code ec;
		const auto path = UserIniPath();
		if (!std::filesystem::exists(path, ec)) {
			return false;
		}
		const auto t = std::filesystem::last_write_time(path, ec);
		if (ec || t == _lastWrite) {
			return false;
		}
		Load();
		return true;
	}
}