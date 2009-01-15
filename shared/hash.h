/****************************************************************************
**
** Copyright (C) 2006-2009 Ralf Habacker. All rights reserved.
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

#ifndef HASH_H
#define HASH_H

class QFile;
class QCryptographicHash;

#include <QtCore/QByteArray>

class Hash {
    public:
        typedef enum { None, MD5, SHA1 } Type; 
        
        Hash(Type type = MD5);
        ~Hash();
        
        /**
            set hash type 
            note that setting the type after already 
            calling addData() will reset the internal hash value
        */
        void setType(Type type) { m_type = type; }
        bool setType(const QString &type);
        
        /// return hash type
        Hash::Type type() { return m_type; }

        /// compute hash from file content 
        QByteArray hash(QFile &f);

        /// compute hash from file content 
        QByteArray hash(const QString &file);

        /// the following three methods supports adding partial data 
        /// reset hash value
        void reset();
        /// Adds the first length chars of data to the cryptographic hash.
        void addData( const char *data, int size );
        /// Returns the final hash value.
        QByteArray result() const;
        
        /// validate hash string
        static bool isHash (const QByteArray &str);

        /// return type of hash
        static Hash::Type isType(const QByteArray &str);

        static Hash &instance();
        
        /// return md5 hash 
        static QByteArray md5(QFile &f);
        static QByteArray md5(const QString &f);
        /// return sha1 hash 
        static QByteArray sha1(QFile &f);
        static QByteArray sha1(const QString &f);
    protected: 
        Type m_type;
        QCryptographicHash *m_cryptoHash;
};

#endif
