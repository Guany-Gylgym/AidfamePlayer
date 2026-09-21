#include "UI/MainWindow.h"
#include "Player/MpvPlayer.h"
#include "Recorder/Recorder.h"
#include "UI/SettingsDialog.h"
#include "Settings/CacheStore.h"
#include "Settings/FileAssociations.h"
#include <QDesktopServices>
#include <QUrl>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QCheckBox>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QSlider>
#include <QStackedWidget>
#include <QSignalBlocker>
#include <QTimer>
#include <QStatusBar>
#include <QStandardPaths>
#include <QDir>
#include <QCloseEvent>
#include <QSvgWidget>
#include <QToolButton>
#include <QVBoxLayout>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace aidfame {
namespace {

QToolButton* iconButton(const QString& name, const QString& icon,
                        const QString& label, QWidget* parent) {
    auto* button = new QToolButton(parent);
    button->setObjectName(name);
    button->setIcon(QIcon(QStringLiteral(":/icons/") + icon + QStringLiteral(".svg")));
    button->setIconSize(QSize(20, 20));
    button->setFixedSize(36, 36);
    button->setToolTip(label);
    button->setAccessibleName(label);
    button->setFocusPolicy(Qt::StrongFocus);
    return button;
}

} // namespace

MainWindow::MainWindow(QWidget* parent, bool staticPreview, const QString& dataRoot) : QMainWindow(parent), staticPreview_(staticPreview), store_(dataRoot) {
    setObjectName(QStringLiteral("mainWindow"));
    setAcceptDrops(!staticPreview_);
    setWindowTitle(QStringLiteral("Aidfame Player · 内部版"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/AidfamePlayer.ico")));
    resize(1280, 800);
    setMinimumSize(800, 500);
    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget#root { background: #101114; color: #eef0f4; }
        QWidget { font-family: "Microsoft YaHei UI"; font-size: 13px; }
        QMenuBar { background: #191b20; color: #d1d6df; padding: 4px 12px; }
        QMenuBar::item { padding: 5px 14px; background: transparent; }
        QMenuBar::item:selected { background: #292d35; border-radius: 4px; }
        QMenu { background: #191b20; color: #eef0f4; border: 1px solid #30343d; padding: 5px; }
        QMenu::item { padding: 8px 28px; }
        QMenu::item:selected { background: #292d35; }
        QMenu::item:disabled { color: #606773; }
        QWidget#viewport { background: #08090b; }
        QLabel { color: #eef0f4; background: transparent; }
        QLabel#brandTitle { font-size: 28px; font-weight: 600; }
        QLabel#subtitle, QLabel#emptyHistory { color: #9299a6; }
        QLabel#stageLabel { color: #606773; font-size: 11px; }
        QPushButton#openButton { background: #37c8ee; color: #061116;
            border: none; border-radius: 6px; padding: 12px 28px; font-weight: 600; }
        QPushButton#openButton:hover { background: #6cd9f5; }
        QPushButton#openButton:pressed { background: #179dbc; }
        QPushButton:focus { border: 2px solid #eef0f4; }
        QWidget#transport { background: #191b20; border-top: 1px solid #30343d; }
        QToolButton { background: transparent; border: 1px solid transparent; border-radius: 5px; }
        QToolButton:hover { background: #292d35; }
        QToolButton:focus { border: 1px solid #37c8ee; }
        QToolButton:disabled { background: transparent; }
        QComboBox { color: #d1d6df; background: #22252b; border: 1px solid #30343d;
            border-radius: 4px; padding: 5px 8px; min-height: 20px; }
        QComboBox:disabled { color: #606773; }
        QComboBox QAbstractItemView { background: #191b20; color: #eef0f4; selection-background-color: #30343d; }
        QSlider::groove:horizontal { background: #30343d; height: 3px; border-radius: 1px; }
        QSlider::sub-page:horizontal { background: #37c8ee; border-radius: 1px; }
        QSlider::handle:horizontal { background: #37c8ee; width: 10px; margin: -4px 0; border-radius: 5px; }
        QSlider::sub-page:horizontal:disabled, QSlider::handle:horizontal:disabled { background: #606773; }
        QLabel#timeLabel { font-family: Consolas; color: #9299a6; font-size: 12px; }
        QStatusBar { background: #101114; color: #9299a6; font-size: 11px; }
        QStatusBar::item { border: none; }
    )"));

    auto* fileMenu = menuBar()->addMenu(QStringLiteral("文件(&F)"));
    auto* openAction = fileMenu->addAction(QStringLiteral("打开视频…"));
    openAction->setObjectName(QStringLiteral("openAction"));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::chooseFile);
    auto* recentMenu = fileMenu->addMenu(QStringLiteral("最近播放"));
    recentMenu->setObjectName("recentMenu");
    fileMenu->addSeparator();
    auto* exitAction = fileMenu->addAction(QStringLiteral("退出"));
    exitAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_F4));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    auto* playbackMenu = menuBar()->addMenu(QStringLiteral("播放(&P)"));
    auto* playAction = playbackMenu->addAction(QStringLiteral("播放 / 暂停"));
    playAction->setObjectName(QStringLiteral("playAction"));
    playAction->setShortcut(QKeySequence(Qt::Key_Space));
    playAction->setEnabled(false);
    connect(playAction, &QAction::triggered, this, [this] { if (player_) player_->setPaused(!paused_); });
    playbackMenu->addSeparator();
    auto* fullAction = playbackMenu->addAction(QStringLiteral("全屏"));
    fullAction->setObjectName(QStringLiteral("fullscreenAction"));
    fullAction->setShortcut(QKeySequence(Qt::Key_F11));
    connect(fullAction, &QAction::triggered, this, &MainWindow::toggleFullscreen);
    auto* miniAction = playbackMenu->addAction(QStringLiteral("小窗 / 恢复窗口"));
    miniAction->setObjectName("miniAction"); miniAction->setShortcut(QKeySequence("Ctrl+M"));
    connect(miniAction,&QAction::triggered,this,&MainWindow::toggleMini);
    auto* aspectMenu = playbackMenu->addMenu(QStringLiteral("画面比例"));
    aspectMenu->setObjectName(QStringLiteral("aspectMenu"));
    aspectMenu->setEnabled(false);
    auto* aspectGroup = new QActionGroup(this);
    const QList<QPair<QString, QString>> aspects = {
        {QStringLiteral("原比例"), QStringLiteral("original")},
        {QStringLiteral("16:9"), QStringLiteral("16:9")},
        {QStringLiteral("4:3"), QStringLiteral("4:3")},
        {QStringLiteral("拉伸"), QStringLiteral("stretch")}};
    for (const auto& item : aspects) {
        auto* action = aspectMenu->addAction(item.first);
        action->setCheckable(true);
        action->setChecked(item.second == QStringLiteral("original"));
        action->setData(item.second);
        aspectGroup->addAction(action);
    }
    connect(aspectGroup, &QActionGroup::triggered, this, [this](QAction* action) {
        aspect_ = action->data().toString();
        if (player_) player_->setAspect(action->data().toString());
    });
    auto* infoAction = playbackMenu->addAction(QStringLiteral("显示媒体信息"));
    infoAction->setObjectName("infoAction");
    infoAction->setCheckable(true);
    infoAction->setChecked(true);
    connect(infoAction, &QAction::toggled, this, [this](bool value) {
        showInfo_ = value;
        if (player_) player_->showMediaInfo(value);
    });

    auto* settingsMenu = menuBar()->addMenu(QStringLiteral("设置(&S)"));
    auto* preferences = settingsMenu->addAction(QStringLiteral("播放器设置…"));
    preferences->setEnabled(!staticPreview_);
    preferences->setObjectName("preferencesAction");
    connect(preferences, &QAction::triggered, this, &MainWindow::showSettings);
    recordingDirectory_ = QDir(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation))
        .filePath(QStringLiteral("AidfamePlayer/Record"));
    connect(settingsMenu->addAction(QStringLiteral("录制保存目录…")), &QAction::triggered, this, [this] {
        const auto directory = QFileDialog::getExistingDirectory(this, QStringLiteral("录制保存目录"), recordingDirectory_);
        if (!directory.isEmpty()) recordingDirectory_ = directory;
    });
    screenshotDirectory_ = QDir(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation))
        .filePath(QStringLiteral("AidfamePlayer/Screenshot"));
    auto* screenshotMenu = settingsMenu->addMenu(QStringLiteral("截图设置"));
    auto* formatGroup = new QActionGroup(this);
    for (const auto& format : {QStringLiteral("png"), QStringLiteral("jpg")}) {
        auto* action = screenshotMenu->addAction(format.toUpper());
        action->setCheckable(true);
        action->setChecked(format == screenshotFormat_);
        action->setData(format);
        formatGroup->addAction(action);
    }
    connect(formatGroup, &QActionGroup::triggered, this, [this](QAction* action) { screenshotFormat_ = action->data().toString(); });
    screenshotMenu->addSeparator();
    connect(screenshotMenu->addAction(QStringLiteral("保存目录…")), &QAction::triggered, this, [this] {
        const auto directory = QFileDialog::getExistingDirectory(this, QStringLiteral("截图保存目录"), screenshotDirectory_);
        if (!directory.isEmpty()) screenshotDirectory_ = directory;
    });
    auto* seekMenu = settingsMenu->addMenu(QStringLiteral("快进 / 快退秒数"));
    auto* seekGroup = new QActionGroup(this);
    for (int seconds : {5, 10, 15, 20, 30}) {
        auto* action = seekMenu->addAction(QStringLiteral("%1 秒").arg(seconds));
        action->setObjectName(QStringLiteral("seekStep%1").arg(seconds));
        action->setCheckable(true);
        action->setChecked(seconds == seekStep_);
        action->setData(seconds);
        seekGroup->addAction(action);
    }
    connect(seekGroup, &QActionGroup::triggered, this, [this](QAction* action) { setSeekStep(action->data().toInt()); });
    settingsMenu->addSeparator();
    auto* about = settingsMenu->addAction(QStringLiteral("关于 Aidfame Player"));
    connect(about, &QAction::triggered, this, [this] {
        QMessageBox::about(this, QStringLiteral("Aidfame Player"),
            QStringLiteral("Aidfame Player 1.0.0\n本地离线视频播放器\n\n仅供个人与公司内部使用。\n兼容范围和已知限制见随附测试报告。"));
    });

    auto* root = new QWidget(this);
    root->setObjectName(QStringLiteral("root"));
    auto* rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    setCentralWidget(root);

    auto* viewport = new QWidget(root);
    viewport->setObjectName(QStringLiteral("viewport"));
    viewport->setMinimumHeight(220);
    auto* emptyLayout = new QVBoxLayout(viewport);
    emptyLayout->setContentsMargins(24, 24, 24, 24);
    emptyLayout->setSpacing(12);
    emptyLayout->addStretch();
    auto* brand = new QLabel(viewport);
    brand->setObjectName("brandMark");
    brand->setPixmap(QPixmap(QStringLiteral(":/icons/brand.png")).scaled(176, 176, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    brand->setScaledContents(true);
    brand->setFixedSize(88, 88);
    emptyLayout->addWidget(brand, 0, Qt::AlignHCenter);
    auto* title = new QLabel(QStringLiteral("Aidfame Player"), viewport);
    title->setObjectName(QStringLiteral("brandTitle"));
    emptyLayout->addWidget(title, 0, Qt::AlignHCenter);
    auto* subtitle = new QLabel(QStringLiteral("专注画面，尽享本地播放"), viewport);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    emptyLayout->addWidget(subtitle, 0, Qt::AlignHCenter);
    emptyLayout->addSpacing(8);
    auto* openButton = new QPushButton(QStringLiteral("打开视频"), viewport);
    openButton->setObjectName(QStringLiteral("openButton"));
    openButton->setAccessibleName(QStringLiteral("打开本地视频"));
    openButton->setCursor(Qt::PointingHandCursor);
    openButton->setFixedWidth(160);
    connect(openButton, &QPushButton::clicked, this, &MainWindow::chooseFile);
    emptyLayout->addWidget(openButton, 0, Qt::AlignHCenter);
    auto* history = new QLabel(QStringLiteral("暂无最近播放记录"), viewport);
    history->setObjectName(QStringLiteral("emptyHistory"));
    emptyLayout->addWidget(history, 0, Qt::AlignHCenter);
    emptyLayout->addStretch();
    auto* stage = new QLabel(staticPreview_ ? QStringLiteral("STAGE 1  /  窗口预览 · 播放功能待接入")
        : QStringLiteral("本地离线  /  内部版"), viewport);
    stage->setObjectName(QStringLiteral("stageLabel"));
    emptyLayout->addWidget(stage, 0, Qt::AlignHCenter);
    viewportStack_ = new QStackedWidget(root);
    viewportStack_->addWidget(viewport);
    videoSurface_ = new QWidget(viewportStack_);
    videoSurface_->setObjectName(QStringLiteral("videoSurface"));
    videoSurface_->setAttribute(Qt::WA_NativeWindow);
    videoSurface_->setStyleSheet(QStringLiteral("background: black;"));
    viewportStack_->addWidget(videoSurface_);
    rootLayout->addWidget(viewportStack_, 1);

    auto* transport = new QWidget(root);
    transport->setObjectName(QStringLiteral("transport"));
    auto* transportLayout = new QVBoxLayout(transport);
    transportLayout->setContentsMargins(16, 4, 16, 8);
    transportLayout->setSpacing(2);
    auto* seek = new QSlider(Qt::Horizontal, transport);
    seek->setObjectName(QStringLiteral("seekSlider"));
    seek->setAccessibleName(QStringLiteral("播放进度"));
    seek->setRange(0, 1000);
    seek->setEnabled(false);
    transportLayout->addWidget(seek);

    auto* row = new QHBoxLayout;
    row->setSpacing(8);
    auto* play = iconButton(QStringLiteral("playButton"), QStringLiteral("play"), QStringLiteral("播放 / 暂停"), transport);
    play->setEnabled(false);
    connect(play, &QToolButton::clicked, playAction, &QAction::trigger);
    row->addWidget(play);
    auto* time = new QLabel(QStringLiteral("00:00 / 00:00"), transport);
    time->setObjectName(QStringLiteral("timeLabel"));
    time->setMinimumWidth(112);
    row->addWidget(time);
    row->addStretch();
    auto* volumeButton = iconButton(QStringLiteral("volumeButton"), QStringLiteral("volume"), QStringLiteral("静音"), transport);
    volumeButton->setEnabled(false);
    connect(volumeButton, &QToolButton::clicked, this, [this, volumeButton] {
        if (player_ && player_->setMuted(!muted_)) {
            muted_ = !muted_;
            volumeButton->setToolTip(muted_ ? QStringLiteral("取消静音") : QStringLiteral("静音"));
        }
    });
    row->addWidget(volumeButton);
    auto* volume = new QSlider(Qt::Horizontal, transport);
    volume->setObjectName(QStringLiteral("volumeSlider"));
    volume->setAccessibleName(QStringLiteral("音量"));
    volume->setRange(0, 100);
    volume->setValue(80);
    volume->setFixedWidth(80);
    volume->setEnabled(false);
    connect(volume, &QSlider::valueChanged, this, [this](int value) { if (player_) player_->setVolume(value); });
    row->addWidget(volume);
    auto* speed = new QComboBox(transport);
    speed->setObjectName(QStringLiteral("speedCombo"));
    speed->setAccessibleName(QStringLiteral("播放倍速"));
    speed->addItems({QStringLiteral("0.5x"), QStringLiteral("0.75x"), QStringLiteral("1x"),
        QStringLiteral("1.25x"), QStringLiteral("1.5x"), QStringLiteral("2x"),
        QStringLiteral("3x"), QStringLiteral("4x"), QStringLiteral("5x")});
    speed->setCurrentIndex(2);
    speed->setFixedWidth(78);
    speed->setEnabled(false);
    connect(speed, &QComboBox::currentTextChanged, this, [this](QString value) {
        value.chop(1);
        if (player_) player_->setSpeed(value.toDouble());
    });
    row->addWidget(speed);
    auto* quality = new QComboBox(transport);
    quality->setObjectName(QStringLiteral("qualityCombo"));
    quality->setAccessibleName(QStringLiteral("画面清晰度"));
    quality->addItem(QStringLiteral("原画"), 0);
    quality->setFixedWidth(80);
    quality->setEnabled(false);
    connect(quality, &QComboBox::currentIndexChanged, this, [this, quality](int index) {
        if (index >= 0 && player_) player_->setResolutionCap(quality->itemData(index).toInt());
    });
    row->addWidget(quality);
    auto* screenshot = iconButton(QStringLiteral("screenshotButton"), QStringLiteral("camera"), QStringLiteral("截图"), transport);
    screenshot->setEnabled(false);
    connect(screenshot, &QToolButton::clicked, this, [this] {
        if (player_) player_->screenshot(screenshotDirectory_, screenshotFormat_);
    });
    row->addWidget(screenshot);
    auto* record = iconButton(QStringLiteral("recordButton"), QStringLiteral("record"), QStringLiteral("录制"), transport);
    record->setEnabled(false);
    connect(record, &QToolButton::clicked, this, &MainWindow::toggleRecording);
    row->addWidget(record);
    auto* fullscreen = iconButton(QStringLiteral("fullscreenButton"), QStringLiteral("fullscreen"), QStringLiteral("全屏 (F11)"), transport);
    connect(fullscreen, &QToolButton::clicked, this, &MainWindow::toggleFullscreen);
    row->addWidget(fullscreen);
    transportLayout->addLayout(row);
    rootLayout->addWidget(transport);
    statusBar()->showMessage(QStringLiteral("就绪  ·  本地离线"));
    auto* buildLabel = new QLabel(staticPreview_ ? QStringLiteral("STATIC QA") : QStringLiteral("1.0.0-internal"), this);
    statusBar()->addPermanentWidget(buildLabel);
    auto* escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escape, &QShortcut::activated, this, [this] {
        if (isFullScreen()) toggleFullscreen();
        else if (mini_) toggleMini();
    });
    connect(seek, &QSlider::sliderReleased, this, [this, seek] {
        if (recorder_ && recorder_->capturing()) recorder_->stop(position_);
        if (player_ && duration_ > 0) player_->seek(duration_ * seek->value() / 1000.0, true);
    });
    auto* infoTimer = new QTimer(this);
    infoTimer->setInterval(1000);
    connect(infoTimer, &QTimer::timeout, this, [this] {
        if (player_ && showInfo_ && viewportStack_->currentIndex() == 1) player_->showMediaInfo(true);
    });
    if (!staticPreview_) infoTimer->start();
    recorder_ = new Recorder(this);
    connect(recorder_, &Recorder::stateChanged, this, &MainWindow::refreshRecording);
    connect(recorder_, &Recorder::completed, this, [this](const QString& path) {
        statusBar()->showMessage(QStringLiteral("录制已保存：%1").arg(QDir::toNativeSeparators(path)), 15000);
    });
    connect(recorder_, &Recorder::failed, this, [this](const QString& message) { statusBar()->showMessage(message, 10000); });
    qApp->installEventFilter(this);
    if (!staticPreview_) {
        store_.load();
        applySettings(); refreshHistory();
        if (!store_.error.isEmpty()) statusBar()->showMessage(store_.error,10000);
        auto* saveTimer = new QTimer(this); saveTimer->setInterval(10000);
        connect(saveTimer,&QTimer::timeout,this,&MainWindow::saveState); saveTimer->start();
    }
}

MainWindow::~MainWindow() {
    saveState();
    qApp->removeEventFilter(this);
    disconnect(recorder_, nullptr, this, nullptr);
    delete recorder_;
    // Destroy the video backend before Qt destroys its native render target.
    delete player_;
}

void MainWindow::applySettings() {
    setSeekStep(store_.settings["seekStepSeconds"].toInt());
    screenshotDirectory_ = store_.settings["screenshotDirectory"].toString();
    recordingDirectory_ = store_.settings["recordingDirectory"].toString();
    screenshotFormat_ = store_.settings["screenshotFormat"].toString();
    showInfo_ = store_.settings["showMediaInfo"].toBool();
    findChild<QAction*>("infoAction")->setChecked(showInfo_);
    findChild<QSlider*>("volumeSlider")->setValue(store_.settings["volume"].toInt());
    findChild<QComboBox*>("speedCombo")->setCurrentText(QString::number(store_.settings["speed"].toDouble())+"x");
    aspect_ = store_.settings["aspectMode"].toString();
    for (auto* action : findChild<QMenu*>("aspectMenu")->actions()) action->setChecked(action->data().toString() == aspect_);
    if (player_) {
        player_->setHardwareDecoder(store_.settings["hardwareDecoder"].toString());
        player_->setVolume(findChild<QSlider*>("volumeSlider")->value());
        player_->setSpeed(store_.settings["speed"].toDouble());
        player_->setAspect(aspect_); player_->showMediaInfo(showInfo_);
    }
}

void MainWindow::saveState() {
    if (staticPreview_) return;
    store_.settings["seekStepSeconds"] = seekStep_;
    store_.settings["screenshotDirectory"] = screenshotDirectory_;
    store_.settings["recordingDirectory"] = recordingDirectory_;
    store_.settings["screenshotFormat"] = screenshotFormat_;
    store_.settings["showMediaInfo"] = showInfo_;
    store_.settings["volume"] = findChild<QSlider*>("volumeSlider")->value();
    QString speed = findChild<QComboBox*>("speedCombo")->currentText(); speed.chop(1);
    store_.settings["speed"] = speed.toDouble(); store_.settings["aspectMode"] = aspect_;
    if (rememberActive_ && fileReady_ && player_) {
        const auto duration = player_->numberProperty("duration");
        if (duration > 0) store_.remember(currentFile_,player_->numberProperty("time-pos"),duration);
    }
    if (!store_.save()) statusBar()->showMessage(store_.error,10000);
}

void MainWindow::refreshHistory() {
    auto* menu = findChild<QMenu*>("recentMenu"); menu->clear();
    QStringList summary;
    for (const auto& item : store_.history) {
        const auto entry = item.toObject(); const auto path = entry["path"].toString();
        const auto name = QFileInfo(path).fileName();
        auto* recent = menu->addMenu(name);
        connect(recent->addAction(QStringLiteral("播放 / 继续")), &QAction::triggered,this,[this,path] { openFile(path); });
        connect(recent->addAction(QStringLiteral("删除记录（保留文件）")), &QAction::triggered,this,[this,path] {
            if (currentFile_.compare(path,Qt::CaseInsensitive) == 0) rememberActive_ = false;
            store_.forget(path); store_.save(); refreshHistory();
        });
        if (summary.size() < 3) summary << QStringLiteral("%1 · %2 秒").arg(name).arg(entry["positionSeconds"].toDouble(),0,'f',0);
    }
    menu->setEnabled(!store_.history.isEmpty());
    auto* label = findChild<QLabel*>("emptyHistory"); label->setTextFormat(Qt::PlainText);
    label->setText(summary.isEmpty() ? QStringLiteral("暂无最近播放记录") : QStringLiteral("最近播放（文件菜单可继续 / 删除）\n") + summary.join('\n'));
    label->setWordWrap(true); label->setMaximumWidth(600);
}

void MainWindow::showSettings() {
    if (recorder_->capturing()) { statusBar()->showMessage(QStringLiteral("请先停止录制，再修改设置。"),5000); return; }
    saveState();
    SettingsDialog dialog(this);
    auto* registerTypes = dialog.findChild<QPushButton*>("registerTypes"); registerTypes->setEnabled(true);
    connect(registerTypes,&QPushButton::clicked,&dialog,[&dialog] {
        QString error;
        if (!registerFileAssociations(&error)) QMessageBox::warning(&dialog,QStringLiteral("注册失败"),error);
        else QMessageBox::information(&dialog,QStringLiteral("文件类型已注册"),QStringLiteral("请在 Windows 默认应用设置中选择 Aidfame Player。"));
    });
    auto* defaultApps = dialog.findChild<QPushButton*>("defaultApps"); defaultApps->setEnabled(true);
    connect(defaultApps,&QPushButton::clicked,&dialog,[&dialog] {
        if (!QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:defaultapps"))))
            QMessageBox::warning(&dialog,QStringLiteral("无法打开设置"),QStringLiteral("请手动打开 Windows 设置 → 应用 → 默认应用。"));
    });
    CacheStore cache;
    auto* cacheSize = dialog.findChild<QLabel*>("cacheSize");
    const auto updateSize = [&cache,cacheSize] { cacheSize->setText(QStringLiteral("%1 MB").arg(cache.size()/1048576.0,0,'f',2)); };
    updateSize();
    auto* clearCache = dialog.findChild<QPushButton*>("clearCache"); clearCache->setEnabled(true);
    connect(clearCache,&QPushButton::clicked,&dialog,[&dialog,&cache,updateSize] {
        if (QMessageBox::question(&dialog,QStringLiteral("清理缓存"),QStringLiteral("仅删除 Aidfame 专用缓存，是否继续？"),
            QMessageBox::Yes | QMessageBox::No,QMessageBox::No) != QMessageBox::Yes) return;
        if (!cache.clear()) QMessageBox::warning(&dialog,QStringLiteral("无法清理"),cache.error.isEmpty() ? QStringLiteral("缓存目录不可用。") : cache.error);
        updateSize();
    });
    dialog.findChild<QCheckBox*>("showInfo")->setChecked(showInfo_);
    dialog.findChild<QCheckBox*>("resumeHistory")->setChecked(store_.settings["resumeHistory"].toBool());
    auto* step = dialog.findChild<QComboBox*>("seekStep"); step->setCurrentIndex(step->findData(seekStep_));
    dialog.findChild<QComboBox*>("screenshotFormat")->setCurrentText(screenshotFormat_.toUpper());
    auto* decoder = dialog.findChild<QComboBox*>("hardwareDecoder"); decoder->setCurrentIndex(decoder->findData(store_.settings["hardwareDecoder"].toString()));
    for (const auto* name : {"screenshotDirectory","recordingDirectory"}) {
        auto* input = dialog.findChild<QLineEdit*>(name); input->setText(store_.settings[name].toString());
        connect(dialog.findChild<QPushButton*>(QString::fromLatin1(name)+"Browse"),&QPushButton::clicked,&dialog,[&dialog,input] {
            const auto path = QFileDialog::getExistingDirectory(&dialog,QStringLiteral("保存目录"),input->text());
            if (!path.isEmpty()) input->setText(path);
        });
    }
    if (dialog.exec() != QDialog::Accepted) return;
    store_.settings["showMediaInfo"] = dialog.findChild<QCheckBox*>("showInfo")->isChecked();
    store_.settings["resumeHistory"] = dialog.findChild<QCheckBox*>("resumeHistory")->isChecked();
    store_.settings["seekStepSeconds"] = step->currentData().toInt();
    store_.settings["screenshotFormat"] = dialog.findChild<QComboBox*>("screenshotFormat")->currentText().toLower();
    store_.settings["hardwareDecoder"] = decoder->currentData().toString();
    for (const auto* name : {"screenshotDirectory","recordingDirectory"}) store_.settings[name] = dialog.findChild<QLineEdit*>(name)->text();
    store_.settings = LocalStore::validated(store_.settings); applySettings(); saveState();
}

bool MainWindow::setSeekStep(int seconds) {
    if (seconds != 5 && seconds != 10 && seconds != 15 && seconds != 20 && seconds != 30) return false;
    seekStep_ = seconds;
    findChild<QAction*>(QStringLiteral("seekStep%1").arg(seconds))->setChecked(true);
    return true;
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    const auto* widget = qobject_cast<QWidget*>(watched);
    if (!widget || widget->window() != this || QApplication::activeModalWidget()
        || QApplication::activePopupWidget() || qobject_cast<const QComboBox*>(widget)
        || qobject_cast<const QLineEdit*>(widget) || !player_
        || !findChild<QAction*>("playAction")->isEnabled()) return QMainWindow::eventFilter(watched, event);
    if (event->type() != QEvent::KeyPress && event->type() != QEvent::ShortcutOverride)
        return QMainWindow::eventFilter(watched, event);
    auto* key = static_cast<QKeyEvent*>(event);
    if (key->modifiers() != Qt::NoModifier || key->key() < Qt::Key_Left || key->key() > Qt::Key_Down)
        return QMainWindow::eventFilter(watched, event);
    if (event->type() == QEvent::ShortcutOverride) { key->accept(); return true; }
    if (key->key() == Qt::Key_Left || key->key() == Qt::Key_Right) {
        if (recorder_->capturing()) recorder_->stop(position_);
        player_->seek(key->key() == Qt::Key_Left ? -seekStep_ : seekStep_);
    } else if (!recorder_->capturing()) {
        auto* volume = findChild<QSlider*>("volumeSlider");
        volume->setValue(volume->value() + (key->key() == Qt::Key_Up ? 5 : -5));
    }
    key->accept();
    return true;
}

void MainWindow::toggleRecording() {
    if (!player_) return;
    if (recorder_->capturing()) { recorder_->stop(player_->numberProperty("time-pos")); return; }
    RecordingOptions options;
    options.speed = player_->numberProperty("speed");
    options.height = findChild<QComboBox*>("qualityCombo")->currentData().toInt();
    options.aspect = aspect_;
    options.displayAspect = double(videoSurface_->width()) / qMax(1, videoSurface_->height());
    options.volume = muted_ ? 0 : player_->numberProperty("volume") / 100.0;
    if (recorder_->start(currentFile_, player_->numberProperty("time-pos"), recordingDirectory_, options))
        statusBar()->showMessage(QStringLiteral("正在记录播放区间 · 停止后生成 MP4 · 暂停时间不计入"));
}

void MainWindow::refreshRecording() {
    const bool loaded = findChild<QAction*>("playAction")->isEnabled();
    const bool capture = recorder_->capturing();
    for (const auto* name : {"volumeSlider", "volumeButton", "speedCombo", "qualityCombo"})
        findChild<QWidget*>(name)->setEnabled(loaded && !capture);
    findChild<QMenu*>("aspectMenu")->setEnabled(loaded && !capture);
    auto* button = findChild<QToolButton*>("recordButton");
    button->setEnabled(loaded && (!recorder_->busy() || capture));
    button->setCheckable(true);
    button->setChecked(capture);
    button->setToolTip(capture ? QStringLiteral("停止录制并生成 MP4") : recorder_->busy() ? QStringLiteral("正在生成 MP4…") : QStringLiteral("开始录制"));
    button->setStyleSheet(capture ? QStringLiteral("background: #94393d;") : QString());
    if (recorder_->busy() && !capture) statusBar()->showMessage(QStringLiteral("正在生成 MP4，请稍候…"));
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (recorder_->busy()) {
        if (QMessageBox::question(this, QStringLiteral("录制尚未完成"),
            QStringLiteral("关闭会取消当前录制并删除未完成文件。确定关闭？"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) { event->ignore(); return; }
        recorder_->cancel();
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::chooseFile() {
    if (staticPreview_) { showStageNotice(); return; }
    const auto path = QFileDialog::getOpenFileName(this, QStringLiteral("打开本地视频"), {},
        QStringLiteral("视频文件 (*.mp4 *.mkv *.mov *.avi *.flv *.wmv *.ts *.mts *.m2ts *.webm *.mxf)"));
    if (!path.isEmpty()) openFile(path);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    const auto urls = event->mimeData()->urls();
    if (urls.size() == 1 && urls.first().isLocalFile()
        && MpvPlayer::validateLocalFile(urls.first().toLocalFile()).isEmpty()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event) {
    const auto urls = event->mimeData()->urls();
    if (urls.size() == 1 && urls.first().isLocalFile()) { openFile(urls.first().toLocalFile()); event->acceptProposedAction(); }
}

void MainWindow::setPlaybackEnabled(bool enabled) {
    for (const auto* name : {"playButton", "seekSlider", "volumeSlider", "volumeButton", "speedCombo", "qualityCombo", "screenshotButton"})
        findChild<QWidget*>(name)->setEnabled(enabled);
    findChild<QAction*>("playAction")->setEnabled(enabled);
    findChild<QMenu*>("aspectMenu")->setEnabled(enabled);
    if (recorder_) refreshRecording();
}

void MainWindow::refreshTime() {
    const auto format = [](double time) {
        const auto seconds = qMax(0, static_cast<int>(time));
        if (seconds >= 3600) return QStringLiteral("%1:%2:%3").arg(seconds / 3600, 2, 10, QLatin1Char('0'))
            .arg((seconds / 60) % 60, 2, 10, QLatin1Char('0')).arg(seconds % 60, 2, 10, QLatin1Char('0'));
        return QStringLiteral("%1:%2").arg(seconds / 60, 2, 10, QLatin1Char('0')).arg(seconds % 60, 2, 10, QLatin1Char('0'));
    };
    findChild<QLabel*>("timeLabel")->setText(format(position_) + QStringLiteral(" / ") + format(duration_));
    auto* seek = findChild<QSlider*>("seekSlider");
    if (!seek->isSliderDown()) seek->setValue(duration_ > 0 ? qBound(0, static_cast<int>(position_ / duration_ * 1000), 1000) : 0);
}

void MainWindow::openFile(const QString& path) {
    if (staticPreview_) return;
    const auto validation = MpvPlayer::validateLocalFile(path);
    if (!validation.isEmpty()) { statusBar()->showMessage(validation); return; }
    if (recorder_->capturing()) recorder_->stop(position_);
    saveState(); fileReady_ = false; resumePosition_ = 0; rememberActive_ = true;
    currentFile_ = path;
    if (store_.settings["resumeHistory"].toBool()) for (const auto& item : store_.history) {
        const auto entry = item.toObject();
        if (entry["path"].toString().compare(path,Qt::CaseInsensitive) == 0
            && entry["durationSeconds"].toDouble() - entry["positionSeconds"].toDouble() > 3)
            resumePosition_ = entry["positionSeconds"].toDouble();
    }
    if (!player_) {
        player_ = new MpvPlayer(this);
        connect(player_, &MpvPlayer::screenshotSaved, this, [this](const QString& path) {
            statusBar()->showMessage(QStringLiteral("截图已保存：%1").arg(QDir::toNativeSeparators(path)), 10000);
        });
        connect(player_, &MpvPlayer::screenshotFailed, this, [this](const QString& error) { statusBar()->showMessage(error, 10000); });
        connect(player_, &MpvPlayer::errorOccurred, this, [this](const QString& error) {
            if (recorder_->capturing()) recorder_->stop(position_);
            statusBar()->showMessage(error);
            setPlaybackEnabled(false);
            viewportStack_->setCurrentIndex(0);
        });
        connect(player_, &MpvPlayer::playbackEnded, this, [this] {
            if (recorder_->capturing()) recorder_->stop(position_);
        });
        connect(player_, &MpvPlayer::positionChanged, this, [this](double value) {
            if (recorder_->capturing() && duration_ > 0 && value >= duration_ - 0.05) recorder_->stop(duration_);
        });
        connect(player_, &MpvPlayer::fileLoaded, this, [this] {
            fileReady_ = true;
            viewportStack_->setCurrentIndex(1);
            setPlaybackEnabled(true);
            auto* quality = findChild<QComboBox*>("qualityCombo");
            const QSignalBlocker blocker(quality);
            quality->clear();
            quality->addItem(QStringLiteral("原画"), 0);
            const double sourceHeight = player_->numberProperty("video-params/h");
            for (const int height : {1080, 720, 480}) {
                if (sourceHeight > height) quality->addItem(QString::number(height) + QStringLiteral("P"), height);
            }
            player_->setResolutionCap(0);
            applySettings();
            if (resumePosition_ > 0) player_->seek(resumePosition_,true);
            refreshHistory();
            const auto decoder = player_->textProperty("hwdec-current");
            statusBar()->showMessage(QStringLiteral("正在播放  ·  解码：%1")
                .arg(decoder.isEmpty() || decoder == QStringLiteral("no") ? QStringLiteral("CPU") : decoder));
        });
        connect(player_, &MpvPlayer::positionChanged, this, [this](double value) { position_ = value; refreshTime(); });
        connect(player_, &MpvPlayer::durationChanged, this, [this](double value) { duration_ = value; refreshTime(); });
        connect(player_, &MpvPlayer::pausedChanged, this, [this](bool value) {
            paused_ = value;
            auto* button = findChild<QToolButton*>("playButton");
            button->setIcon(QIcon(value ? QStringLiteral(":/icons/play.svg") : QStringLiteral(":/icons/pause.svg")));
            button->setToolTip(value ? QStringLiteral("播放") : QStringLiteral("暂停"));
        });
        connect(player_, &MpvPlayer::decoderChanged, this, [this](const QString& decoder) {
            statusBar()->showMessage(QStringLiteral("本地播放  ·  解码：%1")
                .arg(decoder.isEmpty() || decoder == QStringLiteral("no") ? QStringLiteral("CPU") : decoder));
        });
    }
    viewportStack_->setCurrentIndex(1);
    if (!player_->initialize(static_cast<quintptr>(videoSurface_->winId()))) {
        viewportStack_->setCurrentIndex(0);
        return;
    }
    setPlaybackEnabled(false);
    duration_ = position_ = 0;
    refreshTime();
    statusBar()->showMessage(QStringLiteral("正在打开…"));
    if (player_->openLocalFile(path)) setWindowTitle(QFileInfo(path).fileName() + QStringLiteral(" — Aidfame Player"));
}

void MainWindow::toggleFullscreen() {
    if (mini_) toggleMini();
    if (isFullScreen()) {
        if (wasMaximized_) showMaximized();
        else showNormal();
        menuBar()->show();
        statusBar()->show();
    } else {
        wasMaximized_ = isMaximized();
        menuBar()->hide();
        statusBar()->hide();
        showFullScreen();
    }
}

void MainWindow::toggleMini() {
    if (!mini_ && isFullScreen()) toggleFullscreen();
    if (!mini_) {
        normalGeometry_ = saveGeometry();
        showNormal(); setMinimumSize(360,300);
        menuBar()->hide(); statusBar()->hide();
    }
    mini_ = !mini_;
    for (const auto* name : {"brandMark","brandTitle","subtitle","emptyHistory","stageLabel"})
        findChild<QWidget*>(name)->setVisible(!mini_);
    for (const auto* name : {"volumeButton","volumeSlider","speedCombo","qualityCombo","screenshotButton","recordButton"})
        findChild<QWidget*>(name)->setVisible(!mini_);
    if (mini_) resize(480,320);
    else { setMinimumSize(800,500); restoreGeometry(normalGeometry_); menuBar()->show(); statusBar()->show(); }
    // Preserve the native child HWND and its active D3D11 session.
    SetWindowPos(reinterpret_cast<HWND>(winId()),mini_ ? HWND_TOPMOST : HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void MainWindow::showStageNotice() {
    QMessageBox::information(this, QStringLiteral("窗口开发版"),
        QStringLiteral("当前为 Stage 1 窗口版本。\n下一阶段接入 mpv 后启用本地视频播放。"));
}

} // namespace aidfame
