#include "Settings/LocalStore.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QDateTime>
#include <QUuid>
#include <cmath>
#include <algorithm>

namespace aidfame {
LocalStore::LocalStore(QString root) : root_(root.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) : std::move(root)) {
    settings = defaults();
}
QJsonObject LocalStore::defaults() {
    return {{"seekStepSeconds",5},{"screenshotFormat","png"},
        {"screenshotDirectory",QDir(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)).filePath("AidfamePlayer/Screenshot")},
        {"recordingDirectory",QDir(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)).filePath("AidfamePlayer/Record")},
        {"hardwareDecoder","auto"},{"showMediaInfo",true},{"resumeHistory",true},
        {"volume",80},{"speed",1.0},{"aspectMode","original"}};
}
QJsonObject LocalStore::validated(const QJsonObject& input) {
    auto result = defaults();
    const auto choice = [&](const char* key, const QStringList& values) {
        if (input[key].isString() && values.contains(input[key].toString())) result[key] = input[key];
    };
    choice("screenshotFormat",{"png","jpg"}); choice("hardwareDecoder",{"auto","no","d3d11va","dxva2","nvdec"});
    choice("aspectMode",{"original","16:9","4:3","stretch"});
    for (const char* key : {"showMediaInfo","resumeHistory"}) if (input[key].isBool()) result[key] = input[key];
    for (const char* key : {"screenshotDirectory","recordingDirectory"}) {
        const auto value = input[key].toString();
        if (!value.isEmpty() && value.size() < 32760 && !value.contains(QChar::Null)) result[key] = value;
    }
    const auto seek = input["seekStepSeconds"];
    if (seek.isDouble() && QList<double>{5,10,15,20,30}.contains(seek.toDouble())) result["seekStepSeconds"] = seek;
    const auto volume = input["volume"];
    if (volume.isDouble() && volume.toDouble() >= 0 && volume.toDouble() <= 100 && std::floor(volume.toDouble()) == volume.toDouble()) result["volume"] = volume;
    const auto speed = input["speed"];
    if (speed.isDouble() && QList<double>{0.5,0.75,1,1.25,1.5,2,3,4,5}.contains(speed.toDouble())) result["speed"] = speed;
    return result;
}
bool LocalStore::load() {
    error.clear(); settings = defaults(); history = {}; writable_ = true;
    QFile file(QDir(root_).filePath("state.json"));
    if (!file.exists()) return true;
    if (!file.open(QIODevice::ReadOnly)) { writable_ = false; error = QStringLiteral("无法读取本地设置。"); return false; }
    QJsonParseError parse;
    const auto doc = QJsonDocument::fromJson(file.size() <= 1024*1024 ? file.readAll() : QByteArray(), &parse);
    file.close();
    if (parse.error != QJsonParseError::NoError || !doc.isObject() || doc.object()["schemaVersion"].toInt() != 1) {
        const auto backup = file.fileName() + ".invalid-" + QUuid::createUuid().toString(QUuid::Id128);
        if (!file.copy(backup)) { writable_ = false; error = QStringLiteral("设置损坏，无法创建备份；请检查权限。"); return false; }
        error = QStringLiteral("设置文件损坏，已备份并使用默认设置。"); return false;
    }
    settings = validated(doc.object()["settings"].toObject());
    auto entries = doc.object()["history"].toArray();
    QList<QJsonObject> valid;
    for (const auto& entry : entries) {
        auto value = entry.toObject();
        const auto path = value["path"].toString();
        const auto time = QDateTime::fromString(value["lastPlayedUtc"].toString(),Qt::ISODateWithMs);
        const double position = value["positionSeconds"].toDouble(-1), duration = value["durationSeconds"].toDouble(-1);
        if (path.isEmpty() || path.size() > 32760 || !time.isValid() || !std::isfinite(position)
            || !std::isfinite(duration) || position < 0 || duration < 0) continue;
        valid.append(value);
    }
    std::sort(valid.begin(),valid.end(),[](const auto& a,const auto& b) { return a["lastPlayedUtc"].toString() > b["lastPlayedUtc"].toString(); });
    QStringList paths;
    for (const auto& entry : valid) {
        if (paths.contains(entry["path"].toString(),Qt::CaseInsensitive)) continue;
        paths.append(entry["path"].toString()); history.append(entry); if (history.size() >= 50) break;
    }
    return true;
}
bool LocalStore::save() {
    if (!writable_) { error = QStringLiteral("原设置无法读取或备份，本次会话不会覆盖它。"); return false; }
    if (!QDir().mkpath(root_)) { error = QStringLiteral("无法创建本地数据目录。"); return false; }
    QSaveFile file(QDir(root_).filePath("state.json"));
    const auto data = QJsonDocument(QJsonObject{{"schemaVersion",1},{"settings",validated(settings)},{"history",history}}).toJson();
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        error = QStringLiteral("无法保存本地设置，请检查磁盘空间和权限。"); return false;
    }
    error.clear(); return true;
}
void LocalStore::forget(const QString& path) {
    for (qsizetype index = history.size(); index > 0; --index)
        if (history[index-1].toObject()["path"].toString().compare(path,Qt::CaseInsensitive) == 0) history.removeAt(index-1);
}
void LocalStore::remember(const QString& path, double position, double duration) {
    if (path.isEmpty() || !std::isfinite(position) || !std::isfinite(duration) || position < 0 || duration < 0) return;
    forget(path);
    history.prepend(QJsonObject{{"path",path},{"positionSeconds",qMin(position,duration)},
        {"durationSeconds",duration},{"lastPlayedUtc",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}});
    while (history.size() > 50) history.removeLast();
}
}
