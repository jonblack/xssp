// Copyright Maarten L. Hekkelman, Radboud University 2008-2011.
// Copyright Coos Baakman, Jon Black, Wouter G. Touw & Gert Vriend, Radboud university medical center 2015.
//   Distributed under the Boost Software License, Version 1.0.
//       (See accompanying file LICENSE_1_0.txt or copy at
//             http://www.boost.org/LICENSE_1_0.txt)
//
// align-3d

#ifndef XSSP_ALIGN3D_H
#define XSSP_ALIGN3D_H

#pragma once

class substitution_matrix_family;

void align_structures(
  std::istream& structureA, std::istream& structureB,
  char chainA, char chainB,
  uint32 iterations,
  substitution_matrix_family& mat, float gop, float gep, float magic,
  std::vector<entry*>& outAlignment);

#endif
