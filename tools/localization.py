# -*- coding: utf-8 -*-
r"""ObituarySKSE 本地化工具。

单一翻译源：Strings.json（键 -> 文本，扁平字典）。

本脚本做三件事：
  1. 合并原版 Obituary 的 Strings.json（保留已有中文译文）与本模组新增的键；
  2. 生成 MCM\Config\ObituarySKSE\config.json（MCM Helper 只认字面量，所以在这里注入文本）；
  3. 输出 Strings.en.json 英文模板与 LOCALIZATION.md 键表。

用法：
    python localization.py --strings <原版 Strings.json> --out <模组根目录>

翻译流程（给别人看的）：
    - 改 SKSE\Plugins\ObituarySKSE\Strings.json 里的文本（键不要动）
    - 跑一次本脚本，MCM 菜单会自动跟着变
    - 或者直接改 MCM\Config\ObituarySKSE\config.json 里的字面量（不需要 Python）
"""

import argparse
import json
import os
import sys

# ---------------------------------------------------------------- 新增键（本模组独有）

EXTRA_ZH = {
    # 菜单
    "menutitle": "讣告 Obituary",
    "menulogselect": "在左侧选择一条记录查看详情",
    "menusettings": "设置",
    "menulog": "日志",
    "menublacklist": "黑名单",
    "menuheader_log": "日志设置",
    "menuheader_notif": "通知设置",
    "menuclear_log": "清除日志",
    "menuentries": "条目：%d / %d     页：%d / %d",
    "menufirst": "第一页",
    "menuprev": "上一页",
    "menunext": "下一页",
    "maxentriesperpageinfo": "日志浏览页每页显示多少条。\n默认：16",
    "menutarget": "准星目标：%s",
    "menunotarget": "准星目标：无",
    "menuadd": "加入黑名单",
    "menuremove": "移出黑名单",
    "menublacklisted": "已列入：%d 个",
    "menuclear_blacklist": "清空黑名单",
    "menurelabel": "最低关系",
    "menurelvalues": "-4 宿敌 / -3 敌人 / -2 仇敌 / -1 对手 / 0 相识 / 1 朋友 / 2 知己 / 3 盟友 / 4 爱人",
    "menumethodlabel": "方式 0=消息框 1=通知",
    "menuunique": "仅特殊（Unique）",
    "blacklistpage": "黑名单",
    "blacklistinput": "黑名单（FormID 或 EditorID，逗号分隔）",
    "blacklistinputinfo": "手动排除的 ActorBase：FormID（0001A67C）或 EditorID（Nazeem），用逗号分隔。\n留空表示不排除。也可以在 F1 菜单里用准星选中目标一键加入。",
    # 消息框按钮
    "btnnothing": "离开",
    "btnteleport": "传送",
    "btnresurrect": "复活",
    "btnteleportresurrect": "传送并复活",
    "notifteleportresurrectoption": "传送并复活选项",
    "notifteleportresurrectoptioninfo": "若启用，在死亡通知中添加「传送并复活」：先传送到目标身边，确认到位后再复活。\n默认：开启",
    "tpnotarrived": "传送未完成，已取消复活。",
}

EXTRA_EN = {
    "menutitle": "Obituary",
    "menulogselect": "Select an entry on the left",
    "menusettings": "Settings",
    "menulog": "Log",
    "menublacklist": "Blacklist",
    "menuheader_log": "Log Settings",
    "menuheader_notif": "Notification Settings",
    "menuclear_log": "Clear Log",
    "menuentries": "Entries: %d / %d     Page: %d / %d",
    "menufirst": "First Page",
    "menuprev": "Previous Page",
    "menunext": "Next Page",
    "maxentriesperpageinfo": "How many entries are shown per page in the log browser.\nDefault: 16",
    "menutarget": "Crosshair target: %s",
    "menunotarget": "Crosshair target: none",
    "menuadd": "Add to Blacklist",
    "menuremove": "Remove from Blacklist",
    "menublacklisted": "Blacklisted: %d",
    "menuclear_blacklist": "Clear Blacklist",
    "menurelabel": "Min. Relationship",
    "menurelvalues": "-4 Archnemesis / -3 Enemy / -2 Foe / -1 Rival / 0 Acquaintance / 1 Friend / 2 Confidant / 3 Ally / 4 Lover",
    "menumethodlabel": "Method (0=Message Box, 1=Notification)",
    "menuunique": "Unique Only",
    "blacklistpage": "Blacklist",
    "blacklistinput": "Blacklist (FormID or EditorID, comma separated)",
    "blacklistinputinfo": "ActorBases to exclude: FormID (0001A67C) or EditorID (Nazeem), comma separated.\nEmpty means no exclusions. You can also add the crosshair target from the F1 menu.",
    "btnnothing": "Nothing",
    "btnteleport": "Teleport",
    "btnresurrect": "Resurrect",
    "btnteleportresurrect": "Teleport & Resurrect",
    "notifteleportresurrectoption": "Teleport & Resurrect Option",
    "notifteleportresurrectoptioninfo": "If enabled, adds a Teleport & Resurrect option: teleports you to the target and resurrects it only after you actually arrive.\nDefault: ON",
    "tpnotarrived": "Teleport did not complete; resurrect cancelled.",
}

# 原版 .psc 里的英文默认值（作为英文模板的回退）
EN_DEFAULTS = {
    "settings": "Settings", "modenabled": "Mod Enabled", "modenabledinfo": "Whether the mod is active or not.",
    "restoredefaults": "Restore Defaults", "restoredefaultsinfo": "Restores the default settings.",
    "restoredefaultsask": "Restore settings to default?", "restoredefaultsdone": "Default settings restored!",
    "click": "CLICK",
    "logsettings": "Log Settings", "logenabled": "Log Enabled",
    "logenabledinfo": "If enabled, deaths of NPCs will be logged.\nDefault: ON",
    "loguniqueonly": "Unique Only",
    "loguniqueonlyinfo": "If enabled, only deaths of NPCs with the Unique flag will be logged.\nDefault: ON",
    "logcreatures": "Creatures", "logcreaturesinfo": "If enabled, death of creatures will be logged.\nDefault: ON",
    "logplayerkill": "Player Kill", "logplayerkillinfo": "If enabled, NPCs killed by the Player will be logged.\nDefault: ON",
    "logallowplayer": "Allow Player", "logallowplayerinfo": "If enabled, allows the log to intercept the Player's death.\nDefault: OFF",
    "logwritetofile": "Write to File",
    "logwritetofileinfo": "If enabled, NPC deaths will be logged into a file.\nThe file will be located in Data/SKSE/Plugins/Obituary/Logs/<PlayerName>/<GameHour>.txt\nDefault: OFF",
    "maxlogsize": "Max Log Size", "maxlogsizedone": "Max Log Size changed to %d.",
    "maxlogsizeinfo": "The maximum amount of entries that can be logged into the MCM.\nHigher values will cause longer processing times.\nDefault: 1024",
    "clearlog": "Clear Log", "clearlogask": "Clear the log?", "clearlogdone": "Log cleared!", "clearloginfo": "Clears the log.",
    "notificationsettings": "Notification Settings", "notificationenabled": "Notification Enabled",
    "notificationenabledinfo": "If enabled, you will be notified of the deaths of NPCs.\nDefault: OFF",
    "notifuniqueonly": "Unique Only",
    "notifuniqueonlyinfo": "If enabled, you will only be notified of the deaths of NPCs with the Unique flag.\nDefault: ON",
    "notifcreatures": "Creatures", "notifcreaturesinfo": "If enabled, you will be notified of creature deaths.\nDefault: ON",
    "notifplayerkill": "Player Kill", "notifplayerkillinfo": "If enabled, you will be notified of NPCs killed by the Player.\nDefault: OFF",
    "notifminrelationship": "Minimum Relationship",
    "relarchnemesis": "-4 Archnemesis", "relenemy": "-3 Enemy", "relfoe": "-2 Foe", "relrival": "-1 Rival",
    "relacquaintance": "0 Acquaintance", "relfriend": "1 Friend", "relconfidant": "2 Confidant",
    "relally": "3 Ally", "rellover": "4 Lover",
    "notifminrelationshipinfo": "The minimum relationship rank required for a notification to show.\nDefault: 0 Acquaintance",
    "notifmethod": "Method", "notifmethodmessagebox": "Message Box", "notifmethodnotification": "Notification",
    "notifmethodinfo": "The method to use for notifying the Player of a death.\nDefault: Message Box",
    "notifteleportoption": "Teleport Option",
    "notifteleportoptioninfo": "If enabled, adds a Teleport option in the death notification.\nDefault: OFF",
    "notifresurrectoption": "Resurrect Option",
    "notifresurrectoptioninfo": "If enabled, adds a Resurrect option in the death notification.\nDefault: OFF",
    "notifresurrectedkeepsitems": "Resurrected Keeps Items",
    "notifresurrectedkeepsitemsinfo": "If enabled, resurrected NPCs will have the same stuff they had before they were resurrected\ninstead of having a new set of items in their inventory.\nDefault: ON",
    "notifresurrectdragonsoulcost": "Resurrect Dragon Soul Cost",
    "notifresurrectdragonsoulcostinfo": "The amount of dragon souls required for the Resurrect option to work.\nDefault: 0",
    "blacklistsettings": "Blacklist Settings", "blacklisttarget": "Target", "blacklistcheck": "Blacklisted",
    "blacklistadd": "Add to Blacklist", "blacklistaddask": "Add the target actor to the blacklist?",
    "blacklistadddone": "Target actor has been added to the blacklist.",
    "blacklistaddinfo": "Blacklists the target actor using its ActorBase.",
    "blacklistremove": "Remove from Blacklist", "blacklistremoveask": "Remove the target actor from the blacklist?",
    "blacklistremovedone": "Target actor has been removed from the blacklist.",
    "blacklistremoveinfo": "Removes the target actor from the blacklist using its ActorBase.",
    "blacklistclear": "Clear Blacklist", "blacklistclearask": "Clear the blacklist?",
    "blacklistcleardone": "Blacklist has been cleared.", "blacklistclearinfo": "Clears the blacklist.",
    "log": "Log", "maxentriesperpage": "Max Entries Per Page", "totalentries": "Total Entries: ", "page": "Page: ",
    "previouspage": "Previous Page", "nextpage": "Next Page", "firstpage": "First Page", "next10pages": "Next 10 Pages",
    "processingsettingsmsg": "A function of this mod is still processing.",
    "processinglogmsg": "The log is still updating.", "processingpleaseclosemsg": "Please close all menus.",
    "labelname": "Name:", "labelsex": "Sex:", "labelrace": "Race:", "labelunique": "Unique:",
    "labelmaster": "Master:", "labelformid": "Form ID:", "labelkiller": "Killer:", "labeltimeofdeath": "Time of Death:",
    "yes": "Yes ", "no": "No ", "none": "[NONE]", "noname": "[NO_NAME]", "notarget": "[NO_TARGET]",
    "male": "Male ", "female": "Female ",
    "hasdied": "%s has died.", "hasbeenresurrected": "%s has been resurrected.",
    "notenoughdragonsouls": "You do not have enough dragon souls to resurrect %s.",
}

def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)
    ap = argparse.ArgumentParser()
    ap.add_argument("--strings", default=None, help="原版 Obituary 的 Strings.json（可省略）")
    ap.add_argument("--out", default=root, help="模组根目录")
    args = ap.parse_args()

    strings = {}
    if args.strings and os.path.exists(args.strings):
        with open(args.strings, encoding="utf-8-sig") as f:
            strings.update(json.load(f).get("string", {}))
        print(f"读取原版字符串 {len(strings)} 条: {args.strings}")

    strings.update(EXTRA_ZH)
    print(f"合并后中文键 {len(strings)} 条")

    # 1) 运行时字符串
    out_dir = os.path.join(args.out, "SKSE", "Plugins", "ObituarySKSE")
    os.makedirs(out_dir, exist_ok=True)
    with open(os.path.join(out_dir, "Strings.json"), "w", encoding="utf-8") as f:
        json.dump({"string": dict(sorted(strings.items()))}, f, ensure_ascii=False, indent="\t")
        f.write("\n")

    # 2) 英文模板（原版默认值 + 新增键）
    english = dict(EN_DEFAULTS)
    english.update(EXTRA_EN)
    with open(os.path.join(out_dir, "Strings.en.json"), "w", encoding="utf-8") as f:
        json.dump({"string": dict(sorted(english.items()))}, f, ensure_ascii=False, indent="\t")
        f.write("\n")


    # 4) 键表文档
    lines = ["# ObituarySKSE localization", "",
             "All UI text lives in one file; do not rename keys:", "",
             "- `SKSE\\Plugins\\ObituarySKSE\\Strings.json` - runtime text for the plugin and the F1 menu",
             "  (Simplified Chinese by default)",
             "- `SKSE\\Plugins\\ObituarySKSE\\Strings.en.json` - English reference with the same keys", "",
             "## Translating", "",
             "1. Edit the values in `Strings.json` (keep `%s` `%d` placeholders and `\\n`);",
             "2. `Strings.en.json` is only a reference template - replace its values if you want English;",
             "3. `tools\\localization.py` merges the original mod's strings with the keys this plugin adds",
             "   and regenerates `Strings.json` / `Strings.en.json` / this table.", "",
             "## Key table", "", "| Key | Current value |", "|---|---|"]
    for k in sorted(english):
        v = english[k].replace("\n", "\\n").replace("|", "\\|")
        lines.append(f"| `{k}` | {v} |")
    with open(os.path.join(args.out, "LOCALIZATION.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")

    print(f"写入: {out_dir}\\Strings.json, Strings.en.json, LOCALIZATION.md")


if __name__ == "__main__":
    sys.exit(main())