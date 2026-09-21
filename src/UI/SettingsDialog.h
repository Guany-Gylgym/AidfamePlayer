#pragma once
#include <QDialog>
namespace aidfame {
class SettingsDialog final : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
};
}
