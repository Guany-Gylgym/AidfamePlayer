#include "Settings/CacheStore.h"
#include <QTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QScopeGuard>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
class CacheTests : public QObject {
    Q_OBJECT
private slots:
    void refusesJunctionRoot() {
        QTemporaryDir parent;
        const auto target = QDir(parent.path()).filePath("target");
        aidfame::CacheStore real(target); QVERIFY(real.initialize());
        const auto sentinel = QDir(target).filePath("aidfame-keep.afcache");
        QFile file(sentinel); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("preserve"); file.close();
        const auto junction = QDir(parent.path()).filePath("junction");
        const auto cleanup = qScopeGuard([&] { RemoveDirectoryW(junction.toStdWString().c_str()); });
        QCOMPARE(QProcess::execute(QStringLiteral("C:/Windows/System32/cmd.exe"),
            {"/d","/c","mklink","/J",QDir::toNativeSeparators(junction),QDir::toNativeSeparators(target)}),0);
        QVERIFY(GetFileAttributesW(junction.toStdWString().c_str()) & FILE_ATTRIBUTE_REPARSE_POINT);
        aidfame::CacheStore redirected(junction);
        QVERIFY(!redirected.initialize()); QVERIFY(!redirected.clear());
        QVERIFY(QFileInfo::exists(sentinel)); QCOMPARE(real.size(),8);
    }
    void preservesUnownedFilesAndSubdirectories() {
        QTemporaryDir parent;
        const auto path = QDir(parent.path()).filePath("cache-v1");
        aidfame::CacheStore cache(path); QVERIFY(cache.initialize());
        for (const auto* name : {"aidfame-test.afcache","user.mp4","unknown.cache"}) {
            QFile file(QDir(path).filePath(name)); QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write("1234"),4);
        }
        QVERIFY(QDir().mkpath(QDir(path).filePath("nested")));
        QFile nested(QDir(path).filePath("nested/aidfame-nested.afcache")); QVERIFY(nested.open(QIODevice::WriteOnly)); nested.write("safe"); nested.close();
        QCOMPARE(cache.size(),4); QVERIFY(cache.clear()); QCOMPARE(cache.size(),0);
        QVERIFY(QFileInfo::exists(QDir(path).filePath("user.mp4")));
        QVERIFY(QFileInfo::exists(QDir(path).filePath("unknown.cache")));
        QVERIFY(QFileInfo::exists(nested.fileName()));
    }
    void refusesUnownedDirectory() {
        QTemporaryDir root; QFile file(QDir(root.path()).filePath("aidfame-user.afcache"));
        QVERIFY(file.open(QIODevice::WriteOnly)); file.write("user"); file.close();
        aidfame::CacheStore cache(root.path()); QVERIFY(!cache.clear()); QVERIFY(file.exists());
        aidfame::CacheStore driveRoot("D:/"); QVERIFY(!driveRoot.clear());
    }
};
QTEST_GUILESS_MAIN(CacheTests)
#include "CacheTests.moc"
