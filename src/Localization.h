#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace obit
{
	// 读取原版 Strings.json（沿用同一路径，现有简体中文译文可直接生效）。
	// 键名大小写不敏感（原版 JsonUtil 也是这么做的）。
	class Localization
	{
	public:
		static Localization& GetSingleton();

		void Load();

		// 找不到键时返回内置默认值 defaultText
		std::string Get(std::string_view a_key, std::string_view a_default) const;

		// 单次替换（对应原版 StringReplace）
		static std::string Replace(std::string a_str, std::string_view a_from, std::string_view a_to);

		// 把 %1..%8 换成八种标签
		std::string SubstituteDetails(std::string a_details) const;

	private:
		std::unordered_map<std::string, std::string> _strings;
		bool _loaded{ false };
	};

	// 常用键（避免拼写错误）
	namespace keys
	{
		inline constexpr std::string_view kHasDied = "hasdied";
		inline constexpr std::string_view kHasBeenResurrected = "hasbeenresurrected";
		inline constexpr std::string_view kNotEnoughDragonSouls = "notenoughdragonsouls";
		inline constexpr std::string_view kNoName = "noname";
		inline constexpr std::string_view kNone = "none";
		inline constexpr std::string_view kYes = "yes";
		inline constexpr std::string_view kNo = "no";
		inline constexpr std::string_view kMale = "male";
		inline constexpr std::string_view kFemale = "female";
		inline constexpr std::string_view kLabelName = "labelname";
		inline constexpr std::string_view kLabelSex = "labelsex";
		inline constexpr std::string_view kLabelRace = "labelrace";
		inline constexpr std::string_view kLabelUnique = "labelunique";
		inline constexpr std::string_view kLabelMaster = "labelmaster";
		inline constexpr std::string_view kLabelFormId = "labelformid";
		inline constexpr std::string_view kLabelKiller = "labelkiller";
		inline constexpr std::string_view kLabelTimeOfDeath = "labeltimeofdeath";
		inline constexpr std::string_view kHasDiedTitle = "hasdied";
	}
}