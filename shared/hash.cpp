/****************************************************************************
**
** Copyright (C) 2006-2009 Ralf Habacker. All rights reserved.
** Copyright (C) 2006-2007 Christian Ehrlicher <ch.ehrlicher@gmx.de>
**
** This file is part of the KDE installer for windows
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Library General Public
** License version 2 as published by the Free Software Foundation.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.
**
** You should have received a copy of the GNU Library General Public License
** along with this library; see the file COPYING.LIB.  If not, write to
** the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
** Boston, MA 02110-1301, USA.
**
****************************************************************************/

#include "config.h"
#include "hash.h"

#include <QtDebug>
#include <QDir>
#include <QByteArray>
#include <QFile>
#include <QtCore/QCryptographicHash>

Hash::Hash() : m_type(MD5)
{
}

Hash::Hash(Type type) : m_type(type)
{
}

Hash &Hash::instance() 
{
    static Hash instance;
    return instance;
}

QByteArray Hash::hash(QFile &file)
{
    qDebug() << "computing hash with type" << (m_type == MD5 ? "MD5" : "SHA1");
    static const int bufSize = 1024*1024;
    QCryptographicHash hash( m_type == MD5 ? QCryptographicHash::Md5 : QCryptographicHash::Sha1 );
    QByteArray ba;
    ba.resize ( bufSize );
    qint64 iBytesRead;
    while ( ( iBytesRead = file.read ( ba.data(), bufSize ) ) > 0 )
        hash.addData ( ba.data(), iBytesRead );
    return hash.result();
}

QByteArray Hash::hash(const QString &file)
{
    QFile f(file);
    if(!f.open(QIODevice::ReadOnly))
      return QByteArray();
    return hash(f);
}

bool Hash::isHash (const QByteArray &str)
{
    int len = str.length();
    if ( len != 32 && len != 40 )
        return false;
    for ( int i = 0; i < len; i++ ) {
        const char c = str[i];
        if ( c < '0' || c > '9' ) {
            if ( ( c < 'a' || c > 'f' ) && ( c < 'A' || c > 'F' ) )
                return false;
        }
    }
    return true;
}


QByteArray Hash::md5(QFile &f)
{
    Hash hash(MD5);
    return hash.hash(f);
}

QByteArray Hash::md5(const QString &f)
{
    Hash hash(MD5);
    return hash.hash(f);
}

QByteArray Hash::sha1(QFile &f)
{
    Hash hash(SHA1);
    return hash.hash(f);
}

QByteArray Hash::sha1(const QString &f)
{
    Hash hash(SHA1);
    return hash.hash(f);
}