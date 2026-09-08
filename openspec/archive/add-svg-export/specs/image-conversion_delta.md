# Delta: 图片转换的 SVG 输出能力

**Change ID:** `add-svg-export`

**Affects:** 图片转换引擎、格式选择界面、单文件与批量转换、功能说明

## ADDED Requirements

### Requirement: SVG 目标格式

系统 MUST 在现有目标格式列表末尾提供 SVG，使单文件和批量模式都能将现有受支持且可成功解码的 BMP、JPEG、PNG、ICO、HEIC/HEIF 图片输出为 `.svg`。默认目标仍为 JPEG，现有格式枚举值保持不变。

#### Scenario: 单文件导出

- GIVEN 用户选择一个可成功解码的受支持源文件。
- WHEN 用户选择 SVG 并开始转换。
- THEN 系统通过现有工作线程生成对应的 `.svg`，并报告该文件的成功结果与日志。

#### Scenario: 混合格式批量导出

- GIVEN 批量列表包含多种受支持格式，且每个文件均可解码、目标路径互不冲突。
- WHEN 用户选择 SVG 并开始转换。
- THEN 系统按列表顺序为每个源文件生成 SVG，并通过现有接口报告进度、结果和实际成功数。

### Requirement: 自包含位图 SVG

系统 MUST 输出可解析的 UTF-8 SVG XML，声明 SVG 与 XLink 命名空间，以 PNG Base64 data URI 内嵌解码图片。根元素的宽高、`viewBox="0 0 W H"` 以及位于 `(0, 0)` 的 `<image>` 尺寸 MUST 与解码图片的像素尺寸一致。系统 MUST 保留解码后的 RGBA 内容，不新增缩放、背景填充或有损编码。

#### Scenario: 透明图片保持内容

- GIVEN 当前解码流程得到一张含透明和半透明像素的图片。
- WHEN 导出为 SVG 并提取其中的 PNG 数据重新解码。
- THEN PNG 的宽高和标准化 RGBA 像素与原 QImage 一致，透明部分没有被填充成背景色。

#### Scenario: SVG 可脱离原图使用

- GIVEN 用户已成功导出 SVG。
- WHEN 将 SVG 单独复制到无法访问原图的目录并使用支持 SVG 的浏览器打开。
- THEN SVG 完整显示原解码图片，不请求原文件、旁置图片或网络图片，尺寸比例保持一致。

#### Scenario: 质量参数不影响 SVG 图片内容

- GIVEN 同一源文件分别使用两个不同的质量参数进行 SVG 导出，且目标输出没有被跳过。
- WHEN 解码两个 SVG 中内嵌的 PNG。
- THEN 两张图片的尺寸和 RGBA 内容一致，不因 JPEG/HEIC 质量参数变化。

### Requirement: SVG 输出路径与覆盖

系统 MUST 沿用现有输出命名规则，以源文件 `completeBaseName()` 加 `.svg` 组成文件名；目录为空时输出到源目录，否则输出到指定目录并按现有流程创建缺失目录。已有目标和同名冲突 MUST 使用现有覆盖选项与列表顺序处理。

#### Scenario: 默认目录和多点文件名

- GIVEN 源文件名为 `photo.v2.png`，输出目录为空且目标不存在。
- WHEN 导出 SVG。
- THEN 源目录出现 `photo.v2.svg`，源文件保持原样。

#### Scenario: 自定义目录含中文和空格

- GIVEN 用户指定有效且可创建的含中文或空格的输出目录。
- WHEN 导出 SVG。
- THEN 系统在指定目录创建正确命名的 SVG，目录缺失时自动创建。

#### Scenario: 不覆盖已有目标

- GIVEN 目标 `.svg` 已存在，用户未勾选覆盖选项。
- WHEN 该文件进入转换流程。
- THEN 目标内容保持原样，记录跳过日志与结果，不增加实际转换成功数，并继续后续文件。

#### Scenario: 按列表顺序处理同名输出

- GIVEN `photo.png` 和 `photo.jpg` 按此顺序转换到同一目录，初始没有 `photo.svg`，两个源文件均可解码。
- WHEN 未开启覆盖时执行 SVG 批量转换。
- THEN 第一张图生成 `photo.svg`，第二张被跳过；启用覆盖并重新执行同一列表时，最终目标包含第二张图的内容。

### Requirement: SVG 保存失败处理

系统 MUST 只在 PNG 编码、SVG 写入和最终提交均成功后记录转换成功。发生解码、编码、创建目录、文件写入或提交失败时，系统 MUST 记录失败结果和包含路径与失败原因或阶段的日志，继续处理后续文件。未成功提交的新目标 MUST 不留下残缺 SVG，覆盖写入失败 MUST 保留原目标内容。

#### Scenario: 无效图片不阻断批量任务

- GIVEN 批量列表依次包含一个损坏图片和一个有效图片，目标目录可写。
- WHEN 执行 SVG 转换。
- THEN 损坏图片记录失败且不产生 SVG，有效图片仍完成导出，成功数只包含实际转换成功的文件。

#### Scenario: 输出目录无法创建

- GIVEN 指定输出目录因权限或路径冲突无法创建。
- WHEN 尝试导出 SVG。
- THEN 记录包含目录路径的失败日志和失败结果，不报告转换成功。

#### Scenario: 新目标写入或提交失败

- GIVEN 新目标原本不存在，且在写入或提交阶段发生可复现的 I/O 失败。
- WHEN 保存 SVG。
- THEN 返回失败，清理临时数据，不留下以最终目标文件名存在的残缺 SVG。

#### Scenario: 覆盖目标时保存失败

- GIVEN 目标文件已存在且允许覆盖，保存过程发生可复现的编码、写入或提交失败。
- WHEN SVG 保存返回。
- THEN 原目标字节内容保持不变，失败文件不计入实际转换成功数。

### Requirement: SVG 格式说明与控制项

系统 MUST 在选择 SVG 时隐藏 JPEG/HEIC 质量标签与输入框，并显示“保留原图外观，放大清晰度受原图分辨率限制”的说明。切换到其他格式时 MUST 隐藏该 SVG 说明，并按现有 JPEG/HEIC 规则控制质量设置。UI 文案采用现有 `tr()` 风格。

#### Scenario: 切换 SVG 与 JPEG

- GIVEN 图片转换界面的目标格式为 JPEG。
- WHEN 用户改选 SVG，随后切回 JPEG。
- THEN SVG 状态显示清晰度说明且隐藏质量控件；回到 JPEG 后说明隐藏，质量控件恢复可见。

#### Scenario: 输入入口保持原支持范围

- GIVEN 用户浏览单个文件、添加多个文件或从文件夹添加图片。
- WHEN 查看图片过滤器或执行文件夹扫描。
- THEN 公开图片扩展名仍为 bmp、jpg、jpeg、png、ico、heic、heif，不因新增 SVG 目标而把 svg 加入输入支持列表；既有“所有文件”入口不构成 SVG 输入支持承诺。

### Requirement: SVG 任务与既有流程兼容

系统 MUST 在既有工作线程中执行 SVG 编码，并沿用文件边界取消、进度、结果记录及操作结束后的控件恢复。原有五种目标格式的编码路径和质量设置行为 MUST 保持兼容。

#### Scenario: 文件边界取消

- GIVEN 多文件 SVG 转换正在执行，且仍有尚未开始处理的文件。
- WHEN 用户请求取消，且下一文件开始前的取消检查观察到取消标志。
- THEN 当前正在处理的文件允许完成并保存完整输出，后续文件不再开始转换，已完成输出保留，工作线程结束后恢复操作控件。

#### Scenario: 既有格式转换回归

- GIVEN 用户选择原有 BMP、JPEG、PNG、ICO 或 HEIC 作为目标，运行环境具备相应编解码能力。
- WHEN 按既有流程执行转换。
- THEN 仍调用相应既有编码路径，并保持已有输出扩展名、质量设置和覆盖选项行为。

## MODIFIED Requirements

无。此前不存在正式 OpenSpec 规范；本次对现有流程的扩展约束一并记录在新增需求中。

## REMOVED Requirements

无。
