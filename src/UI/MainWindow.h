#pragma once

#include <QMainWindow>
#include "Settings/LocalStore.h"

class QStackedWidget;

namespace aidfame {
class MpvPlayer;
class Recorder;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr, bool staticPreview = false, const QString& dataRoot = {});
    ~MainWindow() override;
    void toggleFullscreen();
    void toggleMini();
    void openFile(const QString& path);
    bool setSeekStep(int seconds);
    int seekStep() const { return seekStep_; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void showStageNotice();
    void chooseFile();
    void refreshTime();
    void setPlaybackEnabled(bool enabled);
    void toggleRecording();
    void refreshRecording();
    void showSettings();
    void applySettings();
    void saveState();
    void refreshHistory();
    QStackedWidget* viewportStack_ = nullptr;
    QWidget* videoSurface_ = nullptr;
    MpvPlayer* player_ = nullptr;
    double duration_ = 0;
    double position_ = 0;
    bool paused_ = false;
    bool staticPreview_ = false;
    bool muted_ = false;
    bool showInfo_ = true;
    bool wasMaximized_ = false;
    int seekStep_ = 5;
    QString screenshotDirectory_;
    QString screenshotFormat_ = QStringLiteral("png");
    Recorder* recorder_ = nullptr;
    QString currentFile_;
    QString recordingDirectory_;
    QString aspect_ = QStringLiteral("original");
    LocalStore store_;
    double resumePosition_ = 0;
    bool fileReady_ = false;
    bool rememberActive_ = true;
    bool mini_ = false;
    QByteArray normalGeometry_;
};

} // namespace aidfame
