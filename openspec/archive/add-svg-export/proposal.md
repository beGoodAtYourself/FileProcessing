# Proposal: 图片格式转换新增 SVG 导出

**Change ID:** `add-svg-export`

**Created:** 2026-09-08

**Status:** Archived

**Implementation Date:** 2026-09-08

**Acceptance Date:** 2026-09-08（用户确认全部通过）

**输出方式：** 已由用户确认，采用内嵌位图的 SVG，保留原图外观，放大清晰度受原图分辨率限制。

## Problem Statement

当前图片转换引擎和界面只提供 BMP、JPEG、PNG、ICO、HEIC 五个目标格式。用户无法从已有单文件或批量流程中直接输出 `.svg` 文件，需要借助其他工具完成封装。

本次需求是把现有可解码的图片导出为可独立使用的 SVG 文件。保真范围以当前解码流程得到的 QImage 为基准，不额外改变 EXIF 方向处理、色彩管理、ICO 图层选择或 HEIC 解码行为。

## Proposed Solution

1. 在目标格式列表末尾追加 `SVG`，保持 JPEG 默认选项和现有枚举值，追加 `FORMAT_SVG = 5`，补全显示名和 `.svg` 扩展名映射。
2. 复用现有图片解码路径；在 `ImageConverterEngine::convertSingleFile()` 的目标编码阶段增加 SVG 分支。
3. 将解码后的图片无损编码为内存 PNG，再以 Base64 data URI 内嵌到 SVG 的 `<image>` 元素中。SVG 的宽、高和 `viewBox` 与解码后的像素尺寸一致，保留透明通道。
4. SVG 选择时隐藏 JPEG/HEIC 质量设置，并显示清晰度说明；SVG 编码不受之前设置的质量值影响。
5. 沿用现有工作线程、单文件与批量入口、输出命名、覆盖选项、进度、日志和文件边界取消机制。

SVG 的 `<image>` 元素可承载 PNG 图片，并通过宽高与坐标控制显示区域，符合本次保留位图外观的目标。参见 [W3C SVG 1.1：image 元素](https://www.w3.org/TR/SVG11/struct.html#ImageElement)。

## Scope

### In Scope

- BMP、JPEG（jpg/jpeg）、PNG、ICO、HEIC/HEIF 输入导出为单个 `.svg` 文件，具体解码能力沿用已有运行环境。
- 单文件转换、添加多个文件、从文件夹添加图片后的批量转换。
- 输出 SVG 自包含，不依赖原图片路径、旁置图片文件或网络资源。
- 保留解码图片的尺寸、像素内容与 Alpha，不增加背景填充、缩放或二次有损压缩。
- 记录解码、PNG 编码、目录创建、SVG 写入及提交失败，并继续处理剩余批量任务。
- SVG 写入失败时不留下不完整的新目标文件；覆盖失败时保留已有目标文件。
- 更新 README、相关源码注释及实现完成时的版本记录，清楚描述输入与输出能力。

### Out of Scope

- 图片描摹、轮廓提取、生成可编辑矢量路径或提升放大清晰度。
- 新增 SVG 输入、SVG 转其他格式、SVG 编辑和预览功能。
- 缩放、裁剪、动画、多帧导出、元数据或色彩管理改造。
- 重构现有跳过统计、取消状态文案、线程回调架构或同名冲突策略。
- 升级 Qt / 编译器、引入 QtSvg 模块、外部转换程序或新的运行时 DLL。

## Impact Analysis

| Component | Change Required | Details |
|-----------|-----------------|---------|
| 数据库 / 外部 API | 否 | 无相关存储或服务接口 |
| `ImageConverterEngine.h/.cpp` | 是 | 新枚举、目标名称与扩展名映射、独立 SVG 保存分支及失败日志 |
| `ImageConverterPanel.h/.cpp` | 是 | 目标格式项、SVG 清晰度提示及格式切换时的可见性 |
| `ImageConverterWorker` | 验证 | 现有枚举传递、进度和结果接口可复用，无需新增业务状态 |
| `HeicDecoder` / `HeicEncoder` | 否 | 复用现有 HEIC 解码结果，既有 HEIC 输出逻辑保持原样 |
| Visual Studio / qmake 工程 | 是 | 补齐 `QT += widgets` 及 Release/Debug 的 Qt Widgets 链接项，确保重新生成工程后仍能正确链接 |
| 文档与验证 | 是 | 说明 SVG 仅为新增输出格式，并覆盖独立打开和图像一致性 |

## Architecture Considerations

- 以私有辅助函数封装 SVG 保存逻辑，由引擎调用；不把 XML 拼接、图片编码或文件写入放到 UI 中。
- 拟使用 `QBuffer` 保存 PNG、`QByteArray::toBase64()` 生成数据、`QXmlStreamWriter` 输出 UTF-8 XML。实现时按 Qt 5.6 SDK 验证 API 可用性，避免引用新版独有方法。
- 根元素声明 SVG 命名空间与 XLink 命名空间，使用 `version="1.1"`、`width="W"`、`height="H"`、`viewBox="0 0 W H"`；`<image>` 位于 `(0, 0)`，宽高为 `W`、`H`，`xlink:href` 为 `data:image/png;base64,...`。
- 使用 `QSaveFile` 临时写入并提交，保持直接覆盖回退关闭；所有编码和写入检查成功且提交成功后才返回成功。不得先删除已有目标文件。
- PNG 保存须显式指定格式且不传入 JPEG/HEIC 的质量参数；检查空图片、编码结果和设备写入错误。
- `.svg` 不加入源文件选择和文件夹扫描过滤器；目标枚举扩展不意味着新增 SVG 输入支持。`detectFormat()` 无需为本次输出能力扩展输入识别范围。
- 不改变现有覆盖和计数契约：跳过文件有日志和结果记录，但不计入引擎返回的实际转换成功数。同名源文件映射到同一目标时，按现有列表顺序与覆盖选项处理。
- 取消仍在文件边界生效：当前文件可完成保存，后续文件在取消检查处停止；不承诺中断一次 PNG 编码或磁盘提交。

## Success Criteria

- [x] 单文件和批量界面均可选择 SVG，输出命名为原基本名加 `.svg`。
- [x] 所有现有输入格式的有效样本均可输出 SVG；透明 PNG 和可用的透明 HEIC 样本保留 Alpha。
- [x] XML 可解析，内嵌数据可独立解码为 PNG，尺寸和 RGBA 像素与引擎解码结果一致。
- [x] 将 SVG 单独复制到其他目录后，可在支持 SVG 的浏览器中打开，外观和比例正确。
- [x] SVG 不显示质量设置，并显示位图清晰度限制；切回 JPEG/HEIC 恢复质量控件。
- [x] 输出目录、已有文件跳过/覆盖、同名冲突、失败继续及取消行为符合增量规范。
- [x] 写入失败不留下不完整 SVG，也不破坏已有目标文件。
- [x] 现有五种目标格式的相关回归检查通过；按实际可用的 Qt 5.6 / v120 x64 环境完成构建验证并记录限制。

## Risks & Mitigations

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| 用户将 SVG 输出理解为自动矢量化 | 中 | 高 | 输出方式已确认，并在 UI 与 README 中说明放大限制 |
| 大图片同时占用解码、PNG 和 Base64 缓冲，文件体积增加 | 高 | 中 | 逐文件处理，及时释放缓冲；使用较大图片验证内存占用与界面响应，不承诺压缩体积 |
| 写入或覆盖中途失败 | 中 | 高 | 临时写入、检查错误、成功后提交；验证已有目标保持完整 |
| 阅读器兼容性差异 | 中 | 中 | 使用简单 SVG 1.1 文档和 PNG 内嵌数据，进行 XML、数据解码和浏览器实际打开检查 |
| 旧版编译器、Qt 或 HEIC 运行环境不齐全 | 中 | 中 | 基于实际 SDK 验证；区分环境缺失与功能失败，记录未执行项，不宣称验证通过 |

## Review & Validation

- 实现任务见 [tasks.md](tasks.md)，行为与验收场景见 [image-conversion_delta.md](specs/image-conversion_delta.md)。
- 项目此前没有 OpenSpec 正式规范，本次需求全部记为新增需求。
- 历次执行记录与最终验收确认见 [validation.md](validation.md)。此前本机测试结果为 21 项通过、5 项 HEIC 环境相关失败；之后 Release/Debug x64 构建验证通过。2026-09-08 用户明确确认“全部通过，直接归档”，据此关闭此前剩余验收项并完成归档。历史日志保留原始结果，本次未重新执行功能测试。
- 当前环境 PATH 未发现 OpenSpec CLI；使用文档结构、场景完整性、内部链接及 Git 空白检查验证，不将其表述为 CLI 严格校验通过。

## Archive Information

**Archived:** 2026-09-08 16:46（Asia/Singapore，UTC+08:00）

**Duration:** 0 天（同日创建、实现与归档）

**Outcome:** Successfully implemented; acceptance confirmed by user.

**Acceptance Source:** 用户在本任务中确认：“全部通过，直接归档”。

### Files Modified

- `ImageConverterEngine.h` / `ImageConverterEngine.cpp`：SVG 目标格式和自包含 PNG 导出。
- `ImageConverterPanel.h` / `ImageConverterPanel.cpp`：格式选择、质量控件及清晰度提示。
- `FileProcessing.pro` / `FileProcessing.vcxproj`：Qt Widgets 模块声明与链接配置。
- `README.md` / `VERSION_HISTORY.md`：功能与版本说明。
- `tests/svg_export_tests.cpp`、`tests/svg-export-tests.pro`、`tests/run-svg-tests.cmd`、`tests/README.md`、`tests/.gitignore`：验证入口、说明和产物隔离。
- `openspec/project.md`：项目上下文与正式规范入口。

### Specs Updated

- [image-conversion.md](../../specs/image-conversion.md)：合并 6 项需求、17 个验收场景。
