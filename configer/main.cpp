/****************************************************************************
**
** Copyright (C) 2007  Ralf Habacker <ralf.habacker@freenet.de>^
**
** All rights reserved.
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
#define  QT_NO_DEBUG_OUTPUT

#include <QtDebug>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include "package.h"
#include "misc.h"
#include <iostream>
using namespace std;

static const QStringList g_fileFilter = QString("*.zip *.tbz *.tar.bz2").split(' ');
bool findHintFiles(const QString &dir, QStringList &files)
{
    QDir d(dir);
    QStringList filters;
    filters << "*.hint";
    d.setFilter(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::AllDirs);
    d.setNameFilters(filters);
    d.setSorting(QDir::Name);
    QFileInfoList list = d.entryInfoList();

    Q_FOREACH(const QFileInfo fi, list) {
        if (fi.isDir()) {
          findHintFiles(fi.absoluteFilePath(),files);
        }
        else {
            files << fi.absoluteFilePath();
            qDebug() << fi.absoluteFilePath();
        }
    }
    return true;
}

QByteArray createMD5Hash(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
         return QByteArray();
    QByteArray content = file.readAll();
    return QCryptographicHash::hash(content, QCryptographicHash::Md5).toHex();
}

bool createMD5Sums(const QStringList &hintFiles )
{
    Q_FOREACH(const QFileInfo hintFile, hintFiles) {

        qDebug() << hintFile.absoluteFilePath();

        QFile md5sumFile(hintFile.absolutePath()+ "/" + hintFile.baseName()+".sum");
        if (!md5sumFile.open(QFile::WriteOnly | QFile::Truncate))
            continue;

        QTextStream out(&md5sumFile);

        QDir d(hintFile.absolutePath());
        QStringList filters = g_fileFilter;
        filters << "*.hint";
        d.setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);
        d.setNameFilters(filters);
        d.setSorting(QDir::Name);
        QFileInfoList list = d.entryInfoList();
        Q_FOREACH (const QFileInfo fi, list) {
            if (!fi.isDir()) {
                out << createMD5Hash(fi.absoluteFilePath()) << " " << fi.size() << " " << fi.fileName() << "\n";
                qDebug() << createMD5Hash(fi.absoluteFilePath()) << fi.fileName();
            }
        }
    }
    return true;
}

bool createCygwinLikeSetupIni(QTextStream &out, const QString &root, const QStringList &hintFiles)
{
    QString _root = root.toLower();
    _root.replace("\\","/");
    if (!root.endsWith("/"))
        _root += "/";
    out << "# This file is automatically generated.  If you edit it, your\n"
          "# edits will be discarded next time the file is generated.\n"
          "# See http://cygwin.com/setup.html for details.\n"
          "#\n\n";
    out << "setup-timestamp: 1191928206\n";
    out << "setup-version: 2.573.2.2\n";

    foreach(QFileInfo hintFile, hintFiles) {

        QFile file(hintFile.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        out << "\n@ " + hintFile.baseName() + "\n";
        while (!file.atEnd())
            out << file.readLine();
        file.close();

        QDir d(hintFile.absolutePath());
        QStringList filters = g_fileFilter;
        d.setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);
        d.setNameFilters(filters);
        d.setSorting(QDir::Name);
        QFileInfoList list = d.entryInfoList();
        QFileInfoList::ConstIterator it = list.constBegin();
        QFileInfoList::ConstIterator end = list.constEnd();
        QString version;
        for ( ; it != end; ++it) {
            const QFileInfo &fi = *it;
            if (fi.isDir()) {
                continue;
            }
            else {
                QString pkgName;
                QString pkgVersion;
                QString pkgType;
                QString pkgFormat;
                Package::fromFileName(fi.completeBaseName(), pkgName, pkgVersion, pkgType,pkgFormat);
                if (version != pkgVersion) {
                    out << "version: " + pkgVersion + "\n";
                    version = pkgVersion;
                }
                QString postfix;
                if (pkgName.endsWith("-msvc"))
                    postfix = "-msvc: ";
                else if (pkgName.endsWith("-mingw"))
                    postfix = "-mingw: ";
                else
                    postfix = ": ";
                out << pkgType + postfix + fi.absoluteFilePath().toLower().replace(_root,"")
                        + " " + QString::number(fi.size()) + " " + createMD5Hash(fi.absoluteFilePath()) + "\n";
            }
        }
    }
    return true;
}

bool createConfigTxt(QTextStream &out, const QString &root, const QStringList &hintFiles,bool withHeader = true)
{
    QString _root = root.toLower().replace('\\','/');
    if (!root.endsWith('/'))
        _root += '/';

    if (withHeader) {
        out << "@format 1.1\n"
	       "; this format is *prelimary* and may be changed without prior notice\n"
    	       "@mirror http://webdev.cegit.de/snapshots/kde-windows Europe,Germany,Essen\n"
        	   "@mirror http://download.cegit.de/kde-windows Europe,Germany,Osnabrueck\n"
	        ;
        //out << "setup-timestamp: 1191928206\n";
        //out << "setup-version: 2.573.2.2\n";

    }
    out << "; This file is automatically generated.  If you edit it, your\n"
	   "; edits will be discarded next time the file is generated.\n"
    	   ";\n\n";

    foreach(QFileInfo hintFile, hintFiles) {

        HintFileDescriptor hint;
        if (!parseHintFile(hintFile.absoluteFilePath(),hint))
            continue;

        QStringList compilers;
        QDir d(hintFile.absolutePath());
        QStringList filters = g_fileFilter;
        d.setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);
        d.setNameFilters(filters);
        d.setSorting(QDir::Name);
        QFileInfoList list = d.entryInfoList();

        out << "\n@package " + hintFile.baseName() + "\n";
        out << "@notes " << hint.shortDesc << "\n";
        out << "@details " << hint.longDesc << "\n";
        out << "@category " << hint.categories << "\n";
        out << "@require " << hint.requires << "\n";

        QString version;
        QString pkgName;
        QString pkgVersion;
        QString pkgType;
        QString pkgFormat;
        Q_FOREACH (const QFileInfo fi, list) {
            if (fi.isDir())
                continue;
            Package::fromFileName(fi.completeBaseName(), pkgName, pkgVersion, pkgType,pkgFormat);
            if (version != pkgVersion) {
                out << "@version " + pkgVersion + "\n";
                version = pkgVersion;
            }
            QString postfix;
/* uncomment to collect package for several compilers
                if (pkgName.endsWith("-msvc"))
                    postfix = "-msvc ";
                else if (pkgName.endsWith("-mingw"))
                    postfix = "-mingw ";
                else
*/
            postfix = " ";
            out << "@url-" + pkgType + postfix + fi.absoluteFilePath().toLower().replace(_root,"")
                    + " " + QString::number(fi.size()) + " " + createMD5Hash(fi.absoluteFilePath()) + "\n";
        }
    }
    return true;
}

QStringList &addDeps(QStringList &deps, const QStringList &add)
{
	foreach(QString dep, add)
	{
		if (dep.contains(" "))
		{
			foreach(QString adep, dep.split(" "))
			{
				if (!deps.contains(adep))
					deps << adep;
			}
		}
		else if (!deps.contains(dep))
			deps << deps;
	}
    return deps;
}


bool printDependencies(const QString &root)
{
    QStringList hintFiles;
    findHintFiles(root,hintFiles);
    QHash<QString,QStringList> deps;

    foreach(QFileInfo hintFile, hintFiles)
    {
        QString pkgName = hintFile.baseName();
        qDebug() << "parsing" << pkgName;
        HintFileDescriptor hint;
        if (!parseHintFile(hintFile.absoluteFilePath(),hint))
            continue;
        if (hint.requires.isEmpty())
        {
            deps[pkgName] = QStringList();
            continue;
        }
        if (deps.contains(pkgName))
            addDeps(deps[pkgName],hint.requires.split(" "));
        else
            deps[pkgName] = hint.requires.split(" ");
    }
     QHashIterator<QString, QStringList> i(deps);
     while (i.hasNext()) {
        i.next();
        QString x = QString("%1 : %2").arg(i.key()).arg(i.value().join(" "));
        cout << x.toAscii().data() << endl;
    }
    return true;
}

static void printHelp(const QString &addInfo)
{
    QTextStream ts(stderr);
    ts << QDir::convertSeparators(QCoreApplication::applicationFilePath());
    if(!addInfo.isEmpty())
        ts << ": " << addInfo;
    ts << "\n";
    ts << "Options: "
       << "\n\t\t"      << "-root <path to package files>"
       << "\n\t\t"      << "-md5 create md5 hashes"
       << "\n\t\t"      << "-printdeps list dependencies of all packages"
       << "\n\t\t"      << "-o <filename> save output into <filename> instead printing on stdout"
       << "\n\t\t"      << "-verbose display verbose processing informations"
       << "\n";

    ts.flush();
    exit(1);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QStringList args = app.arguments();
    QString root;
    QString outFile;
    QString headerFile;
    QFileInfo rootDir;
    bool createMD5 = false;
    bool verbose = false;
    bool printDeps = false;

    for (int i = 1; i < args.size(); i++)
    {
        bool hasValue = i < args.count()-1;

        if (args[i] == "-md5")
            createMD5 = true;
        else if (args[i] == "-root" && hasValue)
        {
            root = args[++i];
            rootDir = QFileInfo(root);
        }
        else if (args[i] == "-header" && hasValue) {
            headerFile = args[++i];
        }
        else if (args[i] == "-o" && hasValue) {
            outFile = args[++i];
        }
        else if (args[i] == "-printdeps")
            printDeps = true;
        else if (args[i] == "-verbose")
            verbose = 1;
        else
            printHelp(QString("unknown command line parameter(s): %1").arg(args[i]));
    }

    if(root.isEmpty())
       printHelp("-root not specified");

    if(!rootDir.isDir() || !rootDir.isReadable())
       printHelp(QString("Root path %1 is not accessible").arg(root));

    if (printDeps)
    {
        printDependencies(root);
        return 0;
    }

    QTextStream *out = 0;
    QFile *f = 0;

    if (!outFile.isEmpty()) {
        f = new QFile(outFile);
        if (f->open(QFile::WriteOnly | QFile::Text))
            out = new QTextStream(f);
    else
        qWarning() << "could not open file" << outFile;
    }
    if (!out)
        out = new QTextStream(stdout);

    if (!headerFile.isEmpty()) {
        QFile header(headerFile);
        header.open(QFile::ReadOnly);
        *out << header.readAll();
        header.close();
    }

    QStringList files;
    findHintFiles(root,files);
    if (createMD5)
        createMD5Sums(files);

    createConfigTxt(*out,root,files,headerFile.isEmpty());

    delete out;
    delete f;
    return 0;
}

