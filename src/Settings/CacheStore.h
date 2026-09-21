#pragma once
#include <QString>
namespace aidfame {
class CacheStore {
public:
    explicit CacheStore(QString root = {});
    bool initialize();
    qint64 size() const;
    bool clear();
    QString error;
private:
    bool safeRoot() const;
    QString root_;
};
}
