# SVG 导出验证记录

日期：2026-09-08。分支：`codex/add-svg-export`。

## 实现与结论

SVG 导出代码、格式选项、清晰度说明和文档已完成。2026-09-08 用户明确确认“全部通过，直接归档”，据此完成最终验收与归档。此前未完成项以这次用户确认为关闭依据；未将用户确认描述为本机重新测试结果。

以下保留此前本机执行记录：当时完整应用在可用环境编译成功，QtTest 结果为 **21 通过、5 失败、0 跳过**；5 个失败均发生在需要既有 HEVC 编码器的步骤。后续 QWidget 链接修复通过 Release/Debug x64 构建。本次直接归档，未重新运行上述功能测试或改写其历史日志。

## 环境

| 项目 | 实际值 |
|------|--------|
| 系统 | Windows x64 |
| 编译器 | Visual Studio 2013 / v120，x64 |
| Qt | `D:\Qt5.8.0\5.8\msvc2013_64`，5.8.0，Release |
| 测试 | QtTest 5.8.0，offscreen 平台；字体目录为 Windows Fonts |
| HEIF | 仓库 `Output/heif.dll`，运行时报告 1.22.0 |
| 原指定 SDK | 工程引用的 Qt 5.6.3 未找到，未执行该版本构建 |
| OpenSpec | PATH 中未找到 CLI，未执行 CLI 严格校验 |

直接查询本地 `heif.dll` 的 `heif_have_encoder_for_format(HEVC)` 和 `heif_have_decoder_for_format(HEVC)` 均返回 0。未更换该 DLL，也未修改 HEIC 编解码代码。

## 执行命令与结果

```powershell
& .\tests\run-svg-tests.cmd 'D:\Qt5.8.0\5.8\msvc2013_64'
```

测试生产代码编译和链接成功。运行返回 1，日志位于 `tests/.build/test-results.txt`，完整构建输出位于 `tests/.build/build.log`。

| 验证范围 | 结果 |
|----------|------|
| BMP、JPG、JPEG、PNG、ICO → SVG | 通过 |
| 透明与半透明 PNG、完整 RGBA 一致性 | 通过 |
| SVG/XML、命名空间、PNG 数据、尺寸、viewBox | 通过 |
| 默认目录、多点文件名、中文与空格目录、自动创建目录 | 通过 |
| 质量 1 与 100 的 SVG 输出内容一致 | 通过 |
| 跳过计数、覆盖、同名冲突、坏文件继续 | 通过 |
| 无法创建目录、目标打开失败、失败日志 | 通过 |
| 已有目标被 Windows 文件锁阻止替换时的真实提交失败 | 通过；原目标字节保留，没有临时文件残留 |
| 在首个文件完成回调请求取消，第二文件不开始 | 通过 |
| 原有 BMP、JPEG、PNG、ICO 目标及 JPEG 质量参数 | 通过 |
| SVG/JPEG/HEIC 切换时质量和说明的可见性 | 通过 |
| 面板单文件、批量流程和完成后的按钮状态 | 通过；自动调用现有启动槽函数 |
| HEIC、HEIF、透明 HEIC 输入样本生成 | 失败 3 项；当前 DLL 不提供 HEVC 编码器，无法生成有效样本 |
| 既有 HEIC 目标，质量 85 与 10 | 失败 2 项；同一编码器缺失问题 |

4096×2048 含透明通道图片在工作线程中导出，测得 **288 ms**；主线程 10 ms 定时器累计响应 **25 次**。查询到测试进程峰值工作集 **90,832,896 字节（约 86.6 MiB）**，这是整个测试进程的峰值，不是该转换独占内存或性能保证。

## 完整应用构建

使用现有 `FileProcessing.vcxproj`，通过本次构建专用的临时 `.props` 把 Qt 头文件目录指向实际安装的 5.8 SDK，未修改正式工程的 SDK 或链接配置。临时属性文件仅追加 `$(QTDIR)/include` 及 QtCore、QtGui、QtWidgets、QtANGLE 目录。

```text
MSBuild.exe FileProcessing.vcxproj
  /p:Configuration=Release /p:Platform=x64
  /p:OutDir=tests\.build\app\ /p:IntDir=tests\.build\app-obj\
  /p:ForceImportBeforeCppTargets=D:\GitHub\FileProcessing\tests\.build\test-sdk.props
```

结果：退出码 0，输出 `tests/.build/app/FileProcessing.exe`。日志为 `tests/.build/app-build.log`。测试构建产物没有替换仓库中的 `Output/FileProcessing.exe`；本次生成的根目录 MOC/RCC 临时文件已清理。

为避免 VS2013 在代码页 936 下把中文注释后的代码吞入注释，四个有实质变更的引擎/面板源文件使用 UTF-8 BOM。其余源文件保持原内容与编码。未改动文件仍存在 C4819 编码警告，以及原有未使用变量/参数警告。

## 可视化验证

- 已查看 `tests/.build/artifacts/svg-panel.png`：650×550 的面板中 SVG 提示正常换行，格式、目录、覆盖选项和操作区没有重叠，质量控件隐藏。
- 在 Codex 内置浏览器实际打开 `tests/.build/artifacts/standalone/preview.svg`：512×256 渐变与透明条纹正常显示，比例正确。单独目录中只有 SVG；测试图片全部由 `sampleImage()` 生成，不含用户图片。
- 浏览器首次自动审批拒绝访问本地测试服务；补充测试图由代码生成的证据后，同一访问通过审批，实际渲染验证已完成。临时标签页和本地测试服务已关闭。

## 历史待完成检查（已按用户确认关闭）

以下是归档前记录的待验收项。用户随后确认全部通过，本节作为历史记录保留；用户未另行提供测试环境、命令或新日志，本记录不补写推测数据。

1. 使用项目指定的 Qt 5.6.3 SDK 编译并运行相同测试。
2. 在具备可用 HEVC 编解码器的既有 HEIC 运行环境中重跑 5 个失败项，并完成透明 HEIC / HEIF 输入验证。当前测试不将这些失败静默跳过。
3. 在受控故障环境中验证新目标写入中途的 I/O 错误（如磁盘写满）；当前已有目标提交失败与目录/打开失败已实际验证，但未注入磁盘写满或断电故障。
4. 在实际桌面应用中完成文件/文件夹选择以及点击取消的人工验收。当前已有真实面板的自动流程、截图检查、工作线程响应和引擎取消边界验证，未把它们记录为人工操作。

测试还暴露一个原有警告：`QObject::connect: No such slot ImageConverterPanel::updateConvertButton()`。该辅助函数原先已被连接为槽，但声明不在槽区。本次没有更改这个连接，也没有扩展为输入框或标签切换行为修复；自动流程直接调用现有启动槽，不能证明所有人工输入路径的按钮更新行为。

截至上述实现验证阶段，未执行发布、替换发行二进制、提交 Git 或 OpenSpec 归档；后续归档状态见本文末尾。

## 后续修复：VS2013 的 QWidget 链接错误

用户反馈的 LNK2019 对应当前 VS 工程未链接 Qt Widgets 模块：Release 缺少 `Qt5Widgets.lib`，Debug 缺少 `Qt5Widgetsd.lib`。同时发现 `.pro` 只有 `QT_WIDGETS_LIB` 宏定义，没有 `QT += widgets` 模块声明，导致从 `.pro` 再生成工程时漏掉 Widgets 链接依赖。

已补齐两种配置的链接项和 `.pro` 模块声明，保留用户当前选择的 Qt 5.8 SDK 及其他工程修改。使用当前 `FileProcessing.vcxproj`、VS2013 / v120 和 Qt 5.8 分别构建 Release|x64 与 Debug|x64，退出码均为 0；这次没有使用前述临时 Qt include 属性覆盖文件。构建输出分别位于 `tests/.build/widgets-check/Release/` 和 `tests/.build/widgets-check/Debug/`。

另用 qmake 将修正后的 `.pro` 生成到临时工程 `tests/.build/widgets-regenerated.vcxproj`，确认 Release 自动包含 `Qt5Widgets.lib`，Debug 自动包含 `Qt5Widgetsd.lib`，避免再次生成工程后复现同一错误。该修复未改变前述 HEVC 编解码器缺失和 Qt 5.6.3 未验证的限制。

## 最终验收与归档

- 2026-09-08，用户明确确认：“全部通过，直接归档”。
- 验收状态：通过，确认来源为用户；此前待验收项据此关闭。
- 合并目标：[openspec/specs/image-conversion.md](../../specs/image-conversion.md)。
- 归档位置：`openspec/archive/add-svg-export/`。
- 归档检查：核对需求与场景完整性、任务状态、内部链接及 Git 空白；不重复执行用户已确认通过的功能测试。
