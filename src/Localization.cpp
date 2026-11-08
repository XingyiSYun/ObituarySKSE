#include "Localization.h"
#include "Config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

#include <spdlog/spdlog.h>

namespace obit
{
	namespace
	{
		std::string ToLower(std::string s)
		{
			std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return s;
		}

		void SkipWs(std::string_view s, std::size_t& i)
		{
			while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) {
				++i;
			}
		}

		bool ParseJsonString(std::string_view s, std::size_t& i, std::string& out)
		{
			SkipWs(s, i);
			if (i >= s.size() || s[i] != '"') {
				return false;
			}
			++i;
			out.clear();
			while (i < s.size()) {
				const char c = s[i++];
				if (c == '"') {
					return true;
				}
				if (c != '\\') {
					out.push_back(c);
					continue;
				}
				if (i >= s.size()) {
					return false;
				}
				const char esc = s[i++];
				switch (esc) {
				case 'n': out.push_back('\n'); break;
				case 't': out.push_back('\t'); break;
				case 'r': out.push_back('\r'); break;
				case 'b': out.push_back('\b'); break;
				case 'f': out.push_back('\f'); break;
				case '"': out.push_back('"'); break;
				case '\\': out.push_back('\\'); break;
				case '/': out.push_back('/'); break;
				case 'u': {
					if (i + 4 > s.size()) {
						return false;
					}
					unsigned cp = 0;
					for (int k = 0; k < 4; ++k) {
						const char h = s[i++];
						cp <<= 4;
						if (h >= '0' && h <= '9') cp |= static_cast<unsigned>(h - '0');
						else if (h >= 'a' && h <= 'f') cp |= static_cast<unsigned>(h - 'a' + 10);
						else if (h >= 'A' && h <= 'F') cp |= static_cast<unsigned>(h - 'A' + 10);
						else return false;
					}
					if (cp < 0x80) {
						out.push_back(static_cast<char>(cp));
					} else if (cp < 0x800) {
						out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
						out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
					} else {
						out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
						out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
						out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
					}
					break;
				}
				default: out.push_back(esc); break;
				}
			}
			return false;
		}

		// 解析 {"string": { "key": "value", ... }}；只取扁平字符串字典
		bool ParseStringsFile(const std::string& a_path, std::unordered_map<std::string, std::string>& a_out)
		{
			std::ifstream in(a_path, std::ios::binary);
			if (!in) {
				return false;
			}
			std::stringstream ss;
			ss << in.rdbuf();
			const std::string text = ss.str();
			const std::string_view s{ text };

			std::size_t i = 0;
			bool found = false;
			while (i < s.size()) {
				SkipWs(s, i);
				if (i >= s.size() || s[i] != '"') {
					++i;
					continue;
				}
				std::string key;
				if (!ParseJsonString(s, i, key)) {
					++i;
					continue;
				}
				SkipWs(s, i);
				if (i >= s.size() || s[i] != ':') {
					continue;
				}
				++i;
				SkipWs(s, i);
				if (i < s.size() && s[i] == '{') {
					const bool isStrings = (ToLower(key) == "string");
					++i;
					while (i < s.size()) {
						SkipWs(s, i);
						if (i < s.size() && s[i] == '}') {
							++i;
							break;
						}
						std::string k;
						if (!ParseJsonString(s, i, k)) {
							++i;
							continue;
						}
						SkipWs(s, i);
						if (i >= s.size() || s[i] != ':') {
							continue;
						}
						++i;
						std::string v;
						if (!ParseJsonString(s, i, v)) {
							while (i < s.size() && s[i] != '"' && s[i] != ',' && s[i] != '}') ++i;
							continue;
						}
						if (isStrings) {
							a_out[ToLower(k)] = v;
							found = true;
						}
					}
					continue;
				}
			}
			return found;
		}
	}

	Localization& Localization::GetSingleton()
	{
		static Localization instance;
		return instance;
	}

	void Localization::Load()
	{
		_strings.clear();
		_loaded = false;

		// 1) 原版的 Strings.json（已有简体中文译文）
		_loaded |= ParseStringsFile(kStringsPath, _strings);
		// 2) 本模组自己的覆盖文件（可补充按钮文本等新键）
		_loaded |= ParseStringsFile("Data/SKSE/Plugins/ObituarySKSE/Strings.json", _strings);

		if (!_loaded) {
			spdlog::warn("ObituarySKSE: no Strings.json found, using built-in defaults");
		}
		spdlog::info("ObituarySKSE: loaded {} localized strings", _strings.size());
	}

	std::string Localization::Get(std::string_view a_key, std::string_view a_default) const
	{
		const auto it = _strings.find(std::string(a_key));
		if (it != _strings.end()) {
			return it->second;
		}
		return std::string(a_default);
	}

	std::string Localization::Replace(std::string a_str, std::string_view a_from, std::string_view a_to)
	{
		if (a_from.empty()) {
			return a_str;
		}
		std::size_t pos = 0;
		while ((pos = a_str.find(a_from, pos)) != std::string::npos) {
			a_str.replace(pos, a_from.size(), a_to);
			pos += a_to.size();
		}
		return a_str;
	}

	std::string Localization::SubstituteDetails(std::string a_details) const
	{
		auto rep = [&](std::string_view from, std::string_view key, std::string_view fallback) {
			a_details = Replace(std::move(a_details), from, Get(key, fallback));
		};
		rep("%1", keys::kLabelName, "Name:");
		rep("%2", keys::kLabelSex, "Sex:");
		rep("%3", keys::kLabelRace, "Race:");
		rep("%4", keys::kLabelUnique, "Unique:");
		rep("%5", keys::kLabelMaster, "Master:");
		rep("%6", keys::kLabelFormId, "Form ID:");
		rep("%7", keys::kLabelKiller, "Killer:");
		rep("%8", keys::kLabelTimeOfDeath, "Time of Death:");
		return a_details;
	}
}