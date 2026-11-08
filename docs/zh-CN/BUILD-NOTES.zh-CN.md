# ObituaryRemake —— SKSE 最小工程构建记录

> 目标：为后续项目提供一份**离线、可复现**的 SKSE 插件骨架（C++ / CommonLibSSE-NG）。
> 运行时定位：Skyrim SE/AE **1.7.104.0**，SKSE **2.3.1**。
> 本记录只描述事实（实测命令、展开后的路径、失败与规避方式），未改动 E: 盘任何游戏/模组目录。

## 1. 结论

| 项目 | 结果 |
|---|---|
| 编译 | **成功**（2026-11-08 14:25，全离线；/W4 + /permissive- 下 0 warning / 0 error） |
| 产物 | `D:\工作区\上古卷轴5工作区\ObituarySKSE\build-vs\Release\ObituaryRemake.dll` |
| 大小 | **545,792 字节** |
| SHA256 | `54644D622594B4731C2C4B3752A8E0AC941F8AB54E6E138537EF20C4E90F7002` |
| 生成器 | **Visual Studio 18 2026**（不是 Ninja） |
| 平台工具集 | **v145 / MSVC 14.51.36231**（cl 19.51.36260.0） |
| 架构 | x64（`8664 machine (x64)`） |
| 导出 | `SKSEPlugin_Load`、`SKSEPlugin_Query`、`SKSEPlugin_Version` |

首次运行即编译通过（第二次是删掉 `build-vs` 后的全量重建，同样通过，故非缓存巧合）。

## 2. 目录结构

```
ObituarySKSE\
├── CMakeLists.txt          # 工程定义：直接吃本地 vcpkg-install 树
├── build.ps1               # 一键：载入 VS 环境 → cmake -G 生成 → cmake --build Release
├── src\plugin.cpp          # 插件声明 + 消息回调 + 一条加载日志
├── BUILD-NOTES.md          # 本文件
└── build-vs\               # 生成目录（VS 生成器，多配置，可整体删除）
    └── Release\
        ├── ObituaryRemake.dll   ← 唯一需要的产物
        ├── ObituaryRemake.lib   ← MSBuild 副产物（导入库，SKSE 插件不需要）
        └── ObituaryRemake.exp   ← MSBuild 副产物
```

## 3. 一键编译（推荐）

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "D:\工作区\上古卷轴5工作区\ObituarySKSE\build.ps1"
```

注意：**本机没有 `pwsh`**（PowerShell 7），只有 `powershell.exe`（5.1）。`build.ps1` 有意保持纯 ASCII，
因为 Windows PowerShell 5.1 会按 ANSI 读取 `.ps1`，中文注释会被解成乱码。

脚本实际做的事（与 `build-env\build.ps1` 同款模式）：

1. `cmd /c "vcvars64.bat -vcvars_ver=14.51.36231 >nul && set"` 导入 MSVC 环境（14.51 不存在时自动退回默认工具集）；
2. `C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`
   `-S <src> -B <src>\build-vs -G "Visual Studio 18 2026" -A x64 -DCMAKE_PREFIX_PATH=<prefix> -DCOMMONLIBSSE_PREFIX=<prefix>`
3. `cmake --build <build> --config Release --parallel`
4. 校验 `build-vs\Release\ObituaryRemake.dll` 存在并打印大小。

### 手动等价命令

```powershell
$cmake = 'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$src   = 'D:\工作区\上古卷轴5工作区\ObituarySKSE'
$build = "$src\build-vs"

& $cmake -S $src -B $build -G "Visual Studio 18 2026" -A x64
& $cmake --build $build --config Release --parallel
```

（`COMMONLIBSSE_PREFIX` 默认由 `CMakeLists.txt` 相对源码目录推出，所以手动跑不传 `-D` 也能配置；
显式传参只是让日志里的路径一目了然。）

## 4. CMake 引用的绝对路径（实测展开值）

`COMMONLIBSSE_PREFIX` = `D:\工作区\上古卷轴5工作区\vcpkg-install\x64-windows-static-md`

| 用途 | 绝对路径 |
|---|---|
| 头文件根 | `D:\工作区\上古卷轴5工作区\vcpkg-install\x64-windows-static-md\include` |
| DirectXTK 头 | `...\vcpkg-install\x64-windows-static-md\include\directxtk` |
| DirectXMath 头 | `...\vcpkg-install\x64-windows-static-md\include\directxmath` |
| CMake 包配置 | `...\vcpkg-install\x64-windows-static-md\share\CommonLibSSE\CommonLibSSEConfig.cmake` |
| 主库 | `...\vcpkg-install\x64-windows-static-md\lib\CommonLibSSE.lib` |
| 依赖库（随 `CommonLibSSE::CommonLibSSE` 的 link interface 自动带上） | `...\lib\spdlog.lib`、`...\lib\fmt.lib`、`...\lib\DirectXTK.lib` |
| 系统库 | `advapi32 bcrypt D3D11 d3dcompiler dbghelp DXGI ole32 version` + CMake 默认的 `kernel32 user32 gdi32 winspool shell32 oleaut32 uuid comdlg32` |
| **外部硬编码依赖（不在 install 树里）** | `D:\VS\vcpkg\buildtrees\commonlibsse-ng\src\53794cf185-7be62b72f5.clean\extern\openvr\lib\win64\openvr_api.lib` |

上面最后一行是 **CommonLibSSE-NG 导出的 CMake 配置自己写死的**，不是本项目添加的。
它落在 `buildtrees`（而非 install 树）里，所以 **`D:\VS\vcpkg\buildtrees` 不能删/不能搬**，
否则链接期报 `LNK1104: 无法打开文件 ...openvr_api.lib`。
本项目在 `CMakeLists.txt` 里加了检测：文件不存在时配置阶段会给出明确 WARNING 并提示补救办法。

## 5. 工程实现要点

### 5.1 声明（新式 `SKSE::PluginDeclaration`）

`src\plugin.cpp` 里手写展开（等价于 `SKSEPluginInfo(...)` 宏，但字段可见、好改）：

```cpp
SKSE_EXPORT constinit SKSE::PluginDeclaration SKSEPlugin_Version({
    .Version = REL::Version{ 0, 1, 0, 0 },
    .Name = "ObituaryRemake"sv,
    .Author = "Obituary Remake Team"sv,
    .SupportEmail = ""sv,
    .StructCompatibility = SKSE::StructCompatibility::Independent,
    .RuntimeCompatibility = {
        REL::Version{ 1, 5, 97, 0 },      // SE
        REL::Version{ 1, 6, 1170, 0 },    // AE
        REL::Version{ 1, 7, 104, 0 },     // AE（本项目主目标）
    },
    .MinimumSKSEVersion = REL::Version{ 0, 0, 0, 0 },
});

static_assert(std::is_same_v<std::remove_cvref_t<decltype(SKSEPlugin_Version)>,
    SKSE::PluginDeclaration>);
```

- `SKSE::PluginDeclaration` 是 CommonLibSSE-NG 的高层声明类型；`SKSE::PluginVersionData` 是同一个
  `SKSEPlugin_Version` 导出背后的**裸 ABI 结构**（`SKSE/Interfaces.h` 里 `sizeof == 0x350`）。
  本工程用的是前者，`SKSEPluginInfo(...)` 宏展开后生成的也是前者。
- 同样手写了 `SKSEPlugin_Query`（SKSE 仍会调用，用于登记插件名/版本）。

### 5.2 加载与日志

```cpp
SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    SKSE::Init(a_skse);
    SKSE::log::init();   // → %Documents%\My Games\Skyrim Special Edition\SKSE\ObituaryRemake.log
    ...
}
```

- 用 SKSE 自带日志（`SKSE::log::init()` + `SKSE::log::info`），不额外造文件 sink；
  日志文件名取自插件声明里的 `Name`。
- 加载时打三条：插件名/版本/作者、运行时版本、`load finished`。
- 注册 `SKSE::MessagingInterface` 监听，处理 `kPostLoad`、`kPostPostLoad`、`kInputLoaded`、
  `kDataLoaded`（`kPostLoad` 用 `a_msg->sender` 记录来源）。
- 没做 `SKSE::AllocTrampoline`（骨架里没有 hook，加了反而浪费代码池）。

### 5.3 CMake 关键点

- `project(ObituaryRemake ...)`，`add_library(... SHARED src/plugin.cpp)`，`OUTPUT_NAME ObituaryRemake` → `ObituaryRemake.dll`。
- `CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL`：与 vcpkg `x64-windows-static-md` 三元组对齐（动态 CRT + 静态依赖），
  与同机已验证的 TMS_RMHeelFix 一致。
- `CMAKE_MAP_IMPORTED_CONFIG_DEBUG/RELWITHDEBINFO/MINSIZEREL → Release`：install 树里只有 Release 变体。
- `find_package(CommonLibSSE CONFIG REQUIRED)` 之前：把 prefix 前置进 `CMAKE_PREFIX_PATH`
  （配置里的 `find_dependency(spdlog CONFIG)` 靠它解析），并手工补一个 `Microsoft::DirectXTK` IMPORTED 目标（见坑 1）。
- `target_compile_features(... cxx_std_23)`，见坑 3。
- 定义：`_UNICODE UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _ITERATOR_DEBUG_LEVEL=0`；
  NG 的接口再补 `WINVER/_WIN32_WINNT=0x0601`、`ENABLE_SKYRIM_SE/AE/VR`、`SKSE_SUPPORT_XBYAK` 等。
- 选项：`/W4 /permissive- /Zc:preprocessor`。**不加 `/utf-8`**（坑 4）。
- 没有 PCH：骨架只有一个 TU，加 PCH 只增加编码风险。

## 6. 验证结果（dumpbin 实测）

导出：

```
ordinal hint RVA      name
1    0    0001C740 SKSEPlugin_Load
2    1    0001CC60 SKSEPlugin_Query
3    2    0007F000 SKSEPlugin_Version
```

导入（`/dependents`）只有 UCRT + 系统 DLL，没有 fmt/spdlog/eastl 等额外 DLL，
说明 `x64-windows-static-md` 的静态依赖确实被链进去了：

```
api-ms-win-crt-locale/string/runtime/convert/stdio/filesystem/time, ole32.dll, VERSION.dll,
USER32.dll, SHELL32.dll
```

导出声明对象的字节（文件偏移 `0x7D000`，即 `SKSEPlugin_Version` 的 RVA `0x7F000`）：

| 字段 | 值 |
|---|---|
| `dataVersion` | `1` |
| `pluginVersion` | `0x00010000`（= 0.1.0.0） |
| `name` | `ObituaryRemake` |
| `StructCompatibility` | `3`（Independent：AddressLibraryV5 \| NoStructUse） |
| `RuntimeCompatibility` 位域 | `0`（给了显式白名单 → 不声明"用 Address Library"） |
| `compatibleVersions` | `0x01050610`(1.5.97.0)、`0x01064920`(1.6.1170.0)、`0x01070680`(1.7.104.0)、`0x01000000`(填充默认 1.0.0.0) |
| `xseMinimum` | `0` |

> 这些打包值是 NG 的 `REL::Version::pack()` 结果（`major<<24 | minor<<16 | patch<<4 | build`），
> 也就是 `PluginVersionData::CompatibleVersions` 会写进去的同一套编码。**SKSE 是否接受这个白名单
> 只能在进游戏时看 `skse64.log` 才能最终确认**（见第 7 节第 9 条）。

## 7. 踩到的坑与已知限制

1. **`Microsoft::DirectXTK` 目标缺失（会直接让 generate 失败）**
   NG 的导出配置在 `INTERFACE_LINK_LIBRARIES` 里列了 `Microsoft::DirectXTK`，但 `CommonLibSSEConfig.cmake`
   不 `find_package` 它；而 DirectXTK 自己的 config 又要 `xaudio2redist`（install 树里没有）。
   解法：手工 `add_library(Microsoft::DirectXTK STATIC IMPORTED GLOBAL)`，头/库都指向**同一棵 install 树**
   （`include\directxtk`、`include\directxmath`、`lib\DirectXTK.lib`），不依赖 `D:\VS\vcpkg\packages`。
   多配置生成器必须给每个配置都设 `IMPORTED_LOCATION_<CFG>`，否则 MSBuild 找不到库。

2. **`openvr_api.lib` 被硬编码到 `buildtrees`**
   见第 4 节表格最后一行。**别删 `D:\VS\vcpkg\buildtrees`**。`CMakeLists.txt` 已加检测 warning。

3. **C++20 在这套 NG 上做不到**
   NG 的导出目标带 `INTERFACE_COMPILE_FEATURES "cxx_std_23"`，CMake 即使收到 `cxx_std_20` 也会抬到 23。
   最终 vcxproj 里是 `<LanguageStandard>stdcpplatest</LanguageStandard>`（v145 没有 `/std:c++23` 开关名）。
   所以 `CMakeLists.txt` 直接写 `cxx_std_23`，避免"看着是 20 实际是 23"的误解。

4. **不要 `/utf-8`**
   源码树在中文路径下，CMake/MSBuild 生成的中转文件按系统代码页编码，加 `/utf-8` 会撞 C4828。
   本项目还干脆不用 PCH（`TMS_RMHeelsFix` 当初就是被"生成的 GBK PCH + /utf-8"坑过）。

5. **必须 MSBuild/Visual Studio 生成器**
   本机 Ninja 生成器会卡死在 `mspdbsrv`（管道 EOF 不返回），这是已知环境事实，不要试。
   另外本工程**没有** `CMakePresets.json`：那套走 vcpkg 清单模式，会联网拉 registry 并重编 NG。

6. **不要 vcpkg 清单模式 / 不要网络**
   全程只用 `find_package(CommonLibSSE CONFIG)` 指向本地 install 树；configure 8 秒完成，无任何下载。
   注意 `CMakePresets.json`（TMS 里那份）虽然存在，但**不要用**。

7. **`.ps1` 必须 ASCII，且本机没有 `pwsh`**
   Windows PowerShell 5.1 按 ANSI 读脚本；本机只有 `powershell.exe`。`build.ps1` 里所有提示都是英文，
   路径全部由 `$PSScriptRoot` 推导（运行期变量可以是中文，文件内容不行）。

8. **CRT 选择：动态 `/MD` + 静态依赖（`static-md`）**
   DLL 导入 UCRT（`api-ms-win-crt-*`），要求 Windows 10+；对 1.7.104 的机器不是问题。
   与 SKSE 插件生态、以及同机已验证的 TMS_RMHeelFix 保持一致。**不要**改成 `/MT`：NG 预编译库是 static-md。

9. **运行期尚未验证（最大的一条限制）**
   本任务只要求"编译通过"，没有进游戏（也不该动 E 盘）。因此：
   - SKSE 是否接受 `compatibleVersions` 里的 1.7.104.0 白名单，只能在
     `Documents\My Games\Skyrim Special Edition\SKSE\skse64.log` 里看
     `plugin ObituaryRemake.dll ... loaded correctly` 才算证实；
   - 若被判定 incompatible，先核对第 6 节的 `compatibleVersions` 打包值；
   - 日志文件 `Documents\My Games\Skyrim Special Edition\SKSE\ObituaryRemake.log` 需插件真正加载后才会出现。
   本机没有 SKSE SDK 头（`PluginAPI.h`/`plugin_version.h` 搜不到），所以上述编码无法离线二次核对。

10. **缺 SKSE 头？不缺。**
    CommonLibSSE-NG 自带 `include\SKSE`（`Interfaces.h`、`API.h`、`Logger.h`、`Events.h` 等），
    本工程不需要外部 SKSE SDK，也不需要 `skse64_*.lib`（SKSE 插件是反向导入游戏/宿主提供的接口）。

11. **版本号是两处**
    `project(ObituaryRemake VERSION 0.1.0)` 只影响 CMake 变量；插件声明的版本号写在 `src\plugin.cpp`
    的 `.Version` 里，**改版本时要两处一起改**（TMS 是用 `string(CONFIGURE)` 生成
    `PluginMetadata.cpp` 来同步的，本项目为了骨架可读性故意没这么做）。

12. **`/W4` 只作用于本项目代码**
    vcpkg install 树的 include 被 CMake 当 SYSTEM（`/external:I`），且 vcxproj 里
    `ExternalWarningLevel = TurnOffAllWarnings`，所以 NG/EASTL 的警告不会淹没自己的警告。

13. **单 TU 编译较慢**
    `RE/Skyrim.h` 体量大，全量重建约 1 分钟量级（configure 约 8 秒）。增量构建几乎瞬时。
    后续源文件多了再考虑 PCH（届时务必保持不 `/utf-8`）。

## 8. 后续扩展指引

- **加源文件**：在 `CMakeLists.txt` 的 `add_library(${PROJECT_NAME} SHARED ...)` 列表里追加 `.cpp`；
  需要的话加 `target_include_directories(${PROJECT_NAME} PRIVATE src)`。
- **加 hook**：`SKSE::AllocTrampoline(1 << 8)` 或按需分配代码池，再 `REL::Relocation`/`SKSE::GetTrampoline()`。
- **要菜单**：放 `SKSEMenuFramework.h` 到工程里（参考 TMS 的 `__has_include` 可选模式）。
- **部署**：把 `build-vs\Release\ObituaryRemake.dll` 复制到
  `<Skyrim Data>\SKSE\Plugins\`（本任务未执行，E 盘未触碰）。
- 建议对接 `build-env\build.ps1` 的统一入口时，把本工程当作第二个 target 或直接调用本目录的 `build.ps1`。

## 9. 参考

- `D:\工作区\上古卷轴5工作区\TMS_RMHeelsFix\CMakeLists.txt`（模式来源：DirectXTK 补丁、无 `/utf-8`、`MSVC` 检查）
- `D:\工作区\上古卷轴5工作区\build-env\build.ps1`（模式来源：`vcvars64` 环境导入 + VS 生成器 + `--config Release`）
- `D:\工作区\上古卷轴5工作区\build-env\移植记录.md`（Ninja 卡死、manifest 联网、`/utf-8`、SKSE 版本白名单等环境结论）
