#include "Player/MpvPlayer.h"
#include "App/WindowsRuntime.h"
#include <QApplication>
#include <QDir>
#include <QWidget>
#include <QTest>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSaveFile>

class MediaTests final : public QObject {
    Q_OBJECT
private slots:
    void realMediaDirectory() {
        const auto directory = qEnvironmentVariable("AIDFAME_MEDIA_DIR");
        if (directory.isEmpty()) QSKIP("Set AIDFAME_MEDIA_DIR for read-only real-media validation.");
        const auto files = QDir(directory).entryInfoList({"*.mp4","*.mov","*.mkv","*.mxf","*.avi","*.webm","*.ts","*.flv","*.wmv","*.mts","*.m2ts"},QDir::Files,QDir::Name);
        QVERIFY(!files.isEmpty());
        QWidget surface; surface.resize(960,540); surface.show();
        QVERIFY(QTest::qWaitForWindowExposed(&surface));
        aidfame::MpvPlayer player; QVERIFY(player.initialize(surface.winId()));
        QSignalSpy loaded(&player,&aidfame::MpvPlayer::fileLoaded), errors(&player,&aidfame::MpvPlayer::errorOccurred);
        QJsonArray results;
        const bool fullPlayback = qEnvironmentVariableIntValue("AIDFAME_FULL_MEDIA") == 1;
        for (const auto& file : files) {
            const auto startFile = qEnvironmentVariable("AIDFAME_MEDIA_START");
            if (!startFile.isEmpty() && file.fileName() < startFile) continue;
            loaded.clear(); errors.clear();
            const auto diagnostic = [&] {
                return QStringLiteral("file=%1 position=%2 duration=%3 pause=%4 eof=%5 errors=%6")
                    .arg(file.fileName()).arg(player.numberProperty("time-pos")).arg(player.numberProperty("duration"))
                    .arg(player.textProperty("pause"),player.textProperty("eof-reached")).arg(errors.count());
            };
            QVERIFY(player.openLocalFile(file.absoluteFilePath()));
            QTRY_COMPARE_WITH_TIMEOUT(loaded.count(),1,20000);
            player.setMuted(true);
            QTRY_VERIFY2_WITH_TIMEOUT(player.numberProperty("time-pos") > 0.2,qPrintable(diagnostic()),15000);
            const double duration = player.numberProperty("duration");
            const auto seek = qMin(5.0,duration/3);
            if (fullPlayback) {
                QVERIFY(duration > 0 && duration < 3600);
                QTRY_VERIFY2_WITH_TIMEOUT(player.textProperty("eof-reached") == QStringLiteral("yes"),qPrintable(diagnostic()),static_cast<int>(duration*1000)+20000);
            } else {
                QVERIFY(player.seek(seek,true));
                QTRY_VERIFY_WITH_TIMEOUT(player.numberProperty("time-pos") >= seek,10000);
                QTest::qWait(1500);
            }
            QVERIFY2(errors.isEmpty(),qPrintable(file.fileName()));
            QVERIFY(player.numberProperty("time-pos") > seek+0.5 || duration < 1);
            const auto result = QJsonObject{{"file",file.fileName()},{"duration",duration},
                {"width",player.numberProperty("width")},{"height",player.numberProperty("height")},
                {"fps",player.numberProperty("container-fps")},{"codec",player.textProperty("video-codec")},
                {"pixelFormat",player.textProperty("video-params/pixelformat")},{"decoder",player.textProperty("hwdec-current")},
                {"droppedFrames",player.numberProperty("frame-drop-count")},{"avSync",player.numberProperty("avsync")},
                {"samplePassed",true},{"fullPlayback",fullPlayback},
                {"endPosition",player.numberProperty("time-pos")},{"eofReached",player.textProperty("eof-reached") == "yes"}};
            results.append(result);
            qInfo().noquote() << QJsonDocument(result).toJson(QJsonDocument::Compact);
            const auto report = qEnvironmentVariable("AIDFAME_MEDIA_REPORT");
            if (!report.isEmpty()) { QSaveFile output(report); QVERIFY(output.open(QIODevice::WriteOnly)); output.write(QJsonDocument(results).toJson()); QVERIFY(output.commit()); }
        }
    }
};
int main(int argc, char** argv) {
    aidfame::WindowsRuntime runtime; if (!runtime.valid()) return 1;
    QApplication app(argc,argv); MediaTests tests; return QTest::qExec(&tests,argc,argv);
}
#include "MediaTests.moc"
