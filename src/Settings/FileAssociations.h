#pragma once
#include <QString>
#include <QList>
namespace aidfame {
struct RegistryEntry { QString key; QString name; QString value; };
QList<RegistryEntry> associationEntries(const QString& executable);
bool registerFileAssociations(QString* error);
}
