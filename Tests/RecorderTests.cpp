#include "Recorder/Recorder.h"
#include "App/WindowsRuntime.h"
#include <QApplication>
#include "Player/MpvPlayer.h"
#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QWidget>
#include <QDir>
#include <QFile>
#include <QRegularExpression>

class RecorderTests : public QObject {
    Q_OBJECT
private slots:
    void invalidOutputReportsFailure() {
        aidfame::Recorder recorder; recorder.setEncoder(QStringLiteral(ENCODER_PATH));
        QSignalSpy failed(&recorder,&aidfame::Recorder::failed);
        QVERIFY(!recorder.start(QStringLiteral(MEDIA_DIR "/test-h264-1080p.mp4"),0,"//server/share",{}));
        QCOMPARE(failed.count(),1); QVERIFY(!failed.first().first().toString().isEmpty());
        QVERIFY(!recorder.busy());
    }
    void synchronizedFlashAndTone() {
        QTemporaryDir source;
        const auto fixture = QDir(source.path()).filePath("sync.mp4");
        QProcess generator;
        generator.start(QStringLiteral(ENCODER_PATH), {"-hide_banner","-loglevel","error","-f","lavfi","-i",
            "color=black:size=320x240:rate=25:duration=8","-f","lavfi","-i",
            "aevalsrc=if(between(t\\,2\\,2.2)\\,0.8*sin(2*PI*1000*t)\\,0):s=48000:d=8",
            "-vf","drawbox=color=white:t=fill:enable='between(t,2,2.2)'","-c:v","libx264","-preset","ultrafast",
            "-threads","2","-c:a","aac","-n",fixture});
        QVERIFY(generator.waitForFinished(30000)); QCOMPARE(generator.exitCode(),0);
        for (double speed : {0.5,1.0,2.0,5.0}) {
            QTemporaryDir output;
            aidfame::Recorder recorder; recorder.setEncoder(QStringLiteral(ENCODER_PATH));
            QSignalSpy done(&recorder,&aidfame::Recorder::completed), failed(&recorder,&aidfame::Recorder::failed);
            aidfame::RecordingOptions options; options.speed = speed;
            QVERIFY(recorder.start(fixture,1,output.path(),options)); QVERIFY(recorder.stop(7));
            QTRY_VERIFY_WITH_TIMEOUT(done.count() == 1 || !failed.isEmpty(),30000);
            QVERIFY(failed.isEmpty()); QCOMPARE(done.count(),1);
            QProcess probe; probe.setProcessChannelMode(QProcess::MergedChannels);
            probe.start(QStringLiteral(ENCODER_PATH), {"-hide_banner","-i",done.first().first().toString(),
                "-vf","blackdetect=d=0.01:pix_th=0.5:pic_th=0.99","-af","silencedetect=noise=-35dB:d=0.01","-f","null","NUL"});
            QVERIFY(probe.waitForFinished(30000)); QCOMPARE(probe.exitCode(),0);
            const auto diagnostics = QString::fromUtf8(probe.readAll());
            const auto video = QRegularExpression("black_end:([0-9.]+)").match(diagnostics);
            const auto audio = QRegularExpression("silence_end: ([0-9.]+)").match(diagnostics);
            QVERIFY2(video.hasMatch() && audio.hasMatch(),qPrintable(diagnostics));
            const double videoTime = video.captured(1).toDouble(), audioTime = audio.captured(1).toDouble();
            qInfo() << "Speed" << speed << "flash" << videoTime << "tone" << audioTime;
            QVERIFY(qAbs(videoTime - 1/speed) <= 0.10);
            QVERIFY(qAbs(videoTime - audioTime) <= 0.10);
        }
    }
    void sourceLossIsReportedAndCleaned() {
        QTemporaryDir root;
        const auto source = QDir(root.path()).filePath("source.mp4");
        QVERIFY(QFile::copy(QStringLiteral(MEDIA_DIR "/test-h264-1080p.mp4"),source));
        const auto output = QDir(root.path()).filePath("output");
        aidfame::Recorder recorder; recorder.setEncoder(QStringLiteral(ENCODER_PATH));
        QSignalSpy failed(&recorder,&aidfame::Recorder::failed), done(&recorder,&aidfame::Recorder::completed);
        QVERIFY(recorder.start(source,0,output,{}));
        QVERIFY(QFile::remove(source));
        QVERIFY(recorder.stop(2));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(),1,10000);
        QCOMPARE(done.count(),0); QVERIFY(!recorder.busy());
        QCOMPARE(QDir(output).entryList(QDir::Files).size(),0);
    }
    void encodeInterval_data() {
        QTest::addColumn<double>("speed");
        for (double speed : {0.5, 1.0, 2.0, 5.0}) QTest::newRow(qPrintable(QString::number(speed))) << speed;
    }
    void encodeInterval() {
        QFETCH(double, speed);
        QTemporaryDir output;
        aidfame::Recorder recorder;
        recorder.setEncoder(QStringLiteral(ENCODER_PATH));
        QSignalSpy done(&recorder, &aidfame::Recorder::completed);
        QSignalSpy errors(&recorder, &aidfame::Recorder::failed);
        aidfame::RecordingOptions options;
        options.speed = speed; options.height = 480;
        QVERIFY(recorder.start(QStringLiteral(MEDIA_DIR "/test-h264-1080p.mp4"), 1, output.path(), options));
        QVERIFY(recorder.capturing());
        QVERIFY(!recorder.start(QStringLiteral(MEDIA_DIR "/test-h264-1080p.mp4"), 1, output.path(), options));
        QVERIFY(recorder.stop(5));
        QTRY_VERIFY_WITH_TIMEOUT(done.count() == 1 || errors.count() > 0, 60000);
        QCOMPARE(errors.count(), 0);
        QCOMPARE(done.count(), 1);
        QWidget surface; surface.show();
        aidfame::MpvPlayer player;
        QSignalSpy loaded(&player, &aidfame::MpvPlayer::fileLoaded);
        QVERIFY(player.initialize(surface.winId(), true));
        QVERIFY(player.openLocalFile(done.first().first().toString()));
        QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, 10000);
        QTRY_VERIFY(player.numberProperty("duration") > 0);
        QVERIFY(qAbs(player.numberProperty("duration") - 4.0 / speed) < 0.25);
        QCOMPARE(player.numberProperty("height"), 480.0);
        QVERIFY(!player.textProperty("audio-codec-name").isEmpty());
    }
    void cancelAndInvalidInput() {
        QTemporaryDir output;
        aidfame::Recorder recorder;
        recorder.setEncoder(QStringLiteral(ENCODER_PATH));
        QVERIFY(!recorder.start("https://example.com/a.mp4", 0, output.path(), {}));
        QVERIFY(recorder.start(QStringLiteral(MEDIA_DIR "/test-h264-1080p.mp4"), 1, output.path(), {}));
        recorder.cancel();
        QVERIFY(!recorder.busy());
        QCOMPARE(QDir(output.path()).entryList(QDir::Files).size(), 0);
    }
};
int main(int argc, char** argv) {
    aidfame::WindowsRuntime runtime;
    if (!runtime.valid()) return 1;
    QApplication app(argc,argv);
    RecorderTests tests;
    return QTest::qExec(&tests,argc,argv);
}
#include "RecorderTests.moc"
