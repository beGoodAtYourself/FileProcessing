# SVG 导出验证

测试直接编译生产转换引擎、工作线程、面板及 HEIC 编解码器，通过 QtTest 自动验证文件内容和界面状态。测试依赖不加入应用运行时。

## 构建与执行

在 Windows 上准备 VS2013 / v120、Qt 5.6 的 `msvc2013_64` SDK，以及项目已有的 `Depends/heif/lib/x64/heif.lib` 和 `Output/heif.dll`。在项目根目录运行，将参数替换为实际 Qt SDK 路径：

```powershell
& .\tests\run-svg-tests.cmd 'D:\qt\5.6.3\msvc2013_64'
```

脚本设置 VS2013 x64 环境，以 qmake 和 nmake 构建 Release 测试程序，并以 Qt offscreen 平台运行界面用例。所有构建文件、结果和示例位于被 Git 忽略的 `tests/.build/`，不会覆盖 `Output/FileProcessing.exe`。若使用其他 Qt 版本验证，应记录实际版本，不能将其等同于 Qt 5.6 验证。

测试包含：

- BMP、JPG/JPEG、PNG、ICO、HEIC/HEIF 输入和透明 PNG/HEIC 的 SVG 导出。
- XML、SVG/XLink 命名空间、画布及图片尺寸、内嵌 PNG 签名与解码后 RGBA 像素一致性。
- 默认目录、多点文件名、中文和空格目录、质量参数独立性。
- 跳过、覆盖、同名冲突、坏文件继续、目录和目标打开失败、取消文件边界。
- Windows 文件锁触发的真实提交失败：检查原目标完整且没有遗留临时文件。
- 原有五种目标编码分支及 JPEG/HEIC 的不同质量参数。
- 面板格式切换、质量和说明可见性、单文件/批量运行及结束后的控件恢复。
- 4096×2048 图片的工作线程转换、主线程定时器响应、耗时及进程峰值内存。

## 可视化验收

运行后检查 `tests/.build/artifacts/svg-panel.png` 的文字和布局。在浏览器中打开 `tests/.build/artifacts/standalone/preview.svg`，确认 512×256 的图像比例、色彩和透明条纹；该目录只含 SVG，不依赖源 PNG。可将此目录复制到其他机器复查。

在实际应用窗口中再检查两个标签页的 SVG 选择、文件/文件夹添加、转换过程及取消操作。正常完成后 Convert 恢复可用，Cancel 禁用。取消在文件边界生效，当前文件可以完成。

## 验证边界

自动测试需要可工作的既有 HEVC 编解码依赖，缺失时会明确失败，不把 HEIC 项静默记为通过。提交失败测试依赖 Windows 文件共享语义。目录/打开失败及已存在目标提交失败有自动覆盖；磁盘写满、断电和新目标写到一半的 I/O 故障需要单独的受控故障环境，不能从成功路径测试推定已经执行。

本次实际执行记录与最终验收确认见 [validation.md](../openspec/archive/add-svg-export/validation.md)。
