#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <mpv/client.h>

namespace aidfame {

/** Owns one playback session; all public methods are called on the Qt main thread. */
class MpvPlayer final : public QObject {
    Q_OBJECT
public:
    explicit MpvPlayer(QObject* parent = nullptr);
    ~MpvPlayer() override;
    bool initialize(quintptr windowId, bool softwareOnly = false);
    bool openLocalFile(const QString& path);
    bool setPaused(bool paused);
    bool seek(double seconds, bool absolute = false);
    bool setVolume(int volume);
    bool setSpeed(double speed);
    bool setMuted(bool muted);
    bool setAspect(const QString& mode);
    bool setResolutionCap(int height);
    bool setHardwareDecoder(const QString& decoder);
    bool setLooping(bool enabled);
    void showMediaInfo(bool enabled);
    bool screenshot(const QString& directory, const QString& format);
    QString textProperty(const char* name) const;
    double numberProperty(const char* name) const;
    QString lastError() const { return lastError_; }
    static QString validateLocalFile(const QString& path);

signals:
    void fileLoaded();
    void playbackEnded();
    void positionChanged(double seconds);
    void durationChanged(double seconds);
    void pausedChanged(bool paused);
    void decoderChanged(const QString& decoder);
    void errorOccurred(const QString& message);
    void screenshotSaved(const QString& path);
    void screenshotFailed(const QString& message);

private:
    static void wakeup(void* context);
    void drainEvents();
    bool command(const QStringList& arguments);
    bool property(const char* name, const QByteArray& value);
    bool fail(const QString& message);
    QByteArray effectiveDecoder() const;
    mpv_handle* handle_ = nullptr;
    QString lastError_;
    bool softwareOnly_ = false;
    QString screenshotPath_;
    QByteArray decoderPolicy_ = "auto";
    int resolutionCap_ = 0;
};

} // namespace aidfame
