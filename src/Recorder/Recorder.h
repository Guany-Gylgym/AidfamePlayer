#pragma once
#include <QObject>
#include <QProcess>

namespace aidfame {
struct RecordingOptions {
    double speed = 1;
    int height = 0;
    QString aspect = QStringLiteral("original");
    double volume = 1;
    double displayAspect = 0;
};

class Recorder final : public QObject {
    Q_OBJECT
public:
    explicit Recorder(QObject* parent = nullptr);
    ~Recorder() override;
    bool start(const QString& source, double position, const QString& directory, const RecordingOptions& options);
    bool stop(double position);
    void cancel();
    bool capturing() const { return capturing_; }
    bool busy() const { return capturing_ || process_.state() != QProcess::NotRunning; }
    void setEncoder(const QString& path) { encoder_ = path; }
signals:
    void stateChanged();
    void completed(const QString& file);
    void failed(const QString& message);
private:
    QProcess process_;
    QString encoder_, source_, output_, temporary_;
    RecordingOptions options_;
    double start_ = 0;
    bool capturing_ = false;
    bool cancelled_ = false;
};
}
