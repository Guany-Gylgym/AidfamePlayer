#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include <QString>

namespace aidfame {
class LocalStore {
public:
    explicit LocalStore(QString root = {});
    bool load();
    bool save();
    QJsonObject settings;
    QJsonArray history;
    QString error;
    void remember(const QString& path, double position, double duration);
    void forget(const QString& path);
    static QJsonObject defaults();
    static QJsonObject validated(const QJsonObject& input);
private:
    QString root_;
    bool writable_ = true;
};
}
