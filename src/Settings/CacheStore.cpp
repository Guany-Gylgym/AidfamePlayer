#include "Settings/CacheStore.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace aidfame {
namespace {
bool linked(const QString& path) {
    const auto attributes = GetFileAttributesW(path.toStdWString().c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT);
}
}
CacheStore::CacheStore(QString root) : root_(root.isEmpty()
    ? QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).filePath("cache-v1") : std::move(root)) {}
bool CacheStore::safeRoot() const {
    QString current = QDir::fromNativeSeparators(root_);
    if (current.size() < 4 || current[1] != ':' || current[2] != '/' || current.mid(2).contains(':')
        || current.split('/').contains("..")) return false;
    const auto drive = GetDriveTypeW(current.left(3).toStdWString().c_str());
    if (drive != DRIVE_FIXED && drive != DRIVE_REMOVABLE) return false;
    while (current.size() > 3) {
        if (linked(current)) return false;
        current = QFileInfo(current).absolutePath();
    }
    return true;
}
bool CacheStore::initialize() {
    if (!safeRoot()) { error = QStringLiteral("缓存目录包含链接或不安全路径。"); return false; }
    QDir directory(root_);
    const auto marker = directory.filePath(".aidfame-cache-owner");
    if (linked(marker)) return false;
    if (!QFileInfo::exists(marker)) {
        if (directory.exists() && !directory.entryList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden).isEmpty()) {
            error = QStringLiteral("目录不属于 Aidfame 缓存，拒绝清理。"); return false;
        }
        if (!QDir().mkpath(root_)) return false;
        QFile file(marker);
        if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) return false;
        const QByteArray contents("AidfamePlayer.Cache.v1\n");
        return file.write(contents) == contents.size();
    }
    QFile file(marker);
    return file.open(QIODevice::ReadOnly) && file.readAll() == "AidfamePlayer.Cache.v1\n";
}
qint64 CacheStore::size() const {
    if (!safeRoot()) return 0;
    qint64 total = 0;
    for (const auto& file : QDir(root_).entryInfoList({"aidfame-*.afcache"},QDir::Files | QDir::NoSymLinks))
        if (!linked(file.absoluteFilePath())) total += file.size();
    return total;
}
bool CacheStore::clear() {
    if (!initialize()) return false;
    for (const auto& file : QDir(root_).entryInfoList({"aidfame-*.afcache"},QDir::Files | QDir::NoSymLinks)) {
        if (!safeRoot() || linked(file.absoluteFilePath()) || !QFile::remove(file.absoluteFilePath())) {
            error = QStringLiteral("部分缓存不可删除；已保留该文件。"); return false;
        }
    }
    return true;
}
}
