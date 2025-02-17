/*
SPDX-FileCopyrightText: 2010 Marco Mentasti <marcomentasti@gmail.com>

SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QContiguousCache>
#include <QSqlQueryModel>
#include <QSqlRecord>

class CachedSqlQueryModel : public QSqlQueryModel
{
public:
    explicit CachedSqlQueryModel(QObject *parent = nullptr, int cacheCapacity = 1000);

    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const override;
    QSqlRecord record(int row) const;
    void clear() override;

    int cacheCapacity() const;

public Q_SLOTS:
    void clearCache();
    void setCacheCapacity(int);

protected:
    void queryChange() override;

private:
    void cacheRecords(int from, int to) const;

    mutable QContiguousCache<QSqlRecord> cache;
};
