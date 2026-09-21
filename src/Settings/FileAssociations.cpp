#include "Settings/FileAssociations.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <ShlObj.h>

namespace aidfame {
QList<RegistryEntry> associationEntries(const QString& executable) {
    if (!QFileInfo(executable).isAbsolute() || executable.contains('"') || executable.contains(QChar::Null)) return {};
    const auto exe = QDir::toNativeSeparators(executable);
    const QString capabilities = QStringLiteral("Software\\Aidfame\\AidfamePlayer\\Capabilities");
    QList<RegistryEntry> entries{{capabilities,"ApplicationName","Aidfame Player"},
        {capabilities,"ApplicationDescription",QStringLiteral("本地离线视频播放器")},
        {capabilities,"ApplicationIcon",QStringLiteral("\"%1\",0").arg(exe)},
        {"Software\\RegisteredApplications","Aidfame Player",capabilities}};
    for (const auto& extension : QStringList{"mp4","mkv","mov","avi"}) {
        const auto progid = "AidfamePlayer." + extension;
        const auto key = "Software\\Classes\\" + progid;
        entries.append({key,{},"Aidfame Player " + extension.toUpper()});
        entries.append({key + "\\DefaultIcon",{},QStringLiteral("\"%1\",0").arg(exe)});
        entries.append({key + "\\shell\\open\\command",{},QStringLiteral("\"%1\" \"%2\"").arg(exe,QStringLiteral("%1"))});
        entries.append({capabilities+"\\FileAssociations","."+extension,progid});
        entries.append({"Software\\Classes\\."+extension+"\\OpenWithProgids",progid,{}});
    }
    return entries;
}
bool registerFileAssociations(QString* error) {
    const auto entries = associationEntries(QCoreApplication::applicationFilePath());
    for (const auto& entry : entries) {
        HKEY key = nullptr;
        auto result = RegCreateKeyExW(HKEY_CURRENT_USER,entry.key.toStdWString().c_str(),0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr);
        if (result == ERROR_SUCCESS) {
            const auto text = entry.value.toStdWString();
            result = RegSetValueExW(key,entry.name.isEmpty() ? nullptr : entry.name.toStdWString().c_str(),0,REG_SZ,
                reinterpret_cast<const BYTE*>(text.c_str()),static_cast<DWORD>((text.size()+1)*sizeof(wchar_t)));
            RegCloseKey(key);
        }
        if (result != ERROR_SUCCESS) { if (error) *error = QStringLiteral("无法注册文件类型（Windows 错误 %1）。").arg(result); return false; }
    }
    SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,nullptr,nullptr);
    return true;
}
}
