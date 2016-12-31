
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

#include <taglib/apetag.h>
#include <taglib/mp4tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/fileref.h>
#include <taglib/mp4file.h>
#include <taglib/wavfile.h>
#include <taglib/apefile.h>
#include <taglib/mpcfile.h>
#include <taglib/mpegfile.h>
#include <taglib/aifffile.h>
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

namespace TagLib {
    namespace RIFF {
        namespace AIFF {
            struct FileExt : public File {
                using File::File;
                ID3v2::Tag* ID3v2Tag() const {
                    return tag();
                }
            };
        }
    }
}

template<class T> static
bool parseID3v2Tag(T &file, QImage &img)
{
    if (!file.hasID3v2Tag()) {
        return false;
    }
    const auto &map = file.ID3v2Tag()->frameListMap();
    if (map["APIC"].isEmpty()) {
        return false;
    }
    auto apicFrame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(map["APIC"].front());
    if (!apicFrame) {
        return false;
    }
    const auto coverData = apicFrame->picture();
    QByteArray data(coverData.data(), coverData.size());
    img.loadFromData(data);
    return true;
}

template<class T> static
bool parseFlacTag(T &file, QImage &img) {
    const auto pictureList = file.pictureList();
    for (const auto &picture : pictureList) {
        if (picture->type() != TagLib::FLAC::Picture::FrontCover) {
            continue;
        }
        const auto coverData = picture->data();
        QByteArray data(coverData.data(), coverData.size());
        img.loadFromData(data);
        return true;
    }
    return false;
}

template<class T> static
bool parseMP4Tag(T &file, QImage &img)
{
    if (!file.hasMP4Tag()) {
        return false;
    }
    const auto &map = file.tag()->itemMap();
    for (const auto &coverList : map) {
        auto coverArtList = coverList.second.toCoverArtList();
        if (coverArtList.isEmpty()) {
            continue;
        }
        const auto coverData = coverArtList[0].data();
        QByteArray data(coverData.data(), coverData.size());
        img.loadFromData(data);
        return true;
    }
    return false;
}

template<class T> static
bool parseAPETag(T &file, QImage &img)
{
    if (!file.hasAPETag()) {
        return false;
    }
    const auto &map = file.APETag()->itemListMap();
    for (const auto &item : map) {
        if (item.second.type() != TagLib::APE::Item::Binary) {
            continue;
        }
        const auto coverData = item.second.binaryData();
        const auto data = coverData.data();
        const auto size = coverData.size();
        const char *start = nullptr;
        for (size_t i=0; i<size; i++) {
            if (data[i] == '\0' && (i+1) < size) {
                start = data+i+1;
                break;
            }
        }
        if (!start) {
            return false;
        }
        QByteArray cdata(start, coverData.size()-(start-data));
        img.loadFromData(cdata);
        return true;
    }
    return false;
}

bool ATCreator::create(const QString &path, int, int, QImage &img)
{
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(path);
    if (!type.isValid()) {
        return false;
    }

    if (type.inherits("audio/mpeg")) {
        TagLib::MPEG::File file(QFile::encodeName(path));
        return parseID3v2Tag(file, img) || parseAPETag(file, img);
    }
    if (type.inherits("audio/x-flac") || type.inherits("audio/flac")) {
        TagLib::FLAC::File file(QFile::encodeName(path));
        return parseFlacTag(file, img) || parseID3v2Tag(file, img);
    }
    if (type.inherits("audio/mp4") || type.inherits("audio/x-m4a")) {
        TagLib::MP4::File file(QFile::encodeName(path));
        return parseMP4Tag(file, img);
    }
    if (type.inherits("audio/x-ape")) {
        TagLib::APE::File file(QFile::encodeName(path));
        return parseAPETag(file, img);
    }
    if (type.inherits("audio/x-wavpack") || type.inherits("audio/x-vw")) {
        TagLib::WavPack::File file(QFile::encodeName(path));
        return parseAPETag(file, img);
    }
    if (type.inherits("audio/x-musepack")) {
        TagLib::MPC::File file(QFile::encodeName(path));
        return parseAPETag(file, img);
    }
    if (type.inherits("audio/ogg") || type.inherits("audio/vorbis")) {
        TagLib::FileRef fileRef(QFile::encodeName(path));
        if (fileRef.isNull()) {
            return false;
        }
        auto xiphComment = dynamic_cast<TagLib::Ogg::XiphComment*>(fileRef.tag());
        if (!xiphComment || xiphComment->isEmpty()) {
            return false;
        }
        return parseFlacTag(*xiphComment, img);
    }
    if (type.inherits("audio/x-aiff")) {
        TagLib::RIFF::AIFF::FileExt file(QFile::encodeName(path));
        return parseID3v2Tag(file, img);
    }
    if (type.inherits("audio/x-wav")) {
        TagLib::RIFF::WAV::File file(QFile::encodeName(path));
        return parseID3v2Tag(file, img);
    }
    return false;
}

ThumbCreator::Flags ATCreator::flags() const
{
    return (Flags)(DrawFrame | BlendIcon);
}
