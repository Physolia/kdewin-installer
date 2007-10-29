/****************************************************************************
**
** Copyright (C) 2007  Christian Ehrlicher <ch.ehrlicher@gmx.de>.
** All rights reserved.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Library General Public
** License as published by the Free Software Foundation; either
** version 2 of the License, or (at your option) any later version.
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
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "bzip2filter.h"

#include <QIODevice>
#include <QString>

#include <bzlib.h>

//
// BZip2IODevice::Private
//
class BZip2IODevice::Private
{
  public:
    Private(BZip2IODevice *parent_, QIODevice *dev, int bs)
  : parent(parent_),
    device(dev),
    iBufSize(1024*1024),
    initialized(false),
    blocksize(bs)
    {}

    bool initializeCompress();
    qint64 doCompress(const char *in, qint64 iLen);
    bool finishCompress();
    bool initializeDecompress();
    qint64 doDecompress(char *out, qint64 iMaxLen);
    bool finishDecompress();

    BZip2IODevice   *parent;
    QIODevice   *device;
    unsigned int iBufSize;
    bool         initialized;
    unsigned int blocksize;
    bz_stream    stream;
};

bool BZip2IODevice::Private::initializeCompress()
{
  if(initialized)
    return true;
  memset(&stream, 0, sizeof(stream));
  int blockSize = (blocksize > 0 && blocksize < 10) ? blocksize : 5;
  int ret = BZ2_bzCompressInit (&stream, blockSize, 0, 0);
  if(ret != BZ_OK) {
    parent->setErrorString(QString(QLatin1String("Error initializing bzip2: %1")).arg(ret));
    initialized = false;
  } else {
    initialized = true;
  }

  return initialized;
}

qint64 BZip2IODevice::Private::doCompress(const char *in, qint64 iMaxLen)
{
  int ret = -1;
  if(!initialized) {
    finishCompress();
    parent->setErrorString(QLatin1String("Internal Error - bzip not initialized"));
    return -1;
  }
  if(iMaxLen > 2*1024*1024*1024LL) {
    finishCompress();
    parent->setErrorString(QLatin1String("Currently can't handle more than 2GB!"));
    return -1;
  }

  QByteArray outBuf(iBufSize, '\0');
  stream.next_in  = const_cast<char*>(in);
  stream.avail_in = iMaxLen;

  do {
    stream.next_out = outBuf.data();
    stream.avail_out = outBuf.size();

    ret = BZ2_bzCompress(&stream, BZ_RUN);
    if(ret != BZ_RUN_OK) {
      finishCompress();
      parent->setErrorString(QString(QLatin1String("Error executing BZ2_bzCompress: %1")).arg(ret));
      return false;
    }
    int iBytesToWrite = outBuf.size() - stream.avail_out;
    if(iBytesToWrite) {
      if(device->write(outBuf.constData(), iBytesToWrite) != iBytesToWrite) {
        finishCompress();
        parent->setErrorString(QLatin1String("Error writing to QIODevice"));
        return -1;
      }
    }
  } while(stream.avail_in != 0);

  return stream.next_in - in;
}

bool BZip2IODevice::Private::finishCompress()
{
  if(!initialized)
    return true;

  int ret = -1;
  QByteArray outBuf(iBufSize, '\0');
  do {
    stream.next_out = outBuf.data();
    stream.avail_out = outBuf.size();
    ret = BZ2_bzCompress(&stream, BZ_FINISH);
    if(ret != BZ_FINISH_OK && ret != BZ_STREAM_END) {
      finishCompress();
      parent->setErrorString(QString(QLatin1String("Error executing BZ2_bzCompress: %1")).arg(ret));
      return false;
    }
    int iBytesToWrite = outBuf.size() - stream.avail_out;
    if(device->write(outBuf.constData(), iBytesToWrite) != iBytesToWrite) {
      finishCompress();
      parent->setErrorString(QLatin1String("Error writing to QIODevice"));
      return -1;
    }
  } while( ret != BZ_STREAM_END );
  BZ2_bzCompressEnd(&stream);

  initialized = false;
  return true;
}

bool BZip2IODevice::Private::initializeDecompress()
{
  if(initialized)
    return true;
  memset(&stream, 0, sizeof(stream));
  int blockSize = (blocksize > 0 && blocksize < 10) ? blocksize : 5;
  int ret = BZ2_bzDecompressInit (&stream, 0, 0);
  if(ret != BZ_OK) {
    parent->setErrorString(QString(QLatin1String("Error initializing bzip2: %1")).arg(ret));
    initialized = false;
  } else {
    initialized = true;
  }

  return initialized;
}

qint64 BZip2IODevice::Private::doDecompress(char *out, qint64 iMaxLen)
{
  int ret = -1;
  int iBytesWritten = 0;
  if(!initialized) {
    parent->setErrorString(QLatin1String("Internal Error - bzip not initialized"));
    finishDecompress();
    return -1;
  }
  if(iMaxLen > 2*1024*1024*1024LL) {
    finishCompress();
    parent->setErrorString(QLatin1String("Currently can't handle more than 2GB!"));
    return -1;
  }

  QByteArray inBuf;
  stream.next_out  = out;
  stream.avail_out = iMaxLen;
  do {
    if(ret != BZ_STREAM_END) {
      inBuf = device->read(iBufSize);
      stream.next_in = inBuf.data();
      stream.avail_in = inBuf.size();
    }

    ret = BZ2_bzDecompress(&stream);
    if(ret != BZ_OK && ret != BZ_STREAM_END) {
      parent->setErrorString(QString(QLatin1String("Error executing BZ2_bzCompress: %1")).arg(ret));
      finishDecompress();
      return -1;
    }
  } while(stream.avail_out != 0 || ret != BZ_STREAM_END);
  return stream.next_out - out;
}

bool BZip2IODevice::Private::finishDecompress()
{
  if(!initialized)
    return true;
  BZ2_bzDecompressEnd(&stream);

  initialized = false;
  return true;
}

//
// BZip2IODevice
//
BZip2IODevice::BZip2IODevice(QIODevice *dev, int blocksize)
  : d(new Private(this, dev, blocksize))
{}

BZip2IODevice::~BZip2IODevice()
{
  close();
  delete d;
}

void BZip2IODevice::setBufferSize(unsigned int size)
{
  d->iBufSize = size > 1024 ? size : 1024;
}

void BZip2IODevice::setBlockSize(unsigned int size)
{
  if(isOpen())
    return;
  d->blocksize = size;
}

bool BZip2IODevice::isSequential() const
{
  return true;
}

bool BZip2IODevice::open(OpenMode mode)
{
  if((mode & QIODevice::ReadWrite) == 0) {
    setErrorString(QString(QLatin1String("Unsupported OpenMode %1")).arg(mode));
    return false;
  }
  if((mode & QIODevice::ReadWrite) == QIODevice::ReadWrite) {
    setErrorString(QLatin1String("Can't read and write simultanionously"));
    return false;
  }
  if((mode & QIODevice::ReadOnly) == QIODevice::ReadOnly) {
    if(!d->device) {
      setErrorString(QLatin1String("No QIODevice for reading available"));
      return false;
    }
    if((d->device->openMode() & QIODevice::ReadOnly) == 0) {
      setErrorString(QLatin1String("QIODevice not opened for reading"));
      return false;
    }
    if(!d->initializeDecompress())
      return false;
  }
  else {
    if(!d->device) {
      setErrorString(QLatin1String("No QIODevice for writing available"));
      return false;
    }
    if((d->device->openMode() & QIODevice::WriteOnly) == 0) {
      setErrorString(QLatin1String("QIODevice not opened for writing"));
      return false;
    }
    if(!d->initializeCompress())
      return false;
  }
  setOpenMode(mode);
  return QIODevice::open(mode);
}

void BZip2IODevice::close()
{
  if(!isOpen())
    return;
  if((openMode() & QIODevice::ReadOnly) == QIODevice::ReadOnly) {
    d->finishDecompress();
  } else {
    d->finishCompress();
  }
  QIODevice::close();
}

qint64 BZip2IODevice::readData(char * data, qint64 maxSize)
{
  return d->doDecompress(data, maxSize);
}

qint64 BZip2IODevice::writeData(const char * data, qint64 maxSize)
{
  return d->doCompress(data, maxSize);
}
