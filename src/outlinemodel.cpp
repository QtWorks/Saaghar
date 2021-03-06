/***************************************************************************
 *  This file is part of Saaghar, a Persian poetry software                *
 *                                                                         *
 *  Copyright (C) 2010-2016 by S. Razi Alavizadeh                          *
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

#include "outlinemodel.h"
#include "saagharapplication.h"
#include "databasebrowser.h"
#include "databaseelements.h"
#include "saagharwidget.h"

struct OutlineNode {
    OutlineNode() : parent(0), cat(0), populated(false) {}
    ~OutlineNode() {
        qDeleteAll(children);
        children.clear();
        delete cat;
    }
    OutlineNode* parent;
    GanjoorCat* cat;

    mutable QVector<OutlineNode*> children;
    mutable bool populated;
};

namespace
{
static QHash<QString, OutlineNode*> RootNodes;
}

OutlineModel::OutlineModel(const QString &connectionID, QObject* parent)
    : QAbstractItemModel(parent)
    , m_connectionID(connectionID)
{
    if (!RootNodes.contains(m_connectionID)) {
        RootNodes.insert(m_connectionID, new OutlineNode);
    }
}

OutlineNode* OutlineModel::node(const QModelIndex &index) const
{
    OutlineNode* p;
    if (index.isValid()) {
        p = static_cast<OutlineNode*>(index.internalPointer());
    }
    else {
        p = RootNodes.value(m_connectionID);
    }

    Q_ASSERT(p != 0);

    return p;
}

OutlineNode* OutlineModel::node(int row, OutlineNode* parent) const
{
    if (row < 0) {
        qDebug() << "negatiove row:" << row << __LINE__ << __FUNCTION__;
        return 0;
    }

    Q_ASSERT(parent != 0);

    const QVector<OutlineNode*> parentChildren = children(parent, true);

    if (row >= parentChildren.size()) {
        qDebug() << "there is no row:" << row << __LINE__ << __FUNCTION__;
        return 0;
    }

    return const_cast<OutlineNode*>(parentChildren.at(row));
}

void OutlineModel::clear(OutlineNode* parent) const
{
    Q_ASSERT(parent);

    qDeleteAll(parent->children);
    parent->children.clear();
    parent->populated = false;

    delete parent->cat;
    parent->cat = 0;
}

QVector<OutlineNode*> OutlineModel::children(OutlineNode* parent, bool useCache) const
{
    Q_ASSERT(parent);

    if (useCache && parent->populated) {
        return parent->children;
    }

    int parentId = (!parent->cat || parent->cat->_ID == -1) ? 0 : parent->cat->_ID;

    QList<GanjoorCat*> cats = sApp->databaseBrowser()->getSubCategories(parentId, m_connectionID);

    QVector<OutlineNode*> nodeList;
    nodeList.reserve(cats.size());

    for (int i = 0; i < cats.size(); ++i) {
        OutlineNode* n = new OutlineNode();
        n->cat = cats.at(i);
        n->parent = parent;
        n->populated = false;

        nodeList.append(n);
    }

    parent->populated = true;
    parent->children = nodeList;

    return nodeList;
}

QModelIndex OutlineModel::find(const QString &key, const QModelIndex &parent) const
{
    OutlineNode* n = node(parent);

    QVector<OutlineNode*> parentChildren = children(n, true);

    for (int i = 0; i < parentChildren.size(); ++i) {
        OutlineNode* child = parentChildren.at(i);

        if (child && child->cat && child->cat->_Text == key) {
            return index(i, 0, parent);
        }
    }

    return QModelIndex();
}

QModelIndex OutlineModel::find(int id, const QModelIndex &parent) const
{
    OutlineNode* n = node(parent);

    QVector<OutlineNode*> parentChildren = children(n, true);

    for (int i = 0; i < parentChildren.size(); ++i) {
        OutlineNode* child = parentChildren.at(i);

        if (child && child->cat && child->cat->_ID == id) {
            return index(i, 0, parent);
        }
    }

    return QModelIndex();
}

OutlineModel::~OutlineModel()
{
    delete RootNodes.take(m_connectionID);
}

QModelIndex OutlineModel::index(int row, int column, const QModelIndex &parent) const
{
    if (hasIndex(row, column, parent)) {
        return createIndex(row, column, node(row, node(parent)));
    }

    return QModelIndex();
}

QModelIndex OutlineModel::parent(const QModelIndex &child) const
{
    OutlineNode* n = node(child);
    OutlineNode* par = (n ? n->parent : 0);

    if (par == 0 || par == RootNodes.value(m_connectionID)) {
        return QModelIndex();
    }

    const QVector<OutlineNode*> grandParentChildren = par->parent
            ? children(par->parent, true)
            : children(RootNodes.value(m_connectionID), true);

    Q_ASSERT(grandParentChildren.count() > 0);

    int row = grandParentChildren.indexOf(par);

    Q_ASSERT(row >= 0 && row < grandParentChildren.size());

    return createIndex(row, 0, par);
}

int OutlineModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) {
        return 0;
    }

    if (!parent.isValid()) {
        return children(RootNodes.value(m_connectionID), true).size();
    }

    if (parent.model() != this) {
        return 0;
    }

    return children(node(parent), true).size();
}

int OutlineModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant OutlineModel::data(const QModelIndex &index, int role) const
{
    if (!indexValid(index)) {
        return QVariant();
    }

    OutlineNode* indexNode = node(index);

    if (!indexNode) {
        return QVariant();
    }

    GanjoorCat* cat = indexNode->cat;

    if (!cat) {
        return QVariant();
    }

    switch (role) {
    default:
        return QVariant();
    case IDRole:
        return cat->_ID;
    case Qt::ToolTipRole:
    case TitleRole:
    case Qt::DisplayRole:
        return cat->_Text;
    case CategoryRole:
        QVariant var;
        var.setValue(*cat);
        return var;
    }
}

void OutlineModel::clear()
{
    beginResetModel();

    clear(RootNodes.value(m_connectionID));

    endResetModel();
}

bool OutlineModel::isPathValid(const QStringList &pathSections)
{
    bool ok;
    index(pathSections, &ok);

    return ok;
}

QModelIndex OutlineModel::index(const QStringList &pathSections, bool* ok) const
{
    QModelIndex parent = QModelIndex();

    for (int i = 0; i < pathSections.size(); ++i) {
        if (i == 0 && pathSections.at(0).compare(SaagharWidget::rootTitle(), Qt::CaseInsensitive) == 0) {
            continue;
        }

        parent = find(pathSections.at(i), parent);

        if (!parent.isValid()) {
            if (ok) {
                *ok = false;
            }
            return QModelIndex();
        }
        else if (i == pathSections.size() - 1) {
            if (ok) {
                *ok = true;
            }
            return parent;
        }
    }

    if (ok) {
        *ok = true;
    }

    return QModelIndex();
}

QModelIndex OutlineModel::indexFromPath(const QList<int> &path) const
{
    QModelIndex parent;

    foreach (int id, path) {
        parent = find(id, parent);
    }

    qDebug() << "\n===========\n"  << __LINE__ << __FUNCTION__ << path << parent.data(TitleRole).toString() << "\n===========\n" ;
    return parent;
}

QList<int> OutlineModel::pathFromIndex(const QModelIndex &index) const
{
    QList<int> path;
    QModelIndex ind = index;

    while (ind.isValid()) {
        path.append(ind.data(IDRole).toInt());
        ind = parent(ind);
    }

    qDebug() << "\n===========\n" << __LINE__ << __FUNCTION__ << path << index.data(TitleRole).toString() << "\n===========\n" ;
    return path;
}

QList<GanjoorCat> OutlineModel::catPathFromIndex(const QModelIndex &index, bool reversed) const
{
    QList<GanjoorCat> cats;
    QModelIndex ind = index;

    while (ind.isValid()) {
        if (!reversed) {
            cats << ind.data(CategoryRole).value<GanjoorCat>();
        }
        else {
            cats.prepend(ind.data(CategoryRole).value<GanjoorCat>());
        }
        ind = parent(ind);
    }

    qDebug() << "\n===========\n" << __LINE__ << __FUNCTION__ << cats.size() << index.data(IDRole).toInt() << "\n===========\n" ;
    return cats;
}
