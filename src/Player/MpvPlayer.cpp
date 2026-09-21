#include "Player/MpvPlayer.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QUuid>
#include <QDateTime>
#include <QMetaObject>
#include <QSet>
#include <cmath>
#include <array>
#include <algorithm>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace aidfame {

MpvPlayer::MpvPlayer(QObject* parent) : QObject(parent) {}

MpvPlayer::~MpvPlayer() {
    if (handle_) {
        mpv_set_wakeup_callback(handle_, nullptr, nullptr);
        mpv_terminate_destroy(handle_);
    }
}

bool MpvPlayer::fail(const QString& message) {
    lastError_ = message;
    emit errorOccurred(message);
    return false;
}

bool MpvPlayer::initialize(quintptr windowId, bool softwareOnly) {
    if (handle_) return true;
    softwareOnly_ = softwareOnly;
    decoderPolicy_ = softwareOnly ? "no" : "auto";
    handle_ = mpv_create();
    if (!handle_) return fail(QStringLiteral("无法创建 mpv 播放器。"));
    const std::pair<const char*, const char*> options[] = {
        {"config", "no"}, {"load-scripts", "no"}, {"ytdl", "no"}, {"osc", "no"},
        {"input-default-bindings", "no"}, {"input-vo-keyboard", "no"},
        {"media-controls", "no"}, {"input-media-keys", "no"},
        {"autoload-files", "no"}, {"access-references", "no"},
        {"load-unsafe-playlists", "no"}, {"demuxer", "lavf"},
        {"demuxer-lavf-o", "protocol_whitelist=file"},
        {"terminal", "no"}, {"idle", "yes"}, {"keep-open", "yes"},
        {"vo", "gpu"}, {"gpu-api", "d3d11"}, {"gpu-context", "d3d11"},
        {"hwdec", softwareOnly ? "no" : "auto"}, {"volume", "80"}
    };
    for (const auto& [name, value] : options) {
        const int result = mpv_set_option_string(handle_, name, value);
        if (result < 0) {
            mpv_terminate_destroy(handle_);
            handle_ = nullptr;
            return fail(QStringLiteral("mpv 配置失败：%1 (%2)")
                .arg(QString::fromLatin1(name), QString::fromUtf8(mpv_error_string(result))));
        }
    }
    const auto wid = QByteArray::number(static_cast<qulonglong>(windowId));
    int result = mpv_set_option_string(handle_, "wid", wid.constData());
    if (result >= 0) result = mpv_initialize(handle_);
    if (result < 0) {
        mpv_terminate_destroy(handle_);
        handle_ = nullptr;
        return fail(QStringLiteral("mpv 初始化失败：%1").arg(QString::fromUtf8(mpv_error_string(result))));
    }
    mpv_observe_property(handle_, 1, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(handle_, 2, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(handle_, 3, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(handle_, 4, "hwdec-current", MPV_FORMAT_STRING);
    mpv_set_wakeup_callback(handle_, &MpvPlayer::wakeup, this);
    return true;
}

QString MpvPlayer::validateLocalFile(const QString& path) {
    const auto normalized = QDir::fromNativeSeparators(path);
    if (normalized.startsWith(QStringLiteral("//")) || normalized.contains(QStringLiteral("://"))
        || normalized.size() < 3 || normalized.at(1) != QLatin1Char(':')
        || normalized.at(2) != QLatin1Char('/') || normalized.mid(2).contains(QLatin1Char(':'))) {
        return QStringLiteral("仅支持本机磁盘上的视频文件，不支持网址、网络共享或设备路径。");
    }
    // Reject remote drives before any potentially network-backed metadata query.
    const auto driveRoot = normalized.left(3).toStdWString();
    const auto type = GetDriveTypeW(driveRoot.c_str());
    if (type != DRIVE_FIXED && type != DRIVE_REMOVABLE && type != DRIVE_CDROM) {
        return QStringLiteral("不支持网络驱动器或不可用的磁盘。");
    }
    const auto parts = normalized.mid(3).split(QLatin1Char('/'), Qt::SkipEmptyParts);
    QString candidate = normalized.left(3);
    for (const auto& part : parts) {
        if (part == QStringLiteral("..") || part == QStringLiteral("."))
            return QStringLiteral("请使用规范的本地文件路径。");
        candidate = QDir(candidate).filePath(part);
        const DWORD attributes = GetFileAttributesW(candidate.toStdWString().c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
            return QStringLiteral("为保证离线播放，不打开符号链接、联接或云占位文件。");
    }
    const QFileInfo file(normalized);
    if (!file.isFile() || !file.isReadable()) return QStringLiteral("视频文件不存在或无法读取。");
    static const QSet<QString> extensions = {"mp4", "mkv", "mov", "avi", "flv", "wmv", "ts", "mts", "m2ts", "webm", "mxf"};
    if (!extensions.contains(file.suffix().toLower())) return QStringLiteral("此文件类型尚未支持。BRAW 插件尚未接入。");
    return {};
}

bool MpvPlayer::openLocalFile(const QString& path) {
    const auto validation = validateLocalFile(path);
    if (!validation.isEmpty()) return fail(validation);
    if (!handle_) return fail(QStringLiteral("播放器尚未初始化。"));
    // keep-open can pause the previous file after a separate unpause request.
    // Apply playback intent to the new file as part of the load command instead.
    return command({QStringLiteral("loadfile"), QFileInfo(path).canonicalFilePath(),
        QStringLiteral("replace"), QStringLiteral("-1"), QStringLiteral("pause=no")});
}

bool MpvPlayer::command(const QStringList& arguments) {
    if (!handle_) return false;
    std::vector<QByteArray> storage;
    storage.reserve(static_cast<size_t>(arguments.size()));
    for (const auto& arg : arguments) storage.push_back(arg.toUtf8());
    std::vector<const char*> pointers;
    for (const auto& arg : storage) pointers.push_back(arg.constData());
    pointers.push_back(nullptr);
    const int result = mpv_command_async(handle_, 0, pointers.data());
    return result >= 0 || fail(QStringLiteral("播放命令失败：%1").arg(QString::fromUtf8(mpv_error_string(result))));
}

bool MpvPlayer::property(const char* name, const QByteArray& value) {
    if (!handle_) return false;
    // Filter changes can recreate native video resources; never block Qt's message pump.
    const char* text = value.constData();
    const int result = mpv_set_property_async(handle_, 0, name, MPV_FORMAT_STRING, &text);
    return result >= 0 || fail(QStringLiteral("播放设置失败：%1").arg(QString::fromUtf8(mpv_error_string(result))));
}

bool MpvPlayer::setPaused(bool paused) { return property("pause", paused ? "yes" : "no"); }
bool MpvPlayer::setLooping(bool enabled) { return property("loop-file", enabled ? "inf" : "no"); }

bool MpvPlayer::seek(double seconds, bool absolute) {
    if (!std::isfinite(seconds)) return false;
    return command({QStringLiteral("seek"), QString::number(seconds, 'f', 3),
                    absolute ? QStringLiteral("absolute+exact") : QStringLiteral("relative+exact")});
}

bool MpvPlayer::setVolume(int volume) { return property("volume", QByteArray::number(qBound(0, volume, 100))); }

bool MpvPlayer::setSpeed(double speed) {
    constexpr std::array<double, 9> allowed{0.5, 0.75, 1, 1.25, 1.5, 2, 3, 4, 5};
    if (std::find(allowed.begin(), allowed.end(), speed) == allowed.end()) return false;
    return property("speed", QByteArray::number(speed));
}

bool MpvPlayer::setMuted(bool muted) { return property("mute", muted ? "yes" : "no"); }

bool MpvPlayer::screenshot(const QString& directory, const QString& format) {
    const auto reject = [this](const QString& error) { emit screenshotFailed(error); return false; };
    if (!handle_ || !screenshotPath_.isEmpty()) return reject(QStringLiteral("播放器未就绪或截图仍在保存。"));
    if (format != QStringLiteral("png") && format != QStringLiteral("jpg")) return reject(QStringLiteral("截图格式无效。"));
    const auto path = QDir::fromNativeSeparators(directory);
    if (path.size() < 3 || path.at(1) != ':' || path.at(2) != '/' || path.mid(2).contains(':')
        || path.contains(QStringLiteral("/../")) || path.endsWith(QStringLiteral("/..")))
        return reject(QStringLiteral("请选择本机截图目录。"));
    const auto drive = GetDriveTypeW(path.left(3).toStdWString().c_str());
    if (drive != DRIVE_FIXED && drive != DRIVE_REMOVABLE) return reject(QStringLiteral("请选择本机可写磁盘。"));
    QString partPath = path.left(3);
    for (const auto& part : path.mid(3).split('/', Qt::SkipEmptyParts)) {
        partPath = QDir(partPath).filePath(part);
        const auto attributes = GetFileAttributesW(partPath.toStdWString().c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
            return reject(QStringLiteral("截图目录不能包含链接或云占位目录。"));
    }
    if (!QDir().mkpath(path)) return reject(QStringLiteral("无法创建截图目录。"));
    screenshotPath_ = QDir(path).filePath(QStringLiteral("Aidfame-%1-%2.%3")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")),
             QUuid::createUuid().toString(QUuid::Id128), format));
    QFile reservation(screenshotPath_);
    if (!reservation.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        screenshotPath_.clear();
        return reject(QStringLiteral("截图目录无法写入。"));
    }
    reservation.close();
    const auto encodedPath = screenshotPath_.toUtf8();
    const char* args[] = {"screenshot-to-file", encodedPath.constData(), "video", nullptr};
    const auto result = mpv_command_async(handle_, 100, args);
    if (result < 0) {
        QFile::remove(screenshotPath_);
        screenshotPath_.clear();
        return reject(QStringLiteral("无法请求截图。"));
    }
    return true;
}

bool MpvPlayer::setAspect(const QString& mode) {
    if (mode != QStringLiteral("original") && mode != QStringLiteral("16:9")
        && mode != QStringLiteral("4:3") && mode != QStringLiteral("stretch")) return false;
    if (!property("keepaspect", mode == QStringLiteral("stretch") ? "no" : "yes")) return false;
    return property("video-aspect-override", mode == QStringLiteral("16:9") || mode == QStringLiteral("4:3") ? mode.toUtf8() : QByteArray("-1"));
}

bool MpvPlayer::setResolutionCap(int height) {
    if (height == 0) {
        resolutionCap_ = 0;
        return property("vf", "") && property("hwdec", effectiveDecoder());
    }
    if (height != 1080 && height != 720 && height != 480) return false;
    if (numberProperty("video-params/h") <= height) return false;
    resolutionCap_ = height;
    // CPU scale filters require copy-back frames rather than opaque D3D11 surfaces.
    return property("hwdec", effectiveDecoder())
        && property("vf", QByteArray("scale=w=-2:h=") + QByteArray::number(height));
}

bool MpvPlayer::setHardwareDecoder(const QString& decoder) {
    if (!QStringList{"auto","no","d3d11va","dxva2","nvdec"}.contains(decoder)) return false;
    decoderPolicy_ = decoder.toLatin1();
    return property("hwdec", effectiveDecoder());
}

QByteArray MpvPlayer::effectiveDecoder() const {
    if (decoderPolicy_ == "no") return "no";
    if (decoderPolicy_ == "nvdec") return "nvdec-copy";
    if (resolutionCap_ == 0) return decoderPolicy_;
    if (decoderPolicy_ == "auto") return "auto-copy";
    return decoderPolicy_ + "-copy";
}

void MpvPlayer::showMediaInfo(bool enabled) {
    if (!enabled) { command({QStringLiteral("show-text"), QString(), QStringLiteral("0")}); return; }
    property("osd-align-x", "right");
    property("osd-align-y", "top");
    property("osd-font-size", "18");
    const double fps = numberProperty("container-fps");
    const double bitrate = numberProperty("video-bitrate");
    const auto decoder = textProperty("hwdec-current");
    const auto info = QStringLiteral("%1 × %2  |  %3 FPS\n%4  |  %5\n音频：%6\n解码：%7")
        .arg(numberProperty("video-params/w"), 0, 'f', 0).arg(numberProperty("video-params/h"), 0, 'f', 0)
        .arg(fps > 0 ? QString::number(fps, 'f', 2) : QStringLiteral("—"))
        .arg(textProperty("video-format"))
        .arg(bitrate > 0 ? QString::number(bitrate / 1000000.0, 'f', 1) + QStringLiteral(" Mbps") : QStringLiteral("码率未知"))
        .arg(textProperty("audio-codec-name"))
        .arg(decoder.isEmpty() || decoder == QStringLiteral("no") ? QStringLiteral("CPU") : decoder);
    command({QStringLiteral("show-text"), info, QStringLiteral("1500")});
}

QString MpvPlayer::textProperty(const char* name) const {
    if (!handle_) return {};
    char* value = mpv_get_property_string(handle_, name);
    if (!value) return {};
    const auto result = QString::fromUtf8(value);
    mpv_free(value);
    return result;
}

double MpvPlayer::numberProperty(const char* name) const {
    double value = 0;
    if (handle_) mpv_get_property(handle_, name, MPV_FORMAT_DOUBLE, &value);
    return value;
}

void MpvPlayer::wakeup(void* context) {
    auto* self = static_cast<MpvPlayer*>(context);
    QMetaObject::invokeMethod(self, [self] { self->drainEvents(); }, Qt::QueuedConnection);
}

void MpvPlayer::drainEvents() {
    if (!handle_) return;
    while (true) {
        const auto* event = mpv_wait_event(handle_, 0);
        if (event->event_id == MPV_EVENT_NONE) break;
        if (event->event_id == MPV_EVENT_COMMAND_REPLY && event->reply_userdata == 100) {
            const auto output = screenshotPath_;
            screenshotPath_.clear();
            if (event->error < 0 || QFileInfo(output).size() == 0) {
                QFile::remove(output);
                emit screenshotFailed(QStringLiteral("截图保存失败，请检查磁盘空间和权限。"));
            } else emit screenshotSaved(output);
            continue;
        }
        if (event->event_id == MPV_EVENT_FILE_LOADED) emit fileLoaded();
        else if (event->event_id == MPV_EVENT_END_FILE) {
            const auto* end = static_cast<mpv_event_end_file*>(event->data);
            if (end->reason == MPV_END_FILE_REASON_ERROR)
                fail(QStringLiteral("无法播放该文件：%1").arg(QString::fromUtf8(mpv_error_string(end->error))));
            emit playbackEnded();
        } else if ((event->event_id == MPV_EVENT_COMMAND_REPLY || event->event_id == MPV_EVENT_SET_PROPERTY_REPLY) && event->error < 0) {
            fail(QStringLiteral("播放操作失败：%1").arg(QString::fromUtf8(mpv_error_string(event->error))));
        } else if (event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
            const auto* data = static_cast<mpv_event_property*>(event->data);
            if (!data->data) continue;
            if (event->reply_userdata == 1 && data->format == MPV_FORMAT_DOUBLE)
                emit positionChanged(*static_cast<double*>(data->data));
            else if (event->reply_userdata == 2 && data->format == MPV_FORMAT_DOUBLE)
                emit durationChanged(*static_cast<double*>(data->data));
            else if (event->reply_userdata == 3 && data->format == MPV_FORMAT_FLAG)
                emit pausedChanged(*static_cast<int*>(data->data) != 0);
            else if (event->reply_userdata == 4 && data->format == MPV_FORMAT_STRING)
                emit decoderChanged(QString::fromUtf8(*static_cast<char**>(data->data)));
        }
    }
}

} // namespace aidfame
