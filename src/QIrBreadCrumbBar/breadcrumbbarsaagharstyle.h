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
#ifndef BREADCRUMBBARSAAGHARSTYLE_H
#define BREADCRUMBBARSAAGHARSTYLE_H

#include "qirbreadcrumbbarstyle.h"

QIR_BEGIN_NAMESPACE

class QIRONSHARED_EXPORT BreadCrumbBarSaagharStyle : public QIrStyledBreadCrumbBarStyle
{
public:
    BreadCrumbBarSaagharStyle() {}
    virtual ~BreadCrumbBarSaagharStyle() {}

    virtual void drawControl(QStyle::ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget = 0) const;
};

QIR_END_NAMESPACE

#endif // BREADCRUMBBARSAAGHARSTYLE_H
