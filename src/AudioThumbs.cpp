
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
#include <taglib/apetag.h>
#include <taglib/mp4tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/mp4file.h>
#include <taglib/oggfile.h>
#include <taglib/apefile.h>
#include <taglib/mpegfile.h>
#include <taglib/flacfile.h>
#include <taglib/wavpackfile.h>
#include <taglib/xiphcomment.h>
#include <taglib/flacpicture.h>
#include <taglib/attachedpictureframe.h>

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
        TagLib::FileRef fileRef(QFile::encodeName(path));
        if (fileRef.isNull()) {
            return false;
        }
        auto file = dynamic_cast<TagLib::MPEG::File*>(fileRef.file());
        if (!file || !file->ID3v2Tag()) {
            return false;
        }
        auto flm = file->ID3v2Tag()->frameListMap();
        if (flm["APIC"].isEmpty()) {
            return false;
        }
        auto apicFrame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(flm["APIC"].front());
        if (!apicFrame) {
            return false;
        }
        img.loadFromData((const uchar *)apicFrame->picture().data(), apicFrame->picture().size());
        return true;
    }
    if (type.inherits("audio/x-flac") || type.inherits("audio/flac")) {
        TagLib::FLAC::File file(QFile::encodeName(path));
        const auto pictureList = file.pictureList();
        for (const auto &picture : pictureList) {
            if (picture->type() != TagLib::FLAC::Picture::FrontCover) {
                continue;
            }
            QByteArray data;
            const auto coverData = picture->data();
            data.setRawData(coverData.data(), coverData.size());
            img.loadFromData(data);
            return true;
        }
    }
    if (type.inherits("audio/mp4")) {
        TagLib::MP4::File file(QFile::encodeName(path));
        const auto &map = file.tag()->itemMap();
        for (const auto &coverList : map) {
            auto coverArtList = coverList.second.toCoverArtList();
            if (coverArtList.isEmpty()) {
                continue;
            }
            QByteArray data;
            const auto coverData = coverArtList[0].data();
            data.setRawData(coverData.data(), coverData.size());
            img.loadFromData(data);
            return true;
        }
    }
    if (type.inherits("audio/x-monkeys-audio") || type.inherits("audio/x-vw")) {
        TagLib::FileRef fileRef(QFile::encodeName(path));
        if (fileRef.isNull()) {
            return false;
        }
        auto apeFile = dynamic_cast<TagLib::APE::File*>(fileRef.file());
        if (apeFile && !apeFile->hasAPETag()) {
            return false;
        }
        auto wavFile = dynamic_cast<TagLib::WavPack::File*>(fileRef.file());
        if (wavFile && !wavFile->hasAPETag()) {
            return false;
        }
        if (!apeFile && !wavFile) {
            return false;
        }
        const auto &map = apeFile ? apeFile->APETag()->itemListMap() :
                                    wavFile->APETag()->itemListMap();
        for (const auto &item : map) {
            if (item.second.type() != TagLib::APE::Item::Binary) {
                continue;
            }
            QByteArray data;
            auto coverData = item.second.binaryData();
            data.setRawData(coverData.data(), coverData.size());
            img.loadFromData(data);
            return true;
        }
    }
    if (type.inherits("audio/ogg")) {
        TagLib::FileRef fileRef(QFile::encodeName(path));
        if (fileRef.isNull()) {
            return false;
        }
        auto file = dynamic_cast<TagLib::Ogg::File*>(fileRef.file());
        if (!file) {
            return false;
        }
        auto xiphComment = dynamic_cast<TagLib::Ogg::XiphComment*>(file->tag());
        if (!xiphComment) {
            return false;
        }
        const auto pictureList = xiphComment->pictureList();
        for (const auto &picture : pictureList) {
            if (picture->type() != TagLib::FLAC::Picture::FrontCover) {
                continue;
            }
            QByteArray data;
            const auto coverData = picture->data();
            data.setRawData(coverData.data(), coverData.size());
            img.loadFromData(data);
            return true;
        }
    }
    return false;
}

ThumbCreator::Flags ATCreator::flags() const
{
    return (Flags)(DrawFrame | BlendIcon);
}
