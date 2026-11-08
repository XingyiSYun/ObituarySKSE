#include "Obituary.h"
#include "Notifier.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <thread>

namespace obit
{
	namespace
	{
		std::string SanitizeFileName(std::string s)
		{
			static constexpr std::string_view bad = "\\/:*?\"<>|";
			for (auto& c : s) {
				if (bad.find(c) != std::string_view::npos || static_cast<unsigned char>(c) < 0x20) {
					c = '_';
				}
			}
			if (s.empty()) {
				s = "Player";
			}
			return s;
		}

		std::string SanitizeText(std::string s)
		{
			for (auto& c : s) {
				if (c == '\n' || c == '\r') {
					c = ' ';
				}
			}
			return s;
		}

		std::string Pad(const std::string& s, int width, char fill = '0')
		{
			if (static_cast<int>(s.size()) >= width) {
				return s;
			}
			return std::string(static_cast<std::size_t>(width) - s.size(), fill) + s;
		}

		std::string FormIdToHex(RE::FormID a_id)
		{
			char buf[16];
			std::snprintf(buf, sizeof(buf), "%08X", a_id);
			return buf;
		}
	}

	Manager& Manager::GetSingleton()
	{
		static Manager instance;
		return instance;
	}

	RE::Actor* Manager::Player() const
	{
		return RE::PlayerCharacter::GetSingleton();
	}

	void Manager::Initialize()
	{
		ConfigStore::GetSingleton().Load();
		Localization::GetSingleton().Load();
		const auto& cfg = GetSettings();
		SKSE::log::info("ObituarySKSE: settings notif={} playerKill={} uniqueOnly={} creatures={} minRelIdx={} method={} log={} writeFile={}",
			cfg.bNotificationEnabled, cfg.bNotifPlayerKill, cfg.bNotifUniqueOnly, cfg.bNotifCreatures,
			cfg.iNotifMinRelIndex, cfg.iNotifMethodIndex, cfg.bLogEnabled, cfg.bLogWriteToFile);

		if (ConfigStore::GetSingleton().HasBlacklistEntries() ||
			std::filesystem::exists(std::filesystem::path{ "Data/SKSE/Plugins/ObituarySKSE/Blacklist.txt" })) {
			RebuildBlacklistFromString();
		}

		// ActorTypeCreature = 0x00013795 (Skyrim.esm)，原版 CreatureRaceKeyword 就是这个关键字
		_creatureKeyword = RE::TESForm::LookupByID<RE::BGSKeyword>(0x00013795);
		if (!_creatureKeyword) {
			SKSE::log::warn("ObituarySKSE: ActorTypeCreature keyword (0x00013795) not found");
		}

		if (!_registered) {
			if (auto* holder = RE::ScriptEventSourceHolder::GetSingleton()) {
				holder->AddEventSink<RE::TESDeathEvent>(this);
				_registered = true;
				SKSE::log::info("ObituarySKSE: TESDeathEvent sink registered");
			} else {
				SKSE::log::error("ObituarySKSE: ScriptEventSourceHolder unavailable");
			}
		}
	}

	void Manager::Shutdown()
	{
		if (_registered) {
			if (auto* holder = RE::ScriptEventSourceHolder::GetSingleton()) {
				holder->RemoveEventSink<RE::TESDeathEvent>(this);
			}
			_registered = false;
		}
		_notifQueue.clear();
		_logQueue.clear();
	}

	// ---------------------------------------------------------------- helpers

	bool Manager::IsCreature(RE::Actor* a_actor) const
	{
		if (!a_actor || !_creatureKeyword) {
			return false;
		}
		auto* race = a_actor->GetRace();
		return race && race->HasKeyword(_creatureKeyword);
	}

	std::string Manager::DisplayName(RE::Actor* a_actor) const
	{
		const auto& loc = Localization::GetSingleton();
		std::string name = a_actor ? a_actor->GetDisplayFullName() : std::string{};
		if (name.empty()) {
			return loc.Get(keys::kNoName, "[NO_NAME]");
		}
		return SanitizeText(name);
	}

	std::string Manager::FormatGameTime() const
	{
		auto* cal = RE::Calendar::GetSingleton();
		if (!cal) {
			return "??/??/???? ??:??";
		}
		const auto month = static_cast<int>(cal->GetMonth()) + 1;
		const auto day = static_cast<int>(cal->GetDay());
		const auto year = static_cast<int>(cal->GetYear());
		const float hourF = cal->GetHour();
		const auto hour = static_cast<int>(hourF);
		const auto minute = static_cast<int>((hourF - static_cast<float>(hour)) * 60.0f);

		char buf[64];
		std::snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02d:%02d", month, day, year, hour, minute);
		return buf;
	}

	std::string Manager::EntryLineFor(RE::Actor* a_actor, std::string_view a_deathTime) const
	{
		return DisplayName(a_actor) + " (" + std::string(a_deathTime) + ")";
	}

	std::string Manager::DetailsFor(RE::Actor* a_actor, RE::Actor* a_killer, std::string_view a_deathTime) const
	{
		const auto& loc = Localization::GetSingleton();
		const auto& s = GetSettings();

		std::string name = DisplayName(a_actor);

		std::string sex = loc.Get(keys::kMale, "Male ");
		if (a_actor) {
			if (auto* npc = a_actor->GetActorBase()) {
				if (npc->GetSex() == RE::SEX::kFemale) {
					sex = loc.Get(keys::kFemale, "Female ");
				}
			}
		}

		std::string race;
		if (a_actor) {
			if (auto* raceForm = a_actor->GetRace()) {
				race = raceForm->GetName();
				// 原版因为游戏字符串缓存会把名字改回小写，这里保留同样的首字母大写映射
				static const std::pair<std::string_view, std::string_view> kRaceNames[] = {
					{ "Argonian", "Argonian " }, { "Breton", "Breton " }, { "Dark Elf", "Dark Elf " },
					{ "High Elf", "High Elf " }, { "Imperial", "Imperial " }, { "Khajiit", "Khajiit " },
					{ "Nord", "Nord " }, { "Orc", "Orc " }, { "Redguard", "Redguard " },
					{ "Wood Elf", "Wood Elf " }
				};
				for (const auto& [from, to] : kRaceNames) {
					if (race == from) {
						race = to;
						break;
					}
				}
			}
		}

		bool unique = false;
		if (a_actor) {
			if (auto* npc = a_actor->GetActorBase()) {
				unique = npc->IsUnique();
			}
		}

		std::string master;
		std::string formId;
		if (a_actor) {
			if (auto* base = a_actor->GetActorBase()) {
				if (auto* file = base->GetFile(0)) {
					master = file->GetFilename();
				}
				// 8 位 hex 去掉前两位（主文件索引），与原版一致
				const auto full = FormIdToHex(base->GetFormID());
				formId = full.size() > 2 ? full.substr(2) : full;
			}
		}

		std::string killer = loc.Get(keys::kNone, "[NONE]");
		if (a_killer) {
			killer = DisplayName(a_killer);
		}

		const std::string yes = loc.Get(keys::kYes, "Yes ");
		const std::string no = loc.Get(keys::kNo, "No ");

		(void)s;
		return "%1 " + name +
		       "\n%2 " + sex +
		       "\n%3 " + race +
		       "\n%4 " + (unique ? yes : no) +
		       "\n%5 " + master +
		       "\n%6 " + formId +
		       "\n%7 " + killer +
		       "\n%8 " + std::string(a_deathTime);
	}

	namespace
	{
		// 同步收集 Papyrus 函数返回值
		class SyncCallback final : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			void operator()(RE::BSScript::Variable a_result) override
			{
				_result = a_result;
				_done = true;
			}
			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}

			bool                   _done{ false };
			RE::BSScript::Variable _result;
		};

		// 调用 Papyrus 的 Actor.GetRelationshipRank（原版脚本走的就是这条路径）。
		// 运行时会受存档 / 脚本改过的关系影响，这层状态在插件文件里看不到，
		// 所以必须问游戏本身，不能只查 BGSRelationship 表。
		bool TryGetRelationshipRankViaScript(RE::Actor* a_victim, float& a_out)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			if (!a_victim || !player || !vm) {
				return false;
			}
			auto* policy = vm->GetObjectHandlePolicy1();
			if (!policy) {
				return false;
			}
			const auto handle = policy->GetHandleForObject(a_victim->GetFormType(), a_victim);
			if (handle == policy->EmptyHandle()) {
				return false;
			}

			RE::BSTSmartPointer<SyncCallback>                        impl(new SyncCallback());
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> result(impl);
			auto* args = RE::MakeFunctionArguments(static_cast<RE::Actor*>(player));
			if (!vm->DispatchMethodCall(handle, "Actor", "GetRelationshipRank", args, result)) {
				return false;
			}
			if (!impl->_done) {
				return false;
			}
			a_out = impl->_result.GetFloat();
			return true;
		}
	}

	std::string Manager::DetailsForDisplay(const LogEntry& a_entry) const
	{
		std::string details = a_entry.details;

		auto valueOf = [&details](std::string_view a_marker) -> std::string {
			const auto pos = details.find(a_marker);
			if (pos == std::string::npos) {
				return {};
			}
			const auto start = pos + a_marker.size();
			auto end = details.find('\n', start);
			if (end == std::string::npos) {
				end = details.size();
			}
			return details.substr(start, end - start);
		};

		const auto master = valueOf("%5 ");
		const auto formId = valueOf("%6 ");

		// 存档里只存 6 位（跨加载顺序稳定），显示时按主文件名查当前编号补回前 2 位
		if (formId.size() == 6 && !master.empty()) {
			std::string prefix = "FE";  // 找不到或 light 插件都用 FE，与原版一致
			if (auto* handler = RE::TESDataHandler::GetSingleton()) {
				if (auto* file = handler->LookupModByName(master)) {
					if (!file->IsLight()) {
						char buf[8];
						std::snprintf(buf, sizeof(buf), "%02X", file->GetCompileIndex());
						prefix = buf;
					}
				}
			}
			const auto needle = "%6 " + formId;
			if (const auto pos = details.find(needle); pos != std::string::npos) {
				details.replace(pos, needle.size(), "%6 " + prefix + formId);
			}
		}

		return Localization::GetSingleton().SubstituteDetails(details);
	}
	int Manager::RelationshipRank(RE::Actor* a_victim) const
	{
		float scriptRank = 0.0f;
		const bool scriptOk = TryGetRelationshipRankViaScript(a_victim, scriptRank);

		// 兜底：直接查 BGSRelationship 表（只能看到插件里定义的关系）
		int tableRank = 0;
		if (auto* player = Player()) {
			if (auto* victimBase = a_victim ? a_victim->GetActorBase() : nullptr) {
				if (auto* playerBase = player->GetActorBase()) {
					auto* rel = RE::BGSRelationship::GetRelationship(victimBase, playerBase);
					if (!rel) {
						rel = RE::BGSRelationship::GetRelationship(playerBase, victimBase);
					}
					tableRank = rel ? 4 - static_cast<int>(rel->level.get()) : 0;
				}
			}
		}

		const int rank = scriptOk ? static_cast<int>(std::lround(scriptRank)) : tableRank;
		if (GetSettings().bVerboseLog) {
			SKSE::log::info("ObituarySKSE: relationship '{}' script={} table={} -> {}",
				a_victim ? (a_victim->GetDisplayFullName() ? a_victim->GetDisplayFullName() : "") : "",
				scriptOk ? scriptRank : -999.0f, tableRank, rank);
		}
		return rank;
	}
	// ---------------------------------------------------------------- filters

	RE::BSEventNotifyControl Manager::ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>*)
	{
		if (!a_event || !a_event->dead) {
			return RE::BSEventNotifyControl::kContinue;
		}
		auto* victim = a_event->actorDying ? a_event->actorDying->As<RE::Actor>() : nullptr;
		auto* killer = a_event->actorKiller ? a_event->actorKiller->As<RE::Actor>() : nullptr;
		if (!victim) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// 事件里直接做重活会拖慢死亡处理，先记下来，下一帧再筛
		SKSE::GetTaskInterface()->AddTask([victim, killer]() {
			Manager::GetSingleton().HandleDeath(victim, killer);
		});
		return RE::BSEventNotifyControl::kContinue;
	}

	void Manager::HandleDeath(RE::Actor* a_victim, RE::Actor* a_killer)
	{
		// 设置文件可能被外部（手改 / 其它工具）改动，处理死亡前先比对时间戳，
		// 有变化就重读，保证改完即时生效。
		ReloadSettingsIfChanged();

		const auto& s = GetSettings();
		if (!s.bModEnabled || !a_victim) {
			return;
		}

		auto* player = Player();
		const bool isPlayer = (a_victim == player);
		if (isPlayer && !s.bLogAllowPlayer) {
			return;
		}
		if (a_victim->IsDisabled()) {
			return;
		}
		if (a_victim->IsDeleted()) {
			return;
		}

		auto* base = a_victim->GetActorBase();
		if (!base || IsBlacklisted(base)) {
			return;
		}

		const bool isCreature = IsCreature(a_victim);
		const bool isUnique = base->IsUnique();
		const bool playerKill = (a_killer && a_killer == player);

		if (!s.bNotificationEnabled && !s.bLogEnabled) {
			return;
		}

		const int relRank = RelationshipRank(a_victim);
		bool queued = false;

		static constexpr int kRanks[] = { -4, -3, -2, -1, 0, 1, 2, 3, 4 };
		const int minRank = kRanks[std::clamp(s.iNotifMinRelIndex, 0, 8)];

		// ---- 通知 ----
		std::string notifWhy = "off";
		if (s.bNotificationEnabled) {
			if (!s.bNotifPlayerKill && playerKill) {
				notifWhy = "blocked:playerKill";
			} else if (s.bNotifUniqueOnly && !isUnique) {
				notifWhy = "blocked:notUnique";
			} else if (!s.bNotifCreatures && isCreature) {
				notifWhy = "blocked:creature";
			} else if (relRank < minRank) {
				notifWhy = "blocked:minRel(" + std::to_string(relRank) + "<" + std::to_string(minRank) + ")";
			} else {
				QueueNotification(a_victim);
				notifWhy = "queued";
				queued = true;
			}
		}

		// ---- 日志 ----
		std::string logWhy = "off";
		if (s.bLogEnabled) {
			if (!s.bLogPlayerKill && playerKill) {
				logWhy = "blocked:playerKill";
			} else if (s.bLogUniqueOnly && !isUnique) {
				logWhy = "blocked:notUnique";
			} else if (!s.bLogCreatures && isCreature) {
				logWhy = "blocked:creature";
			} else {
				const auto deathTime = FormatGameTime();
				LogEntry e;
				e.entry = EntryLineFor(a_victim, deathTime);
				e.details = DetailsFor(a_victim, a_killer, deathTime);
				QueueLogEntry(std::move(e));
				logWhy = "queued";
				queued = true;
			}
		}

		SKSE::log::info("ObituarySKSE: death '{}' unique={} creature={} playerKill={} rel={} minRel={} -> notif={} log={}",
			DisplayName(a_victim), isUnique, isCreature, playerKill, relRank, minRank, notifWhy, logWhy);

		if (queued) {
			ScheduleProcess();
		}
	}

	// ---------------------------------------------------------------- queues

	void Manager::QueueNotification(RE::Actor* a_actor)
	{
		_notifQueue.push_back(a_actor);
	}

	void Manager::QueueLogEntry(LogEntry a_entry)
	{
		_logQueue.push_back(std::move(a_entry));
	}

	void Manager::ScheduleProcess()
	{
		if (_processing) {
			return;
		}
		_processing = true;
		// 原版用 RegisterForSingleUpdate(2.0) 延迟处理，这里保持同样的 2 秒
		std::thread([]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(2000));
			SKSE::GetTaskInterface()->AddTask([]() {
				Manager::GetSingleton().ProcessQueues();
			});
		}).detach();
	}

	void Manager::ProcessQueues()
	{
		ProcessNotifications();
		ProcessLogEntries();
		_processing = false;
	}

	void Manager::ProcessNotifications()
	{
		const auto& loc = Localization::GetSingleton();
		const auto& s = GetSettings();

		for (auto* actor : _notifQueue) {
			if (!actor || actor->IsDeleted()) {
				continue;
			}
			const auto name = DisplayName(actor);
			auto message = Localization::Replace(loc.Get(keys::kHasDied, "%s has died."), "%s", name);

			if (s.iNotifMethodIndex == 1) {
				Notifier::NotifyHUD(message);
				if (s.bNotifTeleportOption || s.bNotifResurrectOption) {
					Notifier::ShowDeathMessageBox(actor);
				}
			} else {
				Notifier::ShowDeathMessageBox(actor);
			}
		}
		_notifQueue.clear();
	}

	void Manager::ProcessLogEntries()
	{
		const auto& s = GetSettings();
		const auto maxSize = static_cast<std::size_t>(std::max(16, s.iMaxLogSize));

		for (auto& e : _logQueue) {
			_entries.push_back(std::move(e));
			while (_entries.size() > maxSize) {
				_entries.pop_front();
			}
			if (s.bLogWriteToFile) {
				WriteEntryToFile(_entries.back());
			}
		}
		_logQueue.clear();
	}

	void Manager::WriteEntryToFile(const LogEntry& a_entry)
	{
		auto* cal = RE::Calendar::GetSingleton();
		const auto hours = cal ? static_cast<int>(cal->GetHoursPassed()) : 0;

		std::filesystem::path dir = std::filesystem::path("Data/SKSE/Plugins/Obituary/Logs") /
		                            SanitizeFileName(DisplayName(Player()));
		std::error_code ec;
		std::filesystem::create_directories(dir, ec);

		const auto path = dir / ("GameHour" + std::to_string(hours) + ".txt");
		std::ofstream out(path, std::ios::app);
		if (!out) {
			SKSE::log::warn("ObituarySKSE: cannot open log file {}", path.string());
			return;
		}
		out << "-\n" << a_entry.entry << "\n"
		    << Localization::GetSingleton().SubstituteDetails(a_entry.details) << "\n";
	}

	// ---------------------------------------------------------------- store/menu

	void Manager::ClearLog()
	{
		_entries.clear();
	}

	bool Manager::IsBlacklisted(RE::TESNPC* a_base) const
	{
		return a_base && _blacklist.contains(a_base->GetFormID());
	}

	void Manager::AddBlacklist(RE::TESNPC* a_base)
	{
		if (a_base) {
			_blacklist.insert(a_base->GetFormID());
			SyncBlacklistToString();
		}
	}

	void Manager::RemoveBlacklist(RE::TESNPC* a_base)
	{
		if (a_base) {
			_blacklist.erase(a_base->GetFormID());
			SyncBlacklistToString();
		}
	}

	void Manager::ClearBlacklist()
	{
		_blacklist.clear();
		SyncBlacklistToString();
	}

	void Manager::SyncBlacklistToString()
	{
		std::string out;
		for (const auto id : _blacklist) {
			char buf[16];
			std::snprintf(buf, sizeof(buf), "%08X", id);
			if (!out.empty()) {
				out += ",";
			}
			out += buf;
		}
		GetSettings().sBlacklist = out;
		ConfigStore::GetSingleton().Save();
	}

	namespace
	{
		// 支持 0x0001A67C / 0001A67C / Nazeem 三种写法
		RE::TESNPC* ResolveBlacklistToken(const std::string& a_token)
		{
			auto isHex = [](const std::string& t) {
				return !t.empty() && std::all_of(t.begin(), t.end(), [](unsigned char c) { return std::isxdigit(c) != 0; });
			};
			RE::TESForm* form = nullptr;
			try {
				const bool prefixed = a_token.size() > 2 && a_token[0] == '0' && (a_token[1] == 'x' || a_token[1] == 'X');
				if (prefixed) {
					form = RE::TESForm::LookupByID(static_cast<RE::FormID>(std::stoul(a_token.substr(2), nullptr, 16)));
				} else if (isHex(a_token) && a_token.size() >= 6) {
					form = RE::TESForm::LookupByID(static_cast<RE::FormID>(std::stoul(a_token, nullptr, 16)));
				} else {
					form = RE::TESForm::LookupByEditorID(a_token);
				}
			} catch (...) {
				form = nullptr;
			}
			if (!form) {
				SKSE::log::warn("ObituarySKSE: blacklist entry '{}' not found", a_token);
				return nullptr;
			}
			auto* npc = form->As<RE::TESNPC>();
			if (!npc) {
				SKSE::log::warn("ObituarySKSE: blacklist entry '{}' is not an ActorBase", a_token);
			}
			return npc;
		}
	}

	void Manager::RebuildBlacklistFromString()
	{
		_blacklist.clear();

		auto parseText = [this](const std::string& a_text) {
			std::string token;
			auto flush = [&]() {
				if (!token.empty()) {
					if (auto* npc = ResolveBlacklistToken(token)) {
						_blacklist.insert(npc->GetFormID());
					}
				}
				token.clear();
			};
			for (const char c : a_text) {
				if (c == ',' || c == ';' || c == ' ' || c == '\n' || c == '\r' || c == '\t') {
					flush();
				} else {
					token.push_back(c);
				}
			}
			flush();
		};

		// 来源 1：设置文件里的 [Blacklist] Entries
		parseText(GetSettings().sBlacklist);

		// 来源 2：Data/SKSE/Plugins/ObituarySKSE/Blacklist.txt（一行一个，# 注释）
		const std::filesystem::path file{ "Data/SKSE/Plugins/ObituarySKSE/Blacklist.txt" };
		std::ifstream in(file);
		if (in) {
			std::string line;
			while (std::getline(in, line)) {
				const auto hash = line.find('#');
				if (hash != std::string::npos) {
					line.erase(hash);
				}
				parseText(line);
			}
			std::error_code ec;
			_blacklistFileTime = std::filesystem::last_write_time(file, ec);
		}

		SKSE::log::info("ObituarySKSE: blacklist = {} entries", _blacklist.size());
	}

	void Manager::ReloadSettingsIfChanged()
	{
		bool changed = ConfigStore::GetSingleton().ReloadIfChanged();

		std::error_code ec;
		const std::filesystem::path file{ "Data/SKSE/Plugins/ObituarySKSE/Blacklist.txt" };
		const bool hasFile = std::filesystem::exists(file, ec);
		if (hasFile) {
			const auto t = std::filesystem::last_write_time(file, ec);
			if (!ec && t != _blacklistFileTime) {
				changed = true;
			}
		}

		if (changed && (ConfigStore::GetSingleton().HasBlacklistEntries() || hasFile)) {
			RebuildBlacklistFromString();
		}
	}

	// ---------------------------------------------------------------- serialization

	namespace
	{
		constexpr std::uint32_t kRecordVersion = 1;
		constexpr std::uint32_t kRecordType = 'OBIT';

		void WriteString(SKSE::SerializationInterface* a_intfc, const std::string& a_str)
		{
			const auto len = static_cast<std::uint16_t>(
				std::min<std::size_t>(a_str.size(), std::numeric_limits<std::uint16_t>::max()));
			a_intfc->WriteRecordData(len);
			if (len > 0) {
				a_intfc->WriteRecordData(a_str.data(), len);
			}
		}

		bool ReadString(SKSE::SerializationInterface* a_intfc, std::string& a_out)
		{
			std::uint16_t len = 0;
			if (a_intfc->ReadRecordData(len) != sizeof(len)) {
				return false;
			}
			a_out.resize(len);
			if (len > 0 && a_intfc->ReadRecordData(a_out.data(), len) != len) {
				return false;
			}
			return true;
		}
	}

	void Manager::Save(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc) {
			return;
		}
		if (!a_intfc->OpenRecord(kRecordType, kRecordVersion)) {
			SKSE::log::error("ObituarySKSE: failed to open co-save record");
			return;
		}
		const auto count = static_cast<std::uint32_t>(_entries.size());
		a_intfc->WriteRecordData(count);
		for (const auto& e : _entries) {
			WriteString(a_intfc, e.entry);
			WriteString(a_intfc, e.details);
		}
		const auto blCount = static_cast<std::uint32_t>(_blacklist.size());
		a_intfc->WriteRecordData(blCount);
		for (const auto id : _blacklist) {
			a_intfc->WriteRecordData(id);
		}
		SKSE::log::info("ObituarySKSE: saved {} log entries, {} blacklist forms", count, blCount);
	}

	void Manager::Load(SKSE::SerializationInterface* a_intfc)
	{
		if (!a_intfc) {
			return;
		}
		_entries.clear();
		_blacklist.clear();

		std::uint32_t type = 0;
		std::uint32_t version = 0;
		std::uint32_t length = 0;
		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (type != kRecordType || version != kRecordVersion) {
				continue;
			}
			std::uint32_t count = 0;
			if (a_intfc->ReadRecordData(count) != sizeof(count)) {
				continue;
			}
			const auto maxSize = static_cast<std::size_t>(std::max(16, GetSettings().iMaxLogSize));
			for (std::uint32_t i = 0; i < count; ++i) {
				LogEntry e;
				if (!ReadString(a_intfc, e.entry) || !ReadString(a_intfc, e.details)) {
					SKSE::log::error("ObituarySKSE: truncated log entry at {}", i);
					break;
				}
				_entries.push_back(std::move(e));
				while (_entries.size() > maxSize) {
					_entries.pop_front();
				}
			}

			std::uint32_t blCount = 0;
			if (a_intfc->ReadRecordData(blCount) == sizeof(blCount)) {
				for (std::uint32_t i = 0; i < blCount; ++i) {
					RE::FormID oldId = 0;
					if (a_intfc->ReadRecordData(oldId) != sizeof(oldId)) {
						break;
					}
					RE::FormID newId = 0;
					if (a_intfc->ResolveFormID(oldId, newId) && newId != 0) {
						_blacklist.insert(newId);
					}
				}
			}
		}
		const auto& cfg = GetSettings();
		SKSE::log::info("ObituarySKSE: settings notif={} playerKill={} uniqueOnly={} creatures={} minRelIdx={} method={} log={} writeFile={}",
			cfg.bNotificationEnabled, cfg.bNotifPlayerKill, cfg.bNotifUniqueOnly, cfg.bNotifCreatures,
			cfg.iNotifMinRelIndex, cfg.iNotifMethodIndex, cfg.bLogEnabled, cfg.bLogWriteToFile);

		if (ConfigStore::GetSingleton().HasBlacklistEntries() ||
			std::filesystem::exists(std::filesystem::path{ "Data/SKSE/Plugins/ObituarySKSE/Blacklist.txt" })) {
			RebuildBlacklistFromString();
		}

		SKSE::log::info("ObituarySKSE: loaded {} log entries, {} blacklist forms", _entries.size(), _blacklist.size());
	}

	void Manager::Revert()
	{
		_entries.clear();
		_blacklist.clear();
		_notifQueue.clear();
		_logQueue.clear();
		_processing = false;
	}
}