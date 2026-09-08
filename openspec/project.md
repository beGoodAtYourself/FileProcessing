# FileProcessing 项目约定

本文根据 2026-09-08 的工程文件和源码记录当前项目上下文；功能变更在 `changes/` 中提出，完成并归档后再形成 `specs/` 中的正式规范。

## 项目与技术栈

- Windows 桌面文件处理工具，包含文件拆分和图片格式转换。
- C++、Visual Studio 2013 / v120。项目原基于 Qt 5.6.3，当前 VS 工程配置使用 Qt 5.8 的 `msvc2013_64` SDK。
- `FileProcessing.sln` 当前配置为 Debug / Release 的 x64 构建；以工程实际配置为准，不由 README 的平台描述推导其他构建已经可用。
- 同时维护 `FileProcessing.vcxproj` 和 qmake 的 `FileProcessing.pro` / `FileProcessing.pri`。
- 图片功能使用 QtCore、QtGui、QtWidgets，HEIC 编解码依赖 `Depends/heif` 中的 libheif。

## 图片转换架构

- `ImageConverterPanel`：选择文件、目标格式、质量、输出目录和覆盖策略，显示进度与日志。
- `ImageConverterWorker`：在 QThread 中执行任务，把引擎回调桥接为 Qt 信号。
- `ImageConverterEngine`：处理格式映射、图片读写、输出路径、逐文件结果和取消检查。
- `HeicDecoder` / `HeicEncoder`：处理 HEIC / HEIF 输入与 HEIC 输出；其他现有格式通过 QImageReader / QImageWriter 处理。
- 当前公开输入扩展名为 bmp、jpg、jpeg、png、ico、heic、heif；目标格式为 BMP、JPEG、PNG、ICO、HEIC，以及新增的 SVG 输出（内嵌 PNG，不扩展 SVG 输入）。
- 输出路径采用源文件完整基本名加目标扩展名，目录为空时输出到源目录。默认跳过已有目标文件，可选择覆盖。
- 取消检查发生在每个文件开始前；单个文件的读写过程没有细粒度中断保证。

## 开发与验证约定

- 保持 UI、工作线程和引擎的职责边界，以及现有信号槽和回调风格。
- 新代码须兼容 Qt 5.6 和 v120，不能直接采用新版 Qt 或 C++ 才提供的 API。
- UI 文案沿用 `tr()` 包装的英文字符串，项目说明和提案可使用中文。
- 新增编译单元时同步 Visual Studio 和 qmake 文件；仅修改已有编译单元时无需新增项目项。
- 图片转换已有 `tests/svg-export-tests.pro` 的 QtTest 验证入口，使用说明见 `tests/README.md`。记录实际构建配置、人工验收与环境限制，不能把不同 SDK 或缺失编解码器的结果视为完整通过。
- 图片转换的 SVG 输出规范见 [specs/image-conversion.md](specs/image-conversion.md)，变更历史见 [archive/add-svg-export/proposal.md](archive/add-svg-export/proposal.md)。后续变更以正式规范为基准增补需求。
