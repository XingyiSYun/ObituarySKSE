# external/

## SKSEMenuFramework.h（不随仓库分发）

这个头文件属于第三方（SKSE Menu Framework），本仓库**不再包含它**。

需要自己编译带 F1 菜单的版本时：

1. 从 <https://github.com/QTR-Modding/SKSE-Menu-Framework-3> 下载
   `resources/SKSEMenuFramework.h`
2. 放到 `external/SKSEMenuFramework.h`
3. 重新运行 `build.ps1`

没有这个头文件也能编译通过——`src/Menu.cpp` 用 `__has_include` 做了守卫，
会编译出一个「无 F1 菜单」的 DLL（设置只能改 `settings.ini`，其余功能正常）。
日志里会打：`built without SKSEMenuFramework.h - F1 menu disabled`。
