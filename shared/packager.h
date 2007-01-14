/****************************************************************************
**
** Copyright (C) 2006 Ralf Habacker. All rights reserved.
**
** This file is part of the KDE installer for windows
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#ifndef PACKAGER_H
#define PACKAGER_H

#include <QStringList>
#include <QList>
#include <QRegExp>
#include "package.h"

class Packager {
    public: 
      // don't know how to make usable package::Type
      enum Type { NONE = 0, BIN = 1 ,LIB = 2 ,DOC = 4 ,SRC = 8, ALL = 15};
      Packager::Packager(const QString &packageName, const QString &packageVersion,const QString &notes=QString());


      bool generatePackageFileList(QStringList &result, const QString &dir, Packager::Type=Packager::BIN);
      bool createManifestFiles(QStringList &fileList, const QString &dir, Packager::Type type);

      bool makePackage(const QString &dir, const QString &destdir=QString());
      bool createZipFile(const QString &fileName, const QStringList &files, const QString &rootDir=QString());
      
    protected: 
      bool generateFileList(QStringList &result, const QString &root, const QString &subdir, const QString &filter, const QList<QRegExp> &excludeList);
      bool generateFileList(QStringList &result, const QString &dir, const QString &filter, const QString &exclude);
      
    private: 
      QString m_name;
      QString m_version;
      QString m_notes;
}; 

#endif
