#include "UI/MainWindow.h"
#include "App/WindowsRuntime.h"
#include "Player/MpvPlayer.h"
#include "Recorder/Recorder.h"
#include "UI/SettingsDialog.h"
#include <QTabWidget>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QLabel>
#include <QMenuBar>
#include <QPushButton>
#include <QSlider>
#include <QTest>
#include <QToolButton>
#include <QTemporaryDir>
#include <QSignalSpy>

class UiTests final : public QObject {
    Q_OBJECT

private slots:
    void miniRestoresLayout() {
        aidfame::MainWindow window(nullptr,true); window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const auto before = window.size();
        const auto child = window.findChild<QWidget*>("videoSurface")->winId();
        window.toggleMini();
        QCOMPARE(window.size(),QSize(480,320));
        QVERIFY(GetWindowLongPtrW(reinterpret_cast<HWND>(window.winId()),GWL_EXSTYLE) & WS_EX_TOPMOST);
        QVERIFY(window.menuBar()->isHidden());
        QVERIFY(window.findChild<QComboBox*>("speedCombo")->isHidden());
        QCOMPARE(window.findChild<QWidget*>("videoSurface")->winId(),child);
        const auto output = qEnvironmentVariable("AIDFAME_QA_DIR");
        if (!output.isEmpty()) QVERIFY(window.grab().save(QDir(output).filePath("mini.png")));
        window.toggleMini(); QCOMPARE(window.size(),before);
        QVERIFY(!(GetWindowLongPtrW(reinterpret_cast<HWND>(window.winId()),GWL_EXSTYLE) & WS_EX_TOPMOST));
        QVERIFY(!window.menuBar()->isHidden());
    }
    void miniPreservesPlayback() {
        QTemporaryDir root;
        aidfame::MainWindow window(nullptr,false,root.path()); window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.openFile(QStringLiteral(MEDIA_DIR "/test-h264-1080p.mp4"));
        auto* player = window.findChild<aidfame::MpvPlayer*>(); QVERIFY(player);
        QSignalSpy errors(player,&aidfame::MpvPlayer::errorOccurred);
        QTRY_VERIFY_WITH_TIMEOUT(player->numberProperty("time-pos") > 0.2,10000);
        const auto child = window.findChild<QWidget*>("videoSurface")->winId();
        const double before = player->numberProperty("time-pos");
        window.toggleMini(); window.resize(560,360);
        QTRY_VERIFY(player->numberProperty("time-pos") > before+0.5);
        QCOMPARE(window.findChild<QWidget*>("videoSurface")->winId(),child);
        window.toggleFullscreen();
        QTRY_VERIFY(window.isFullScreen());
        QVERIFY(!(GetWindowLongPtrW(reinterpret_cast<HWND>(window.winId()),GWL_EXSTYLE) & WS_EX_TOPMOST));
        window.toggleFullscreen();
        QVERIFY(errors.isEmpty());
    }
    void historyResumesAndPersists() {
        QTemporaryDir root;
        aidfame::LocalStore store(root.path());
        const auto path = QStringLiteral(MEDIA_DIR "/test-h264-1080p.mp4");
        store.remember(path,7,12); store.settings["seekStepSeconds"] = 20; QVERIFY(store.save());
        {
            aidfame::MainWindow window(nullptr,false,root.path()); window.show();
            QVERIFY(QTest::qWaitForWindowExposed(&window));
            QCOMPARE(window.seekStep(),20);
            QVERIFY(window.findChild<QLabel*>("emptyHistory")->text().contains("test-h264-1080p"));
            window.openFile(path);
            auto* player = window.findChild<aidfame::MpvPlayer*>(); QVERIFY(player);
            QTRY_VERIFY_WITH_TIMEOUT(player->numberProperty("time-pos") >= 7,4000);
            QVERIFY(player->setPaused(true));
        }
        QVERIFY(store.load());
        QCOMPARE(store.history.size(),1);
        QVERIFY(store.history.first().toObject()["positionSeconds"].toDouble() >= 7);
    }
    void settingsStaticLayout() {
        aidfame::SettingsDialog dialog;
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        auto* tabs = dialog.findChild<QTabWidget*>("settingsTabs");
        QCOMPARE(tabs->count(), 7);
        const auto output = qEnvironmentVariable("AIDFAME_QA_DIR");
        for (int index = 0; index < tabs->count(); ++index) {
            tabs->setCurrentIndex(index); QTest::qWait(50);
            for (auto* child : tabs->currentWidget()->findChildren<QWidget*>()) {
                if (child->isVisible() && child->parentWidget() == tabs->currentWidget())
                    QVERIFY(tabs->currentWidget()->rect().contains(child->geometry()));
            }
            if (!output.isEmpty()) QVERIFY(dialog.grab().save(QDir(output).filePath(QStringLiteral("settings-%1-%2.png").arg(index).arg(qEnvironmentVariable("QT_SCALE_FACTOR","1")))));
        }
    }
    void shortcutsDuringPlayback() {
        QTemporaryDir settings;
        aidfame::MainWindow window(nullptr,false,settings.path());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        for (int value : {5, 10, 15, 20, 30}) { QVERIFY(window.setSeekStep(value)); QCOMPARE(window.seekStep(), value); }
        for (int value : {0, 4, 6, 31}) QVERIFY(!window.setSeekStep(value));
        QVERIFY(window.setSeekStep(5));
        window.openFile(QStringLiteral(MEDIA_DIR "/test-h264-1080p.mp4"));
        auto* player = window.findChild<aidfame::MpvPlayer*>();
        QVERIFY(player);
        QTRY_VERIFY_WITH_TIMEOUT(window.findChild<QAction*>("playAction")->isEnabled(), 15000);
        QVERIFY(player->setPaused(true));
        QTRY_COMPARE(player->textProperty("pause"), QStringLiteral("yes"));
        QVERIFY(player->seek(1, true));
        QTRY_VERIFY(qAbs(player->numberProperty("time-pos") - 1) < 0.2);
        QTest::keyClick(&window, Qt::Key_Right);
        QTRY_VERIFY(qAbs(player->numberProperty("time-pos") - 6) < 0.2);
        QTest::keyClick(&window, Qt::Key_Left);
        QTRY_VERIFY(qAbs(player->numberProperty("time-pos") - 1) < 0.2);
        auto* volume = window.findChild<QSlider*>("volumeSlider");
        volume->setValue(98);
        QTest::keyClick(volume, Qt::Key_Up);
        QCOMPARE(volume->value(), 100);
        QTest::keyClick(volume, Qt::Key_Down);
        QCOMPARE(volume->value(), 95);
        volume->setValue(2);
        QTest::keyClick(volume, Qt::Key_Down);
        QCOMPARE(volume->value(), 0);
        auto* combo = window.findChild<QComboBox*>("speedCombo");
        const auto before = combo->currentIndex();
        QTest::keyClick(combo, Qt::Key_Down);
        QCOMPARE(combo->currentIndex(), before + 1);
        QTemporaryDir captures;
        auto* recorder = window.findChild<aidfame::Recorder*>();
        QVERIFY(recorder);
        QSignalSpy complete(recorder, &aidfame::Recorder::completed);
        QVERIFY(recorder->start(QStringLiteral(MEDIA_DIR "/test-h264-1080p.mp4"), 0, captures.path(), {}));
        QVERIFY(!combo->isEnabled());
        QVERIFY(!volume->isEnabled());
        QTest::keyClick(&window, Qt::Key_Right);
        QVERIFY(!recorder->capturing());
        QTRY_COMPARE_WITH_TIMEOUT(complete.count(), 1, 30000);
        QVERIFY(combo->isEnabled());
    }

    void initialStateIsHonest() {
        QTemporaryDir root;
        aidfame::MainWindow window(nullptr, false, root.path());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QCOMPARE(window.menuBar()->actions().size(), 3);
        QCOMPARE(window.findChild<QComboBox*>("speedCombo")->currentText(), QStringLiteral("1x"));
        QCOMPARE(window.findChild<QComboBox*>("qualityCombo")->count(), 1);
        QCOMPARE(window.findChild<QSlider*>("volumeSlider")->value(), 80);
        QVERIFY(!window.findChild<QSlider*>("seekSlider")->isEnabled());
        for (const auto* name : {"playButton", "screenshotButton", "recordButton"}) {
            auto* button = window.findChild<QToolButton*>(name);
            QVERIFY(button);
            QVERIFY(!button->isEnabled());
            QVERIFY(!button->icon().isNull());
            QVERIFY(!button->accessibleName().isEmpty());
        }
        QVERIFY(window.findChild<QPushButton*>("openButton")->isEnabled());
        const auto output = qEnvironmentVariable("AIDFAME_QA_DIR");
        if (!output.isEmpty()) QVERIFY(window.grab().save(QDir(output).filePath(QStringLiteral("internal-empty-%1.png").arg(qEnvironmentVariable("QT_SCALE_FACTOR","1")))));
    }

    void fullscreenRestoresWindowState() {
        aidfame::MainWindow window(nullptr, true);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.findChild<QAction*>("fullscreenAction")->trigger();
        QTRY_VERIFY(window.isFullScreen());
        QVERIFY(window.menuBar()->isHidden());
        window.toggleFullscreen();
        QTRY_VERIFY(!window.isFullScreen());
        QVERIFY(!window.isMaximized());
        QVERIFY(!window.menuBar()->isHidden());
        window.showMaximized();
        QTRY_VERIFY(window.isMaximized());
        window.toggleFullscreen();
        window.toggleFullscreen();
        QTRY_VERIFY(window.isMaximized());
    }

    void layoutAndCapture_data() {
        QTest::addColumn<QSize>("canvas");
        QTest::newRow("desktop") << QSize(1280, 800);
        QTest::newRow("minimum") << QSize(800, 500);
    }

    void layoutAndCapture() {
        QFETCH(QSize, canvas);
        aidfame::MainWindow window(nullptr, true);
        window.resize(canvas);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTest::qWait(100);
        qInfo() << "Requested canvas" << canvas << "actual" << window.size()
                << "devicePixelRatio" << window.devicePixelRatioF();
        const auto* transport = window.findChild<QWidget*>("transport");
        QVERIFY(transport);
        QList<QWidget*> controls;
        for (const auto* name : {"playButton", "timeLabel", "volumeButton", "volumeSlider",
                                "speedCombo", "qualityCombo", "screenshotButton", "recordButton", "fullscreenButton"}) {
            auto* control = window.findChild<QWidget*>(name);
            QVERIFY(control);
            QVERIFY(control->isVisible());
            QVERIFY2(transport->rect().contains(control->geometry()), name);
            for (auto* previous : controls) {
                QVERIFY2(!previous->geometry().intersects(control->geometry()), "Transport controls overlap");
            }
            controls.append(control);
        }
        auto* title = window.findChild<QLabel*>("brandTitle");
        QVERIFY(title->width() >= title->fontMetrics().horizontalAdvance(title->text()));
        const auto output = qEnvironmentVariable("AIDFAME_QA_DIR");
        if (!output.isEmpty()) {
            QVERIFY(QDir().mkpath(output));
            const auto name = QStringLiteral("stage1-%1-%2.png")
                .arg(QString::fromLatin1(QTest::currentDataTag()), qEnvironmentVariable("QT_SCALE_FACTOR", "1"));
            QVERIFY(window.grab().save(QDir(output).filePath(name)));
        }
    }
};

int main(int argc, char** argv) {
    aidfame::WindowsRuntime runtime;
    if (!runtime.valid()) return 1;
    QApplication app(argc, argv);
    QApplication::setStyle(QStringLiteral("Fusion"));
    UiTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "UiTests.moc"
