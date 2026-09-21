#include "Settings/LocalStore.h"
#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
class SettingsTests : public QObject {
    Q_OBJECT
private slots:
    void unreadableStateCannotBeOverwritten() {
        QTemporaryDir root;
        const auto path = QDir(root.path()).filePath("state.json");
        QVERIFY(QDir().mkpath(path));
        aidfame::LocalStore store(root.path());
        QVERIFY(!store.load()); QVERIFY(!store.save());
        QVERIFY(QFileInfo(path).isDir());
    }
    void roundTripAndBounds() {
        QTemporaryDir root;
        aidfame::LocalStore store(root.path()); QVERIFY(store.load());
        store.settings["seekStepSeconds"] = 30;
        store.settings["volume"] = 999;
        store.settings["speed"] = 4.5;
        store.settings["screenshotFormat"] = "jpg";
        store.remember("D:/movie.mp4",5,12);
        store.remember("d:/MOVIE.mp4",8,12);
        QCOMPARE(store.history.size(),1);
        QVERIFY(store.save());
        aidfame::LocalStore restored(root.path()); QVERIFY(restored.load());
        QCOMPARE(restored.settings["seekStepSeconds"].toInt(),30);
        QCOMPARE(restored.settings["volume"].toInt(),80);
        QCOMPARE(restored.settings["speed"].toDouble(),1.0);
        QCOMPARE(restored.settings["screenshotFormat"].toString(),QStringLiteral("jpg"));
        QCOMPARE(restored.history.first().toObject()["positionSeconds"].toDouble(),8.0);
        restored.forget("D:/movie.mp4"); QVERIFY(restored.history.isEmpty());
        for (int index = 0; index < 80; ++index) restored.remember(QStringLiteral("D:/%1.mp4").arg(index),1,12);
        QCOMPARE(restored.history.size(),50);
    }
    void corruptRecovery() {
        QTemporaryDir root;
        QFile file(QDir(root.path()).filePath("state.json")); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("{invalid"); file.close();
        aidfame::LocalStore store(root.path()); QVERIFY(!store.load());
        QVERIFY(!store.error.isEmpty()); QCOMPARE(store.settings["seekStepSeconds"].toInt(),5);
        QCOMPARE(QDir(root.path()).entryList({"state.json.invalid-*"},QDir::Files).size(),1);
        QVERIFY(store.save()); QVERIFY(store.load());
    }
};
QTEST_GUILESS_MAIN(SettingsTests)
#include "SettingsTests.moc"
