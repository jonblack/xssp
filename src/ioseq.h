// Copyright Maarten L. Hekkelman, Radboud University 2008-2011.
// Copyright Coos Baakman, Jon Black, Wouter G. Touw & Gert Vriend, Radboud university medical center 2015.
//   Distributed under the Boost Software License, Version 1.0.
//       (See accompanying file LICENSE_1_0.txt or copy at
//             http://www.boost.org/LICENSE_1_0.txt)
//
// i/o code for mas

#ifndef XSSP_IOSEQ_H
#define XSSP_IOSEQ_H

#pragma once

#include <boost/filesystem/path.hpp>

#include <string>
#include <vector>

struct entry;

void readFasta(std::istream& is, std::vector<entry>& seq);
void readPDB(std::istream& is, char chainID, std::vector<entry>& seq);
void readAlignmentFromHsspFile(std::istream& is,
  char& chainID, std::vector<entry>& seq);
void readWhatifMappingFile(std::istream& is,
  std::vector<entry>& seq);
void readFamilyIdsFile(std::istream& is, const boost::filesystem::path& dir,
  std::vector<entry>& seq);
//void readSecStruct(std::vector<entry>& seq);

void report(const std::vector<entry*>& alignment,
  std::ostream& os, const std::string& format);

#endif
