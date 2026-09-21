#include "Player/MpvPlayer.h"
#include "App/WindowsRuntime.h"

#include <QApplication>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>
#include <QTemporaryDir>
#include <QImageReader>
#include <QDir>
#include <memory>
#include <cmath>

class PlaybackTests final : public QObject {
    Q_OBJECT
private slots:
    void damagedFileDoesNotPoisonNextOpen() {
        QTemporaryDir root;
        QWidget surface; surface.show();
        aidfame::MpvPlayer player; QVERIFY(player.initialize(surface.winId()));
        QSignalSpy errors(&player,&aidfame::MpvPlayer::errorOccurred);
        const auto path = QDir(root.path()).filePath("truncated.mp4");
        QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QByteArray::fromHex("000000206674797069736f6d0000020069736f6d69736f32617663316d703431"));
        file.close();
        QVERIFY(player.openLocalFile(path));
        QTRY_VERIFY_WITH_TIMEOUT(!errors.isEmpty(),10000);
        errors.clear();
        QVERIFY(player.openLocalFile(QStringLiteral(MEDIA_DIR "/test-h264-1080p.mp4")));
        QVERIFY(player.setMuted(true));
        QTRY_VERIFY_WITH_TIMEOUT(player.numberProperty("time-pos") > 0.5,10000);
        QVERIFY(errors.isEmpty());
    }
    void openAfterEndOfFile() {
        QWidget surface; surface.show();
        aidfame::MpvPlayer player; QVERIFY(player.initialize(surface.winId()));
        QSignalSpy loaded(&player,&aidfame::MpvPlayer::fileLoaded);
        const auto path = qEnvironmentVariable("AIDFAME_EOF_MEDIA",QStringLiteral(MEDIA_DIR "/test-h264-1080p.mp4"));
        for (int cycle = 0; cycle < 12; ++cycle) {
            const auto nextPath = cycle%2 ? qEnvironmentVariable("AIDFAME_EOF_NEXT",path) : path;
            loaded.clear(); QVERIFY(player.openLocalFile(nextPath));
            QTRY_COMPARE_WITH_TIMEOUT(loaded.count(),1,10000);
            QVERIFY(player.setMuted(true));
            QTRY_VERIFY2_WITH_TIMEOUT(player.numberProperty("time-pos") > 0.3,
                qPrintable(QStringLiteral("cycle=%1 pause=%2 eof=%3 position=%4")
                    .arg(cycle).arg(player.textProperty("pause"),player.textProperty("eof-reached"))
                    .arg(player.numberProperty("time-pos"))),5000);
            if (cycle != 0) QVERIFY(player.seek(player.numberProperty("duration")-0.2,true));
            QTRY_COMPARE_WITH_TIMEOUT(player.textProperty("eof-reached"),QStringLiteral("yes"),60000);
        }
    }
    void qsvCodecPreferenceProbe() {
        QWidget surface; surface.show();
        std::unique_ptr<mpv_handle,decltype(&mpv_terminate_destroy)> handle(mpv_create(),mpv_terminate_destroy);
        QVERIFY(handle);
        for (const auto& option : {std::pair{"config","no"},{"load-scripts","no"},{"ytdl","no"},
                {"autoload-files","no"},{"access-references","no"},{"demuxer","lavf"},
                {"demuxer-lavf-o","protocol_whitelist=file"},{"vo","gpu"},{"gpu-api","d3d11"},
                {"gpu-context","d3d11"},{"hwdec","no"},{"vd","h264_qsv,hevc_qsv,av1_qsv,vp9_qsv"},
                {"keep-open","yes"},{"mute","yes"},{"media-controls","no"}})
            QVERIFY(mpv_set_option_string(handle.get(),option.first,option.second) >= 0);
        QVERIFY(mpv_set_option_string(handle.get(),"wid",QByteArray::number(surface.winId()).constData()) >= 0);
        QVERIFY(mpv_initialize(handle.get()) >= 0);
        const auto path = QStringLiteral(MEDIA_DIR "/test-h264-1080p.mp4").toUtf8();
        const char* args[]{"loadfile",path.constData(),nullptr};
        QVERIFY(mpv_command_async(handle.get(),0,args) >= 0);
        const auto position = [&] { double value = 0; mpv_get_property(handle.get(),"time-pos",MPV_FORMAT_DOUBLE,&value); return value; };
        QTRY_VERIFY_WITH_TIMEOUT(position() > 0.5,15000);
        char* codec = mpv_get_property_string(handle.get(),"video-codec");
        qInfo() << "QSV preference probe actual codec:" << (codec ? codec : "unknown");
        mpv_free(codec);
    }
    void repeatedOpenAndRapidSeek() {
        QWidget surface; surface.resize(640,360); surface.show();
        aidfame::MpvPlayer player; QVERIFY(player.initialize(surface.winId()));
        QSignalSpy loaded(&player,&aidfame::MpvPlayer::fileLoaded), errors(&player,&aidfame::MpvPlayer::errorOccurred);
        for (int cycle = 0; cycle < 12; ++cycle) {
            loaded.clear();
            const auto path = QStringLiteral(MEDIA_DIR) + (cycle%2 ? "/test-hevc-4k.mp4" : "/test-h264-1080p.mp4");
            QVERIFY(player.openLocalFile(path)); QTRY_COMPARE_WITH_TIMEOUT(loaded.count(),1,10000);
            QVERIFY(player.setMuted(true)); QVERIFY(player.setPaused(true));
            QTRY_COMPARE(player.textProperty("pause"),QStringLiteral("yes"));
            for (double position : {1,8,2,9,3,7,4,6,5}) QVERIFY(player.seek(position,true));
            QTRY_VERIFY_WITH_TIMEOUT(qAbs(player.numberProperty("time-pos")-5) < 0.15,10000);
            surface.resize(cycle%2 ? QSize(720,480) : QSize(640,360));
            QVERIFY(player.setPaused(false)); QTRY_VERIFY(player.numberProperty("time-pos") > 5.2);
            QVERIFY(errors.isEmpty());
        }
    }
    void rejectsDisguisedRemotePlaylist() {
        QTemporaryDir root;
        const auto path = QDir(root.path()).filePath("not-a-video.mp4");
        QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("#EXTM3U\n#EXT-X-TARGETDURATION:10\n#EXTINF:10,\nhttps://127.0.0.1:9/never-request.ts\n#EXT-X-ENDLIST\n"); file.close();
        QWidget surface; surface.show();
        aidfame::MpvPlayer player; QSignalSpy errors(&player,&aidfame::MpvPlayer::errorOccurred);
        QSignalSpy loaded(&player,&aidfame::MpvPlayer::fileLoaded);
        QVERIFY(player.initialize(surface.winId())); QVERIFY(player.openLocalFile(path));
        QTRY_VERIFY_WITH_TIMEOUT(!errors.isEmpty(),10000);
        QCOMPARE(loaded.count(),0);
    }
    void hardwarePolicies_data() {
        QTest::addColumn<QString>("policy"); QTest::addColumn<QString>("path"); QTest::addColumn<bool>("fallback");
        for (const auto* policy : {"d3d11va","dxva2","nvdec"})
            QTest::newRow(policy) << QString::fromLatin1(policy) << QStringLiteral(MEDIA_DIR "/test-h264-1080p.mp4") << false;
        QTest::newRow("unsupported-profile-cpu-fallback") << QStringLiteral("d3d11va")
            << QStringLiteral(MEDIA_DIR "/../FormatMedia/prores.mov") << true;
    }
    void hardwarePolicies() {
        QFETCH(QString,policy); QFETCH(QString,path); QFETCH(bool,fallback);
        path = QDir::cleanPath(path);
        QVERIFY2(QFileInfo::exists(path),"Run scripts/generate-format-fixtures.ps1 before the extended suite.");
        QWidget surface; surface.show();
        aidfame::MpvPlayer player; QSignalSpy errors(&player,&aidfame::MpvPlayer::errorOccurred);
        QVERIFY(player.initialize(surface.winId())); QVERIFY(player.setHardwareDecoder(policy));
        QVERIFY(player.openLocalFile(path));
        QTRY_VERIFY_WITH_TIMEOUT(player.numberProperty("time-pos") > 0.5,15000);
        const auto actual = player.textProperty("hwdec-current");
        qInfo() << "Requested" << policy << "actual" << actual << "fallback fixture" << fallback;
        if (fallback) QCOMPARE(actual,QStringLiteral("no"));
        else QVERIFY(actual == policy || actual == policy+"-copy" || actual == "no");
        QVERIFY(errors.isEmpty());
    }
    void decoderCapabilities() {
        QWidget surface; surface.show();
        aidfame::MpvPlayer player; QVERIFY(player.initialize(surface.winId()));
        const auto decoders = player.textProperty("decoder-list");
        QVERIFY(!decoders.isEmpty());
        qInfo() << "h264_qsv" << decoders.contains("h264_qsv")
                << "hevc_qsv" << decoders.contains("hevc_qsv") << "braw" << decoders.contains("braw");
    }
    void rejectsUnsafeInputs() {
        for (const auto& path : {QStringLiteral("https://example.com/video.mp4"),
                QStringLiteral("\\\\server\\share\\video.mp4"), QStringLiteral("C:/file.mp4:stream"),
                QStringLiteral("C:/../file.mp4"), QStringLiteral("relative.mp4")}) {
            QVERIFY2(!aidfame::MpvPlayer::validateLocalFile(path).isEmpty(), qPrintable(path));
        }
    }

    void playPauseSeek_data() {
        QTest::addColumn<QString>("filename");
        QTest::addColumn<bool>("software");
        QTest::newRow("h264-hardware") << QStringLiteral("test-h264-1080p.mp4") << false;
        QTest::newRow("hevc-hardware") << QStringLiteral("test-hevc-4k.mp4") << false;
        QTest::newRow("h264-software") << QStringLiteral("test-h264-1080p.mp4") << true;
    }

    void playPauseSeek() {
        QFETCH(QString, filename);
        QFETCH(bool, software);
        const auto path = QStringLiteral(MEDIA_DIR) + QLatin1Char('/') + filename;
        QVERIFY2(QFileInfo::exists(path), "Generate the required fixtures with scripts/generate-fixtures.ps1");
        QWidget surface;
        surface.setAttribute(Qt::WA_NativeWindow);
        surface.resize(960, 540);
        surface.show();
        QVERIFY(QTest::qWaitForWindowExposed(&surface));
        aidfame::MpvPlayer player;
        QSignalSpy loaded(&player, &aidfame::MpvPlayer::fileLoaded);
        QSignalSpy errors(&player, &aidfame::MpvPlayer::errorOccurred);
        QVERIFY2(player.initialize(static_cast<quintptr>(surface.winId()), software), qPrintable(player.lastError()));
        QVERIFY(player.openLocalFile(path));
        QTRY_VERIFY_WITH_TIMEOUT(loaded.count() == 1, 15000);
        QTRY_VERIFY_WITH_TIMEOUT(player.numberProperty("time-pos") > 0.5, 15000);
        QVERIFY(player.numberProperty("duration") > 11);
        qInfo() << "Decoder:" << player.textProperty("hwdec-current")
                << "Codec:" << player.textProperty("video-codec")
                << "Dimensions:" << player.numberProperty("width") << player.numberProperty("height");
        if (software) QCOMPARE(player.textProperty("hwdec-current"), QStringLiteral("no"));
        QVERIFY(player.setPaused(true));
        QTRY_COMPARE(player.textProperty("pause"), QStringLiteral("yes"));
        QTemporaryDir captures;
        QVERIFY(captures.isValid());
        QSignalSpy saved(&player, &aidfame::MpvPlayer::screenshotSaved);
        for (const auto& format : {QStringLiteral("png"), QStringLiteral("jpg")}) {
            const int count = saved.count();
            QVERIFY(player.screenshot(captures.path(), format));
            QTRY_COMPARE_WITH_TIMEOUT(saved.count(), count + 1, 10000);
            QImageReader reader(saved.last().at(0).toString());
            QVERIFY(reader.canRead());
            QCOMPARE(reader.size().height(), filename.contains(QStringLiteral("4k")) ? 2160 : 1080);
            QCOMPARE(reader.format(), format == QStringLiteral("jpg") ? QByteArray("jpeg") : QByteArray("png"));
        }
        QVERIFY(saved.at(0).at(0) != saved.at(1).at(0));
        QVERIFY(!player.screenshot(captures.path(), QStringLiteral("exe")));
        QVERIFY(!player.screenshot(QStringLiteral("//server/share"), QStringLiteral("png")));
        const double before = player.numberProperty("time-pos");
        QTest::qWait(250);
        QVERIFY(std::abs(player.numberProperty("time-pos") - before) < 0.15);
        QVERIFY(player.seek(5, true));
        QTRY_VERIFY_WITH_TIMEOUT(std::abs(player.numberProperty("time-pos") - 5) < 0.2, 5000);
        surface.resize(800, 450);
        QVERIFY(player.setPaused(false));
        QTRY_VERIFY(player.numberProperty("time-pos") > 5.25);
        QVERIFY(player.setSpeed(1.5));
        QTRY_COMPARE(player.numberProperty("speed"), 1.5);
        QVERIFY(!player.setSpeed(4.5));
        QVERIFY(player.setSpeed(1));
        QVERIFY(player.setMuted(true));
        QTRY_COMPARE(player.textProperty("mute"), QStringLiteral("yes"));
        QVERIFY(player.setMuted(false));
        QVERIFY(player.setAspect(QStringLiteral("4:3")));
        QTRY_VERIFY(std::abs(player.numberProperty("video-aspect-override") - 4.0 / 3.0) < 0.01);
        QVERIFY(player.setAspect(QStringLiteral("original")));
        QVERIFY(!player.setResolutionCap(4320));
        QVERIFY(player.setResolutionCap(720));
        QTest::qWait(500);
        qInfo() << "Scale filter:" << player.textProperty("vf") << "decoder" << player.textProperty("hwdec-current")
                << "errors" << errors;
        QTRY_COMPARE_WITH_TIMEOUT(player.numberProperty("video-out-params/h"), 720.0, 10000);
        QVERIFY(player.setResolutionCap(0));
        QTest::qWait(300);
        QTRY_COMPARE_WITH_TIMEOUT(player.numberProperty("video-out-params/h"), filename.contains(QStringLiteral("4k")) ? 2160.0 : 1080.0, 10000);
        QCOMPARE(errors.count(), 0);
    }
};

int main(int argc, char** argv) {
    aidfame::WindowsRuntime runtime;
    if (!runtime.valid()) return 1;
    QApplication app(argc,argv);
    PlaybackTests tests;
    return QTest::qExec(&tests,argc,argv);
}
#include "PlaybackTests.moc"
