/***************************************************************************
 *  This file is part of Saaghar, a Persian poetry software                *
 *                                                                         *
 *  Copyright (C) 2010-2015 by S. Razi Alavizadeh                          *
 *  E-Mail: <s.r.alavizadeh@gmail.com>, WWW: <http://pozh.org>             *
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 3 of the License,         *
 *  (at your option) any later version                                     *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details                            *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program; if not, see http://www.gnu.org/licenses/      *
 *                                                                         *
 ***************************************************************************/

#ifndef OUTLINEMODEL_H
#define OUTLINEMODEL_H

#include <QAbstractItemModel>

struct OutlineNode;

class OutlineModel : public QAbstractItemModel
{
public:
    static OutlineModel* instance();
    ~OutlineModel();

    enum DataRole {
        IDRole = Qt::UserRole + 1,
        TitleRole
    };

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;


    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void clear();

private:
    Q_DISABLE_COPY(OutlineModel)
    OutlineModel(QObject* parent = 0);
    static OutlineModel* s_instance;

    inline bool indexValid(const QModelIndex &index) const {
         return index.isValid() && (index.row() >= 0) && (index.column() >= 0) && (index.model() == this);
    }

    OutlineNode* node(const QModelIndex &index) const;
    OutlineNode* node(int row, OutlineNode* parent) const;
    void clear(OutlineNode *parent) const;
    QVector<OutlineNode *> children(OutlineNode *parent, bool useCache = false) const;
};

#endif // OUTLINEMODEL_H