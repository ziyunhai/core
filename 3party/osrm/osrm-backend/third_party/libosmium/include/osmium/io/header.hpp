#ifndef OSMIUM_IO_HEADER_HPP
#define OSMIUM_IO_HEADER_HPP

/*

This file is part of Osmium (http://osmcode.org/libosmium).

Copyright 2013-2015 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <initializer_list>
#include <vector>

#include <osmium/osm/box.hpp>
#include <osmium/util/options.hpp>

namespace osmium {

    namespace io {

        /**
        * Meta information from the header of an OSM file.
        */
        class Header : public osmium::util::Options {

            /// Bounding boxes
            std::vector<osmium::Box> m_boxes;

            /**
            * Are there possibly multiple versions of the same object in this stream of objects?
            * This is true for history files and for change files, but not for normal OSM files.
            */
            bool m_has_multiple_object_versions = false;

        public:

            Header() = default;

            explicit Header(const std::initializer_list<osmium::util::Options::value_type>& values) :
                Options(values) {
            }

            Header(const Header&) = default;
            Header& operator=(const Header&) = default;

            Header(Header&&) = default;
            Header& operator=(Header&&) = default;

            ~Header() = default;

            std::vector<osmium::Box>& boxes() noexcept {
                return m_boxes;
            }

            const std::vector<osmium::Box>& boxes() const noexcept {
                return m_boxes;
            }

            Header& boxes(const std::vector<osmium::Box>& boxes) noexcept {
                m_boxes = boxes;
                return *this;
            }

            osmium::Box box() const {
                return m_boxes.empty() ? osmium::Box() : m_boxes.front();
            }

            osmium::Box joined_boxes() const {
                osmium::Box box;
                for (const auto& b : m_boxes) {
                    box.extend(b.bottom_left());
                    box.extend(b.top_right());
                }
                return box;
            }

            Header& add_box(const osmium::Box& box) {
                m_boxes.push_back(box);
                return *this;
            }

            bool has_multiple_object_versions() const noexcept {
                return m_has_multiple_object_versions;
            }

            Header& set_has_multiple_object_versions(bool value) noexcept {
                m_has_multiple_object_versions = value;
                return *this;
            }

        }; // class Header

    } // namespace io

} // namespace osmium

#endif // OSMIUM_IO_HEADER_HPP
