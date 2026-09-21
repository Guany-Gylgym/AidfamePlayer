#include "Player/MpvPlayer.h"
#include "App/WindowsRuntime.h"
#include <QApplication>
#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSaveFile>
#include <QDateTime>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>

int main(int argc, char** argv) {
    aidfame::WindowsRuntime runtime;
    if (!runtime.valid()) return 1;
    QApplication app(argc,argv);
    const auto args = app.arguments();
    if (args.size() != 4) return 2;
    bool valid = false; const int seconds = args[2].toInt(&valid);
    if (!valid || seconds < 10 || seconds > 86400) return 2;
    QWidget surface; surface.setWindowTitle(QStringLiteral("Aidfame Player — stability test"));
    surface.resize(960,540); surface.show();
    aidfame::MpvPlayer player;
    QElapsedTimer elapsed, lastProgress;
    QJsonArray samples;
    QString failure;
    double previousPosition = -1;
    int loops = 0;
    const auto report = [&](bool finished) {
        QSaveFile file(args[3]);
        if (!file.open(QIODevice::WriteOnly)) return false;
        const auto data = QJsonDocument(QJsonObject{{"targetSeconds",seconds},
            {"elapsedSeconds",elapsed.isValid() ? elapsed.elapsed()/1000.0 : 0},
            {"finished",finished},{"passed",finished && failure.isEmpty() && elapsed.isValid() && elapsed.elapsed() >= seconds*1000LL},
            {"failure",failure},{"loops",loops},{"decoder",player.textProperty("hwdec-current")},
            {"updatedUtc",QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}, {"samples",samples}}).toJson();
        return file.write(data) == data.size() && file.commit();
    };
    QObject::connect(&player,&aidfame::MpvPlayer::errorOccurred,&app,[&](const QString& error) {
        failure = error; report(true); app.exit(1);
    });
    QObject::connect(&player,&aidfame::MpvPlayer::fileLoaded,&app,[&] {
        player.setMuted(true); player.setLooping(true);
        if (!elapsed.isValid()) { elapsed.start(); lastProgress.start(); }
    });
    QTimer tick;
    tick.setInterval(1000);
    QObject::connect(&tick,&QTimer::timeout,&app,[&] {
        if (!elapsed.isValid()) return;
        const double position = player.numberProperty("time-pos");
        if (position != previousPosition) { if (position < previousPosition) ++loops; lastProgress.restart(); }
        previousPosition = position;
        if (lastProgress.elapsed() > 15000) { failure = "Playback stalled for more than 15 seconds"; report(true); app.exit(1); return; }
        if (samples.isEmpty() || elapsed.elapsed()/1000 >= samples.last().toObject()["second"].toInt()+30) {
            PROCESS_MEMORY_COUNTERS_EX memory{}; memory.cb = sizeof(memory);
            GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),sizeof(memory));
            FILETIME created{},exited{},kernel{},user{}; GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user);
            ULARGE_INTEGER k{},u{}; k.LowPart=kernel.dwLowDateTime; k.HighPart=kernel.dwHighDateTime;
            u.LowPart=user.dwLowDateTime; u.HighPart=user.dwHighDateTime;
            samples.append(QJsonObject{{"second",elapsed.elapsed()/1000},{"position",position},
                {"privateBytes",double(memory.PrivateUsage)},{"workingSetBytes",double(memory.WorkingSetSize)},
                {"cpuSeconds",double(k.QuadPart+u.QuadPart)/10000000.0},
                {"droppedFrames",player.numberProperty("frame-drop-count")},
                {"avSync",player.numberProperty("avsync")}});
            if (!report(false)) { failure = "Cannot save stability report"; app.exit(1); }
        }
        if (elapsed.elapsed() >= seconds*1000LL) { report(true); app.exit(0); }
    });
    if (!player.initialize(surface.winId()) || !player.openLocalFile(args[1])) return 1;
    QTimer::singleShot(20000,&app,[&] { if (!elapsed.isValid()) { failure="File did not load"; report(true); app.exit(1); } });
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
    tick.start(); const auto result = app.exec();
    SetThreadExecutionState(ES_CONTINUOUS);
    if (!elapsed.isValid() || elapsed.elapsed() < seconds*1000LL) {
        if (failure.isEmpty()) failure = "Test window or event loop exited before the required duration";
        report(true);
        return 1;
    }
    return failure.isEmpty() ? result : 1;
}
