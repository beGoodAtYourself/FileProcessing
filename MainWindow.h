/**
 * @file MainWindow.h
 * @brief 主窗口类声明
 *
 * MainWindow 为 FileSplitter GUI 的主界面，提供以下功能：
 *   - 源文件选择（文件浏览对话框）
 *   - 拆分大小和单位设置（数字输入 + KB/MB/GB 下拉框）
 *   - 并行线程数设置
 *   - 输出目录选择（文件夹浏览对话框）
 *   - 实时预览预估分块数
 *   - 开始/取消拆分操作
 *   - 进度条显示
 *   - 彩色日志输出（红色错误、蓝色信息、绿色完成、橙色取消）
 *   - 图片格式转换功能（独立模块）
 *
 * 架构说明：
 *   MainWindow 持有 SplitWorker 指针，通过信号槽与工作线程通信。
 *   拆分期间禁用输入控件，防止参数被修改；拆分完成后恢复。
 *   通过模式切换在文件拆分和图片转换功能之间切换。
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QLabel>
#include <QtWidgets/QStackedWidget>
#include "FileSplitterCore.h"

// 前置声明，避免头文件循环依赖
class SplitWorker;
class ImageConverterPanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    /** @brief 点击源文件浏览按钮，弹出文件选择对话框 */
    void onBrowseSourceFile();

    /** @brief 点击输出目录浏览按钮，弹出文件夹选择对话框 */
    void onBrowseOutputDir();

    /** @brief 点击开始按钮，校验参数并启动拆分工作线程 */
    void onStartSplit();

    /** @brief 点击取消按钮，设置取消标志中止拆分 */
    void onCancelSplit();

    /**
     * @brief 响应进度变化信号，更新进度条
     * @param partIndex    当前分块序号
     * @param writtenBytes 已写入字节数
     * @param totalBytes   总字节数
     * @param totalParts   总分块数
     */
    void onProgressChanged(int partIndex, long long writtenBytes, long long totalBytes, int totalParts);

    /**
     * @brief 响应日志消息信号，追加彩色日志
     * @param message 日志文本
     */
    void onLogMessage(const QString& message);

    /**
     * @brief 响应拆分完成信号，恢复 UI 状态
     * @param retCode 返回码，0 成功，-100 取消，其他负数失败
     */
    void onSplitFinished(int retCode);

    /** @brief 输入参数变化时更新预估信息 */
    void onEstimationChanged();

    /** @brief 导航按钮点击：切换到文件拆分页面 */
    void onNavFileSplitterClicked();

    /** @brief 导航按钮点击：切换到图片转换页面 */
    void onNavImageConverterClicked();

private:
    /** @brief 创建所有 UI 控件和布局 */
    void createUI();

    /** @brief 创建文件拆分页面 */
    QWidget* createSplitPage();

    /** @brief 连接信号与槽 */
    void createConnections();

    /**
     * @brief 批量启用/禁用输入控件
     * @param enabled true 启用输入控件并禁用取消按钮，false 反之
     */
    void setControlsEnabled(bool enabled);

    /** @brief 根据当前输入计算并显示预估分块数 */
    void updateEstimation();

    /**
     * @brief 向日志区域追加带颜色的文本
     * @param text  日志内容
     * @param color HTML 颜色值
     */
    void appendLog(const QString& text, const QString& color = "black");

    /** @brief 更新导航按钮的选中状态样式 */
    void updateNavigationButtons();

    // ---- 顶部导航栏 ----
    QPushButton* m_fileSplitterBtn;     ///< 文件拆分导航按钮
    QPushButton* m_imageConverterNavBtn; ///< 图片转换导航按钮

    // ---- 主界面容器 ----
    QStackedWidget* m_stackedWidget;    ///< 堆栈窗口，用于切换功能页面

    // ---- 文件拆分页面控件 ----
    QWidget* m_splitPage;               ///< 文件拆分页面
    QLineEdit*    m_sourceFileEdit;   ///< 源文件路径输入框
    QPushButton*  m_browseSourceBtn;  ///< 源文件浏览按钮
    QSpinBox*     m_splitSizeSpin;    ///< 拆分大小数值输入
    QComboBox*    m_unitCombo;        ///< 拆分单位下拉框（KB/MB/GB）
    QSpinBox*     m_threadCountSpin;  ///< 并行线程数输入框
    QLineEdit*    m_outputDirEdit;    ///< 输出目录路径输入框
    QPushButton*  m_browseOutputBtn;  ///< 输出目录浏览按钮
    QPushButton*  m_startBtn;         ///< 开始拆分按钮
    QPushButton*  m_cancelBtn;        ///< 取消拆分按钮
    QProgressBar* m_progressBar;      ///< 进度条
    QLabel*       m_estimationLabel;  ///< 预估信息标签
    QLabel*       m_sourceFileIcon;   ///< 源文件图标标签
    QLabel*       m_outputDirIcon;    ///< 输出目录图标标签
    QTextEdit*    m_logEdit;          ///< 日志显示区域

    // ---- 图片转换页面 ----
    ImageConverterPanel* m_imageConverterPanel; ///< 图片转换面板

    // ---- 工作线程 ----
    SplitWorker*  m_pWorker;          ///< 拆分工作线程指针，NULL 表示空闲
};

#endif // MAINWINDOW_H
