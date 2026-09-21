#include "Recorder/Recorder.h"
#include "Player/MpvPlayer.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <QStorageInfo>
#include <cmath>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace aidfame {
Recorder::Recorder(QObject* parent) : QObject(parent) {
    encoder_ = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("ffmpeg.exe"));
    process_.setProcessChannelMode(QProcess::MergedChannels);
    connect(&process_, &QProcess::readyReadStandardOutput, this, [this] { process_.readAllStandardOutput(); });
    connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            QFile::remove(temporary_);
            emit failed(QStringLiteral("无法启动本地 FFmpeg 编码器。"));
            emit stateChanged();
        }
    });
    connect(&process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this](int code, QProcess::ExitStatus status) {
            if (!cancelled_ && code == 0 && status == QProcess::NormalExit
                && QFileInfo(temporary_).size() > 1024 && QFile::rename(temporary_, output_)) {
                emit completed(output_);
            } else {
                QFile::remove(temporary_);
                if (!cancelled_) emit failed(QStringLiteral("录制编码失败，请检查磁盘空间及源文件。"));
            }
            emit stateChanged();
        });
}
Recorder::~Recorder() { cancel(); }

bool Recorder::start(const QString& source, double position, const QString& directory, const RecordingOptions& options) {
    if (busy() || !std::isfinite(position) || position < 0) return false;
    if (!MpvPlayer::validateLocalFile(source).isEmpty() || !QFileInfo::exists(encoder_)) {
        emit failed(QStringLiteral("源文件或本地编码器不可用。")); return false;
    }
    const QList<double> speeds{0.5, 0.75, 1, 1.25, 1.5, 2, 3, 4, 5};
    if (!speeds.contains(options.speed) || !QList<int>{0, 480, 720, 1080}.contains(options.height)
        || !std::isfinite(options.volume) || options.volume < 0 || options.volume > 1
        || !QStringList{"original", "16:9", "4:3", "stretch"}.contains(options.aspect)) return false;
    // Destination must be a local non-linked directory; never follow user-selected links.
    const auto normalized = QDir::fromNativeSeparators(directory);
    if (normalized.size() < 3 || normalized[1] != ':' || normalized[2] != '/'
        || normalized.mid(2).contains(':') || normalized.split('/').contains("..")) {
        emit failed(QStringLiteral("请选择规范的本机录制目录，不支持网络路径。")); return false;
    }
    const auto drive = GetDriveTypeW(normalized.left(3).toStdWString().c_str());
    if (drive != DRIVE_FIXED && drive != DRIVE_REMOVABLE) {
        emit failed(QStringLiteral("录制目录必须位于可用的本机磁盘。")); return false;
    }
    if (options.aspect == "stretch" && (!std::isfinite(options.displayAspect) || options.displayAspect <= 0)) return false;
    QString parentPath = normalized;
    while (parentPath.size() > 3) {
        QFileInfo info(parentPath);
        const auto attributes = GetFileAttributesW(parentPath.toStdWString().c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
            emit failed(QStringLiteral("录制目录不能包含链接或云占位目录。")); return false;
        }
        parentPath = info.absolutePath();
    }
    if (!QDir().mkpath(normalized) || !QStorageInfo(normalized).isReady()) {
        emit failed(QStringLiteral("录制目录不可写。")); return false;
    }
    source_ = source; options_ = options; start_ = position;
    output_ = QDir(normalized).filePath(QStringLiteral("Aidfame-%1.mp4").arg(QUuid::createUuid().toString(QUuid::Id128)));
    temporary_ = output_ + QStringLiteral(".part.mp4");
    QFile reservation(temporary_);
    if (!reservation.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        emit failed(QStringLiteral("无法创建录制文件。")); return false;
    }
    reservation.close(); capturing_ = true; cancelled_ = false;
    emit stateChanged(); return true;
}

bool Recorder::stop(double position) {
    if (!capturing_) return false;
    capturing_ = false;
    if (!std::isfinite(position) || position - start_ < 0.08) {
        QFile::remove(temporary_);
        emit failed(QStringLiteral("录制区间太短，没有生成视频。")); emit stateChanged(); return false;
    }
    QStringList videoFilters;
    if (options_.height > 0) videoFilters << QStringLiteral("scale=-2:%1").arg(options_.height);
    else videoFilters << QStringLiteral("scale=trunc(iw/2)*2:trunc(ih/2)*2");
    if (options_.aspect == "16:9" || options_.aspect == "4:3")
        videoFilters << QStringLiteral("setdar=%1").arg(options_.aspect);
    else if (options_.aspect == "stretch") videoFilters << QStringLiteral("setdar=%1").arg(options_.displayAspect);
    videoFilters << QStringLiteral("setpts=(PTS-STARTPTS)/%1").arg(options_.speed);
    QStringList audioFilters{QStringLiteral("asetpts=PTS-STARTPTS")};
    double remainingSpeed = options_.speed;
    while (remainingSpeed > 2) { audioFilters << QStringLiteral("atempo=2"); remainingSpeed /= 2; }
    audioFilters << QStringLiteral("atempo=%1").arg(remainingSpeed) << QStringLiteral("volume=%1").arg(options_.volume);
    const auto audioFilter = audioFilters.join(',');
    QStringList args{"-hide_banner", "-loglevel", "error", "-nostdin", "-y", "-protocol_whitelist", "file",
        "-ss", QString::number(start_, 'f', 6), "-t", QString::number(position - start_, 'f', 6),
        "-i", source_, "-map", "0:v:0", "-map", "0:a:0?", "-sn", "-dn",
        "-filter_threads", "2", "-vf", videoFilters.join(','), "-af", audioFilter,
        "-c:v", "libx264", "-preset", "veryfast", "-crf", "18", "-pix_fmt", "yuv420p",
        "-threads", "4", "-c:a", "aac", "-b:a", "192k", "-movflags", "+faststart", temporary_};
    process_.start(encoder_, args);
    emit stateChanged(); return true;
}

void Recorder::cancel() {
    cancelled_ = true; capturing_ = false;
    if (process_.state() != QProcess::NotRunning) { process_.kill(); process_.waitForFinished(5000); }
    if (!temporary_.isEmpty()) QFile::remove(temporary_);
    emit stateChanged();
}
}
