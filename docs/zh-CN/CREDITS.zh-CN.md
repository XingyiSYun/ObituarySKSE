# 第三方组件与来源

本项目是**独立重写**：没有复制原版 Obituary 的 Papyrus 脚本或 esp 记录，行为规格来自对原版
`.psc` 源码的阅读，UI 文本键名与原版 `Strings.json` 保持一致以便复用现有译文。

| 组件 | 位置 | 来源 | 许可 |
|---|---|---|---|
| `SKSEMenuFramework.h` | `external/` | [QTR-Modding/SKSE-Menu-Framework-3](https://github.com/QTR-Modding/SKSE-Menu-Framework-3) 的 `resources/SKSEMenuFramework.h`，原样引入供编译期使用 | 按上游仓库声明；仅作为头文件被引用，未修改 |
| `Strings.json` / `Strings.en.json` | `mod/SKSE/Plugins/ObituarySKSE/` | 文本键名沿用原版 Obituary（作者 asdasfa）的 `Strings.json`；中文译文来自该模组的简体中文语言文件，英文取自原版脚本内置默认值 | 归原作者；本项目仅沿用文字，不包含其代码 |
| Obituary 行为规格 | `docs/` | 阅读原版 `_asd_obituaryquestscript.psc` 等三个脚本后重写的行为描述 | 文档为本项目原创 |
| CommonLibSSE-NG | 构建依赖（不随仓库分发） | [CharmedBaryon/CommonLibSSE-NG](https://github.com/CharmedBaryon/CommonLibSSE-NG) | Apache-2.0 |
| SKSE | 运行依赖（不随仓库分发） | ianpatt / behippo | 见 skse.silverlock.org |

## 关于原版 mod

- 原版 Obituary：Nexus Mods，作者 **asdasfa**。本项目不是它的 fork，也不包含它的 esp 或 pex。
- 如果你要二次分发本项目的 `Strings.json`，建议在描述里注明文本源自原版 Obituary。

## 关于 `SKSEMenuFramework.h`

该头文件 511 KB，是菜单框架提供给插件开发者的 SDK 头文件（通过 `GetProcAddress` 绑定运行时），
本身不含游戏资源。若上游修改了许可或你不想再分发它，可以：

1. 删除 `external/SKSEMenuFramework.h`；
2. 把 `CMakeLists.txt` 里对该头文件的依赖设为可选（`__has_include` 守卫，未找到时编译“无 F1 菜单版”）；
3. 或改为在 README 里指引用户自行从上游下载并放到 `external/`。
