
/***************************************************************************
 *   This file is part of the AudioThumbs.                                 *
 *   Copyright (C) 2009 Vytautas Mickus <vmickus@gmail.com>                *
 *   Copyright (C) 2016 Anthony Fieroni <bvbfan@abv.com>                   *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it  under the terms of the GNU Lesser General Public License version  *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#include "AudioThumbs.h"

#include <QFile>
#include <QImage>
#include <QMimeType>
#include <QMimeDatabase>
#include <QScopedPointer>

#include <taglib/fileref.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/attachedpictureframe.h>

#include <FLAC++/metadata.h>

#define QCM_duplicate(t, d, s) t = QByteArray(d, s)

extern "C"
{
    Q_DECL_EXPORT ThumbCreator* new_creator()
    {
        return new ATCreator;
    }
}

ATCreator::ATCreator()
{
}

ATCreator::~ATCreator()
{
}

bool ATCreator::create(const QString &path, int, int, QImage &img)
{
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(path);
    if (!type.isValid()) {
        return false;
    }

    if (type.inherits("audio/mpeg")) {
        QByteArray fn = QFile::encodeName(path);
        TagLib::FileRef fileRef = TagLib::FileRef(fn);
        if (fileRef.isNull()) {
            return false;
        }

        auto file = dynamic_cast<TagLib::MPEG::File*>(fileRef.file());
        if (!file || !file->ID3v2Tag()) {
            return false;
        }

        const auto flm = file->ID3v2Tag()->frameListMap();
        if (flm["APIC"].isEmpty()) {
            return false;
        }

        const auto apicFrame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(flm["APIC"].front());
        if (!apicFrame) {
            return false;
        }

        img.loadFromData((unsigned char *)apicFrame->picture().data(), apicFrame->picture().size());
        return true;
    }
    if (type.inherits("audio/x-flac")) {
        QByteArray fn = QFile::encodeName(path);
        FLAC::Metadata::Chain m_chain;
        if (!m_chain.read(fn)) {
            return false;
        }

        FLAC::Metadata::Iterator mdit;
        mdit.init(m_chain);
        while (mdit.is_valid()) {
            auto mdt = mdit.get_block_type();
            if (mdt == FLAC__METADATA_TYPE_PICTURE) {
                QScopedPointer<FLAC::Metadata::Prototype> proto(mdit.get_block());
                if (proto) {
                    auto pic = dynamic_cast<FLAC::Metadata::Picture*>(proto.data());
                    if (pic) {
                        QByteArray ba;
                        QCM_duplicate(ba, reinterpret_cast<const char*>(pic->get_data()), pic->get_data_length());
                        img.loadFromData(ba);
                        return true;
                    }
                }
                if (!mdit.next()) {
                    break;
                }
            }
        }
    }
    return false;
}

ThumbCreator::Flags ATCreator::flags() const
{
    return (Flags)(DrawFrame | BlendIcon);
}
