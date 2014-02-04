/*
 * Copyright © 2009-2012 Siyan Panayotov <xsisqox@gmail.com>
 *
 * Based on code by (see README for details):
 * - Björn Lindqvist <bjourne@gmail.com>
 *
 * This file is part of Viewnior.
 *
 * Viewnior is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Viewnior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Viewnior.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <exiv2/exiv2.hpp>
#include <iostream>

#include "uni-exiv2.hpp"

static Exiv2::Image::AutoPtr cached_image;

extern "C"
void 
uni_exif_dictionary_map(void (*callback)(const char*, const char*, void*), void *user_data)
{
    uint i;
    for( i=0; i<sizeof(exifDataDictionary)/sizeof(exifDataDictionary[0]); i++ ) {
        callback(exifDataDictionary[i].key, exifDataDictionary[i].label, user_data);
    }
}

extern "C" 
void 
uni_read_exiv2_map(const char *uri, void (*callback)(const char*, const char*, void*), void *user_data)
{
    Exiv2::LogMsg::setLevel(Exiv2::LogMsg::mute);
    try {
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(uri);
        if ( image.get() == 0 ) {
            return;
        }

        image->readMetadata();
        Exiv2::ExifData &exifData = image->exifData();

        if ( exifData.empty() ) {
            return;
        }

        Exiv2::ExifData::const_iterator pos;
        uint i;
        ExifDataDictionary dict;
        for( i=0; i<sizeof(exifDataDictionary)/sizeof(exifDataDictionary[0]); i++ ) {
            dict = exifDataDictionary[i];
            if ( dict.finder == NULL ) {
                Exiv2::ExifKey key(dict.key);
                pos = exifData.findKey(key);

                if ( pos == exifData.end() ) {
                    callback(dict.label, NULL, user_data);
                } else {
                    callback(dict.label, pos->print(&exifData).c_str(), user_data);
                }
            } else {
                pos = dict.finder(exifData);

                if ( pos == exifData.end() ) {
                    callback(dict.label, NULL, user_data);
                } else {
                    callback(dict.label, pos->print(&exifData).c_str(), user_data);
                }
            }
        }
    } catch (Exiv2::AnyError& e) {
        std::cerr << "Exiv2: '" << e << "'\n";
    }
}

extern "C"
int
uni_read_exiv2_to_cache(const char *uri)
{
    Exiv2::LogMsg::setLevel(Exiv2::LogMsg::mute);

    if ( cached_image.get() != NULL ) {
        cached_image->clearMetadata();
        cached_image.reset(NULL);
    }

    try {
        cached_image = Exiv2::ImageFactory::open(uri);
        if ( cached_image.get() == 0 ) {
            return 1;
        }

        cached_image->readMetadata();
    } catch (Exiv2::AnyError& e) {
        std::cerr << "Exiv2: '" << e << "'\n";
    }

    return 0;
}

extern "C"
int 
uni_write_exiv2_from_cache(const char *uri)
{
    Exiv2::LogMsg::setLevel(Exiv2::LogMsg::mute);
    
    if ( cached_image.get() == NULL ) {
        return 1;
    }

    try {
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(uri);
        if ( image.get() == 0 ) {
            return 2;
        }

        image->setMetadata( *cached_image );
        image->writeMetadata();
        
        cached_image->clearMetadata();
        cached_image.reset(NULL);

        return 0;
    } catch (Exiv2::AnyError& e) {
        std::cerr << "Exiv2: '" << e << "'\n";
    }

    return 0;
}