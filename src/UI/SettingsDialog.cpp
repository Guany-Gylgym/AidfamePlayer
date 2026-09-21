#include "UI/SettingsDialog.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QStandardItemModel>

namespace aidfame {
SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("播放器设置"));
    resize(640, 460); setMinimumSize(600, 420);
    setStyleSheet(QStringLiteral("QDialog,QWidget{background:#191b20;color:#eef0f4;font:13px 'Microsoft YaHei UI';}"
        "QLineEdit,QComboBox{background:#101114;border:1px solid #30343d;padding:6px;}"
        "QPushButton{background:#292d35;border:1px solid #30343d;border-radius:4px;padding:7px 16px;}"
        "QPushButton:focus{border-color:#37c8ee;}QTabBar::tab{padding:8px;background:#101114;}"
        "QTabBar::tab:selected{background:#292d35;color:#37c8ee;}"));
    auto* layout = new QVBoxLayout(this); layout->setContentsMargins(16,16,16,16);
    auto* tabs = new QTabWidget(this); tabs->setObjectName("settingsTabs"); layout->addWidget(tabs);
    const auto page = [tabs](const QString& label) {
        auto* widget = new QWidget(tabs); auto* form = new QFormLayout(widget);
        form->setContentsMargins(16,24,16,16); form->setSpacing(12); tabs->addTab(widget,label); return form;
    };
    auto* playback = page(QStringLiteral("播放"));
    auto* info = new QCheckBox(QStringLiteral("右上角显示媒体信息")); info->setObjectName("showInfo"); info->setChecked(true); playback->addRow(info);
    auto* resume = new QCheckBox(QStringLiteral("从上次观看位置继续")); resume->setObjectName("resumeHistory"); resume->setChecked(true); playback->addRow(resume);
    auto* shortcuts = page(QStringLiteral("快捷键"));
    auto* step = new QComboBox; step->setObjectName("seekStep");
    for (int value : {5,10,15,20,30}) step->addItem(QStringLiteral("%1 秒").arg(value),value);
    shortcuts->addRow(QStringLiteral("左右键快进 / 快退"),step);
    shortcuts->addRow(new QLabel(QStringLiteral("上下键调节音量 · 空格播放暂停 · F11 全屏")));
    const auto directoryRow = [](QFormLayout* form, const char* name, const QString& value) {
        auto* container = new QWidget; auto* row = new QHBoxLayout(container); row->setContentsMargins(0,0,0,0);
        auto* input = new QLineEdit(value); input->setObjectName(name); row->addWidget(input,1);
        auto* browse = new QPushButton(QStringLiteral("浏览…")); browse->setObjectName(QString::fromLatin1(name)+"Browse"); row->addWidget(browse);
        form->addRow(QStringLiteral("保存目录"),container);
    };
    auto* screenshot = page(QStringLiteral("截图"));
    auto* format = new QComboBox; format->setObjectName("screenshotFormat"); format->addItems({"PNG","JPG"});
    screenshot->addRow(QStringLiteral("图像格式"),format);
    directoryRow(screenshot,"screenshotDirectory",QStringLiteral("C:\\Users\\User\\Pictures\\AidfamePlayer\\Screenshot"));
    auto* record = page(QStringLiteral("录制"));
    directoryRow(record,"recordingDirectory",QStringLiteral("C:\\Users\\User\\Videos\\AidfamePlayer\\Record"));
    auto* recordingHelp = new QLabel(QStringLiteral("MP4 · 停止后生成文件。暂停不计入；跳转会结束录制。\n录制期间锁定倍速、清晰度、音量与画面比例。"));
    recordingHelp->setWordWrap(true); record->addRow(recordingHelp);
    auto* cache = page(QStringLiteral("缓存"));
    auto* size = new QLabel(QStringLiteral("0 B")); size->setObjectName("cacheSize"); cache->addRow(QStringLiteral("本程序缓存"),size);
    auto* clear = new QPushButton(QStringLiteral("清理缓存")); clear->setObjectName("clearCache"); clear->setEnabled(false); cache->addRow(clear);
    auto* safety = new QLabel(QStringLiteral("仅清理本程序专用缓存，不删除视频、截图或录制文件。")); safety->setWordWrap(true); cache->addRow(safety);
    auto* performance = page(QStringLiteral("性能"));
    auto* decoder = new QComboBox; decoder->setObjectName("hardwareDecoder");
    decoder->addItem(QStringLiteral("自动（优先 GPU，失败回退 CPU）"),"auto");
    decoder->addItem("D3D11VA","d3d11va"); decoder->addItem("DXVA2","dxva2");
    decoder->addItem("NVIDIA NVDEC","nvdec");
    decoder->addItem(QStringLiteral("Intel QSV（尚未通过实机验证）"),"qsv");
    qobject_cast<QStandardItemModel*>(decoder->model())->item(decoder->count()-1)->setEnabled(false);
    decoder->addItem(QStringLiteral("CPU 软件解码"),"no");
    performance->addRow(QStringLiteral("解码策略"),decoder);
    auto* hwHelp = new QLabel(QStringLiteral("可用性取决于显卡、驱动、编码格式与位深。\n媒体信息显示实际使用的解码器。")); hwHelp->setWordWrap(true); performance->addRow(hwHelp);
    auto* defaults = page(QStringLiteral("默认应用"));
    auto* registerButton = new QPushButton(QStringLiteral("注册 MP4 / MKV / MOV / AVI")); registerButton->setObjectName("registerTypes"); registerButton->setEnabled(false); defaults->addRow(registerButton);
    auto* defaultButton = new QPushButton(QStringLiteral("打开 Windows 默认应用设置")); defaultButton->setObjectName("defaultApps"); defaultButton->setEnabled(false); defaults->addRow(defaultButton);
    auto* note = new QLabel(QStringLiteral("Windows 要求你在系统设置中确认默认播放器。")); note->setWordWrap(true); defaults->addRow(note);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel,this);
    buttons->button(QDialogButtonBox::Save)->setText(QStringLiteral("保存")); buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
    layout->addWidget(buttons);
}
}
