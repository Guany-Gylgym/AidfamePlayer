#include "Settings/FileAssociations.h"
#include <QTest>
#include <QCoreApplication>
#include <QUuid>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

// Redirect this test process's HKCU only; never change the user's real defaults.
class RegistrySandbox final {
public:
    RegistrySandbox() : path_(L"Software\\AidfamePlayerTests\\" + QUuid::createUuid().toString(QUuid::WithoutBraces).toStdWString()) {
        if (RegCreateKeyExW(HKEY_CURRENT_USER,path_.c_str(),0,nullptr,0,KEY_ALL_ACCESS,nullptr,&key_,nullptr) == ERROR_SUCCESS)
            redirected_ = RegOverridePredefKey(HKEY_CURRENT_USER,key_) == ERROR_SUCCESS;
    }
    ~RegistrySandbox() {
        if (redirected_) RegOverridePredefKey(HKEY_CURRENT_USER,nullptr);
        if (key_) { RegCloseKey(key_); RegDeleteTreeW(HKEY_CURRENT_USER,path_.c_str()); }
    }
    bool valid() const { return redirected_; }
private:
    std::wstring path_;
    HKEY key_ = nullptr;
    bool redirected_ = false;
};
class AssociationTests : public QObject {
    Q_OBJECT
private slots:
    void writesIsolatedRegistry() {
        RegistrySandbox sandbox;
        QVERIFY(sandbox.valid());
        QString error;
        QVERIFY2(aidfame::registerFileAssociations(&error),qPrintable(error));
        for (const auto& entry : aidfame::associationEntries(QCoreApplication::applicationFilePath())) {
            wchar_t text[2048]{};
            DWORD size = sizeof(text), type = 0;
            QCOMPARE(RegGetValueW(HKEY_CURRENT_USER,entry.key.toStdWString().c_str(),entry.name.isEmpty() ? nullptr : entry.name.toStdWString().c_str(),
                RRF_RT_REG_SZ,&type,text,&size),ERROR_SUCCESS);
            QCOMPARE(QString::fromWCharArray(text),entry.value);
        }
    }
    void safeRegistrationPlan() {
        const auto entries = aidfame::associationEntries(QStringLiteral("C:/Program Files/Aidfame Player/AidfamePlayer.exe"));
        QCOMPARE(entries.size(),24);
        int commands = 0, capabilities = 0;
        for (const auto& entry : entries) {
            QVERIFY(!entry.key.contains("UserChoice"));
            QVERIFY(!entry.key.contains("Explorer"));
            if (entry.key.endsWith("\\command")) {
                QCOMPARE(entry.value,QStringLiteral("\"C:\\Program Files\\Aidfame Player\\AidfamePlayer.exe\" \"%1\"")); ++commands;
            }
            if (entry.key.endsWith("\\FileAssociations")) ++capabilities;
        }
        QCOMPARE(commands,4); QCOMPARE(capabilities,4);
        QVERIFY(aidfame::associationEntries("relative.exe").isEmpty());
        QVERIFY(aidfame::associationEntries("C:/bad\".exe").isEmpty());
    }
};
QTEST_GUILESS_MAIN(AssociationTests)
#include "AssociationTests.moc"
