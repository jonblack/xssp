#include "mas.h"

#include <iostream>
#include <set>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/program_options.hpp>
#include <boost/program_options/config.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/date_clock_device.hpp>
#include <boost/regex.hpp>

#include "utils.h"
#include "structure.h"
#include "dssp.h"
#include "matrix.h"
#include "buffer.h"

using namespace std;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;
namespace po = boost::program_options;
namespace io = boost::iostreams;

int VERBOSE = 0;

// --------------------------------------------------------------------

namespace HSSP
{

const float kThreshold = 0.05f;

// precalculated threshold table for identity values between 10 and 80
const double kHomologyThreshold[] = {
	0.795468, 0.75398, 0.717997, 0.686414, 0.658413, 0.633373, 0.610811,
	0.590351, 0.571688, 0.554579, 0.53882, 0.524246, 0.510718, 0.498117,
	0.486344, 0.475314, 0.464951, 0.455194, 0.445984, 0.437275, 0.429023,
	0.421189, 0.413741, 0.406647, 0.399882, 0.39342, 0.38724, 0.381323,
	0.375651, 0.370207, 0.364976, 0.359947, 0.355105, 0.35044, 0.345941,
	0.341599, 0.337406, 0.333352, 0.329431, 0.325636, 0.32196, 0.318396,
	0.314941, 0.311587, 0.308331, 0.305168, 0.302093, 0.299103, 0.296194,
	0.293362, 0.290604, 0.287917, 0.285298, 0.282744, 0.280252, 0.277821,
	0.275448, 0.273129, 0.270865, 0.268652, 0.266488, 0.264372, 0.262302,
	0.260277, 0.258294, 0.256353, 0.254452, 0.252589, 0.250764, 0.248975,
	0.247221
};

bool drop(float score, uint32 length, float threshold)
{
	uint32 ix = max(10U, min(length, 80U)) - 10;
	return score < kHomologyThreshold[ix] + threshold;
}

// --------------------------------------------------------------------

const int8 kM6Pam250[] = {
	  2,                                                                                                               // A
	  0,   3,                                                                                                          // B
	 -2,  -4,  12,                                                                                                     // C
	  0,   3,  -5,   4,                                                                                                // D
	  0,   3,  -5,   3,   4,                                                                                           // E
	 -3,  -4,  -4,  -6,  -5,   9,                                                                                      // F
	  1,   0,  -3,   1,   0,  -5,   5,                                                                                 // G
	 -1,   1,  -3,   1,   1,  -2,  -2,   6,                                                                            // H
	 -1,  -2,  -2,  -2,  -2,   1,  -3,  -2,   5,                                                                       // I
	 -1,   1,  -5,   0,   0,  -5,  -2,   0,  -2,   5,                                                                  // K
	 -2,  -3,  -6,  -4,  -3,   2,  -4,  -2,   2,  -3,   6,                                                             // L
	 -1,  -2,  -5,  -3,  -2,   0,  -3,  -2,   2,   0,   4,   6,                                                        // M
	  0,   2,  -4,   2,   1,  -3,   0,   2,  -2,   1,  -3,  -2,   2,                                                   // N
	  1,  -1,  -3,  -1,  -1,  -5,   0,   0,  -2,  -1,  -3,  -2,   0,   6,                                              // P
	  0,   1,  -5,   2,   2,  -5,  -1,   3,  -2,   1,  -2,  -1,   1,   0,   4,                                         // Q
	 -2,  -1,  -4,  -1,  -1,  -4,  -3,   2,  -2,   3,  -3,   0,   0,   0,   1,   6,                                    // R
	  1,   0,   0,   0,   0,  -3,   1,  -1,  -1,   0,  -3,  -2,   1,   1,  -1,   0,   2,                               // S
	  1,   0,  -2,   0,   0,  -3,   0,  -1,   0,   0,  -2,  -1,   0,   0,  -1,  -1,   1,   3,                          // T
	  0,  -2,  -2,  -2,  -2,  -1,  -1,  -2,   4,  -2,   2,   2,  -2,  -1,  -2,  -2,  -1,   0,   4,                     // V
	 -6,  -5,  -8,  -7,  -7,   0,  -7,  -3,  -5,  -3,  -2,  -4,  -4,  -6,  -5,   2,  -2,  -5,  -6,  17,                // W
	 -3,  -3,   0,  -4,  -4,   7,  -5,   0,  -1,  -4,  -1,  -2,  -2,  -5,  -4,  -4,  -3,  -3,  -2,   0,  10,           // Y
	  0,   2,  -5,   3,   3,  -5,   0,   2,  -2,   0,  -3,  -2,   1,   0,   3,   0,   0,  -1,  -2,  -6,  -4,   3,      // Z
	  0,  -1,  -3,  -1,  -1,  -2,  -1,  -1,  -1,  -1,  -1,  -1,   0,  -1,  -1,  -1,   0,   0,  -1,  -4,  -2,  -1,  -1, // x
};

// Dayhoff matrix as used by maxhom
const float kDayhoffData[] =
{
	 1.5f,																																 // A
	 0.0f, 0.0f,                                                                                                                         // B
	 0.3f, 0.0f, 1.5f,                                                                                                                   // C
	 0.3f, 0.0f,-0.5f, 1.5f,                                                                                                             // D
	 0.3f, 0.0f,-0.6f, 1.0f, 1.5f,                                                                                                       // E
	-0.5f, 0.0f,-0.1f,-1.0f,-0.7f, 1.5f,                                                                                                 // F
	 0.7f, 0.0f, 0.2f, 0.7f, 0.5f,-0.6f, 1.5f,                                                                                           // G
	-0.1f, 0.0f,-0.1f, 0.4f, 0.4f,-0.1f,-0.2f, 1.5f,                                                                                     // H
	 0.0f, 0.0f, 0.2f,-0.2f,-0.2f, 0.7f,-0.3f,-0.3f, 1.5f,                                                                               // I
	 0.0f, 0.0f,-0.6f, 0.3f, 0.3f,-0.7f,-0.1f, 0.1f,-0.2f, 1.5f,                                                                         // K
	-0.1f, 0.0f,-0.8f,-0.5f,-0.3f, 1.2f,-0.5f,-0.2f, 0.8f,-0.3f, 1.5f,                                                                   // L
	 0.0f, 0.0f,-0.6f,-0.4f,-0.2f, 0.5f,-0.3f,-0.3f, 0.6f, 0.2f, 1.3f, 1.5f,                                                             // M
	 0.2f, 0.0f,-0.3f, 0.7f, 0.5f,-0.5f, 0.4f, 0.5f,-0.3f, 0.4f,-0.4f,-0.3f, 1.5f,                                                       // N
	 0.5f, 0.0f, 0.1f, 0.1f, 0.1f,-0.7f, 0.3f, 0.2f,-0.2f, 0.1f,-0.3f,-0.2f, 0.0f, 1.5f,                                                 // P
	 0.2f, 0.0f,-0.6f, 0.7f, 0.7f,-0.8f, 0.2f, 0.7f,-0.3f, 0.4f,-0.1f, 0.0f, 0.4f, 0.3f, 1.5f,                                           // Q
	-0.3f, 0.0f,-0.3f, 0.0f, 0.0f,-0.5f,-0.3f, 0.5f,-0.3f, 0.8f,-0.4f, 0.2f, 0.1f, 0.3f, 0.4f, 1.5f,                                     // R
	 0.4f, 0.0f, 0.7f, 0.2f, 0.2f,-0.3f, 0.6f,-0.2f,-0.1f, 0.2f,-0.4f,-0.3f, 0.3f, 0.4f,-0.1f, 0.1f, 1.5f,                               // S
	 0.4f, 0.0f, 0.2f, 0.2f, 0.2f,-0.3f, 0.4f,-0.1f, 0.2f, 0.2f,-0.1f, 0.0f, 0.2f, 0.3f,-0.1f,-0.1f, 0.3f, 1.5f,                         // T
	 0.2f, 0.0f, 0.2f,-0.2f,-0.2f, 0.2f, 0.2f,-0.3f, 1.1f,-0.2f, 0.8f, 0.6f,-0.3f, 0.1f,-0.2f,-0.3f,-0.1f, 0.2f, 1.5f,                   // V
	-0.8f, 0.0f,-1.2f,-1.1f,-1.1f, 1.3f,-1.0f,-0.1f,-0.5f, 0.1f, 0.5f,-0.3f,-0.3f,-0.8f,-0.5f, 1.4f, 0.3f,-0.6f,-0.8f, 1.5f,             // W
	-0.3f, 0.0f, 1.0f,-0.5f,-0.5f, 1.4f,-0.7f, 0.3f, 0.1f,-0.6f, 0.3f,-0.1f,-0.1f,-0.8f,-0.6f,-0.6f,-0.4f,-0.3f,-0.1f, 1.1f, 1.5f,       // Y
	 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f  // Z
                                                                                                                                         // x
//     1.5f,                                                                                                                  // V
//     0.8f, 1.5f,                                                                                                            // L
//     1.1f, 0.8f, 1.5f,                                                                                                      // I
//     0.6f, 1.3f, 0.6f, 1.5f,                                                                                                // M
//     0.2f, 1.2f, 0.7f, 0.5f, 1.5f,                                                                                          // F
//    -0.8f, 0.5f,-0.5f,-0.3f, 1.3f, 1.5f,                                                                                    // W
//    -0.1f, 0.3f, 0.1f,-0.1f, 1.4f, 1.1f, 1.5f,                                                                              // Y
//     0.2f,-0.5f,-0.3f,-0.3f,-0.6f,-1.0f,-0.7f, 1.5f,                                                                        // G
//     0.2f,-0.1f, 0.0f, 0.0f,-0.5f,-0.8f,-0.3f, 0.7f, 1.5f,                                                                  // A
//     0.1f,-0.3f,-0.2f,-0.2f,-0.7f,-0.8f,-0.8f, 0.3f, 0.5f, 1.5f,                                                            // P
//    -0.1f,-0.4f,-0.1f,-0.3f,-0.3f, 0.3f,-0.4f, 0.6f, 0.4f, 0.4f, 1.5f,                                                      // S
//     0.2f,-0.1f, 0.2f, 0.0f,-0.3f,-0.6f,-0.3f, 0.4f, 0.4f, 0.3f, 0.3f, 1.5f,                                                // T
//     0.2f,-0.8f, 0.2f,-0.6f,-0.1f,-1.2f, 1.0f, 0.2f, 0.3f, 0.1f, 0.7f, 0.2f, 1.5f,                                          // C
//    -0.3f,-0.2f,-0.3f,-0.3f,-0.1f,-0.1f, 0.3f,-0.2f,-0.1f, 0.2f,-0.2f,-0.1f,-0.1f, 1.5f,                                    // H
//    -0.3f,-0.4f,-0.3f, 0.2f,-0.5f, 1.4f,-0.6f,-0.3f,-0.3f, 0.3f, 0.1f,-0.1f,-0.3f, 0.5f, 1.5f,                              // R
//    -0.2f,-0.3f,-0.2f, 0.2f,-0.7f, 0.1f,-0.6f,-0.1f, 0.0f, 0.1f, 0.2f, 0.2f,-0.6f, 0.1f, 0.8f, 1.5f,                        // K
//    -0.2f,-0.1f,-0.3f, 0.0f,-0.8f,-0.5f,-0.6f, 0.2f, 0.2f, 0.3f,-0.1f,-0.1f,-0.6f, 0.7f, 0.4f, 0.4f, 1.5f,                  // Q
//    -0.2f,-0.3f,-0.2f,-0.2f,-0.7f,-1.1f,-0.5f, 0.5f, 0.3f, 0.1f, 0.2f, 0.2f,-0.6f, 0.4f, 0.0f, 0.3f, 0.7f, 1.5f,            // E
//    -0.3f,-0.4f,-0.3f,-0.3f,-0.5f,-0.3f,-0.1f, 0.4f, 0.2f, 0.0f, 0.3f, 0.2f,-0.3f, 0.5f, 0.1f, 0.4f, 0.4f, 0.5f, 1.5f,      // N
//    -0.2f,-0.5f,-0.2f,-0.4f,-1.0f,-1.1f,-0.5f, 0.7f, 0.3f, 0.1f, 0.2f, 0.2f,-0.5f, 0.4f, 0.0f, 0.3f, 0.7f, 1.0f, 0.7f, 1.5f // D
};

// 22 real letters and 1 dummy
const char kResidues[] = "ABCDEFGHIKLMNPQRSTVWYZX";

inline uint8 ResidueNr(char inAA)
{
	int result = -1;

	const static uint8 kResidueNrTable[] = {
	//	A   B   C   D   E   F   G   H   I       K   L   M   N       P   Q   R   S   T  U=X  V   W   X   Y   Z
		0,  1,  2,  3,  4,  5,  6,  7,  8, 23,  9, 10, 11, 12, 23, 13, 14, 15, 16, 17, 22, 18, 19, 22, 20, 21
	};
	
	inAA |= 040;
	if (inAA >= 'a' and inAA <= 'z')
		result = kResidueNrTable[inAA - 'a'];
	return result;
}

template<typename T>
inline T score(const T inMatrix[], uint8 inAA1, uint8 inAA2)
{
	T result;

	if (inAA1 >= inAA2)
		result = inMatrix[(inAA1 * (inAA1 + 1)) / 2 + inAA2];
	else
		result = inMatrix[(inAA2 * (inAA2 + 1)) / 2 + inAA1];

	return result;	
}

sequence encode(const string& s)
{
	sequence result(s.length(), 0);
	transform(s.begin(), s.end(), result.begin(), [](char aa) -> uint8 { return aa == '-' ? '-' : ResidueNr(aa); });
	return result;
}

string decode(const sequence& s)
{
	string result(s.length(), 0);
	transform(s.begin(), s.end(), result.begin(), [](uint8 r) -> char { return r == '-' ? '-' : kResidues[r]; });
	return result;
}

// --------------------------------------------------------------------

float calculateDistance(const sequence& a, const sequence& b)
{
	const float kDistanceGapOpen = 10;
	const float kDistanceGapExtend = 0.2f;

	int32 x = 0, dimX = static_cast<int32>(a.length());
	int32 y = 0, dimY = static_cast<int32>(b.length());

	matrix<float>	B(dimX, dimY);
	matrix<float>	Ix(dimX, dimY);
	matrix<float>	Iy(dimX, dimY);
	matrix<uint16>	id(dimX, dimY);
	
	Ix(0, 0) = 0;
	Iy(0, 0) = 0;
	
	uint16 highId = 0;
	
	Ix(x, y) = 0;
	Iy(x, y) = 0;
	if (x > 0 and y > 0)
		id(x - 1, y - 1) = highId;

	int32 startX = x, startY = y;
	float high = -numeric_limits<float>::max();
	uint16 highIdSub = 0;

	for (x = startX; x < dimX; ++x)
	{
		for (y = startY; y < dimY; ++y)
		{
			float Ix1 = 0; if (x > startX) Ix1 = Ix(x - 1, y);
			float Iy1 = 0; if (y > startY) Iy1 = Iy(x, y - 1);

			// (1)
			float M = score(kM6Pam250, a[x], b[y]);
			if (x > startX and y > startY)
				M += B(x - 1, y - 1);

			float s;
			uint16 i = 0;
			if (a[x] == b[y])
				i = 1;

			if (M >= Ix1 and M >= Iy1)
			{
				if (x > startX and y > startY)
					i += id(x - 1, y - 1);
				s = M;
			}
			else if (Ix1 >= Iy1)
			{
				if (x > startX)
					i += id(x - 1, y);
				s = Ix1;
			}
			else
			{
				if (y > startY)
					i += id(x, y - 1);
				s = Iy1;
			}
			
			B(x, y) = s;
			id(x, y) = i;
			
			if ((x == dimX - 1 or y == dimY - 1) and high < s)
			{
				high = s;
				highIdSub = i;
			}

			// (3)
			Ix(x, y) = max(M - kDistanceGapOpen, Ix1 - kDistanceGapExtend);
			
			// (4)
			Iy(x, y) = max(M - kDistanceGapOpen, Iy1 - kDistanceGapExtend);
		}
	}

	highId += highIdSub;
	
	float result = 1.0f - float(highId) / max(dimX, dimY);

	assert(result >= 0.0f);
	assert(result <= 1.0f);
	
	return result;
}

// --------------------------------------------------------------------

struct MHit
{
	MHit(const MHit& e);
	
	MHit(const string& id, const string& def, const sequence& seq)
		: m_id(id), m_def(def), m_seq(seq), m_distance(0) {}

	struct insertion
	{
		uint32			m_pos;
		sequence		m_seq;
	};
	
	static MHit*		Create(const string& id, const string& def,
							const string& seq, const sequence& chain);

	void				Update(const matrix<int8>& inTraceBack, const sequence& inChain,
							int32 inX, int32 inY, const matrix<float>& inB,const int8 inMatrix[]);

	string				m_id, m_acc, m_def;
	sequence			m_seq;
	string				m_aligned;
	float				m_distance, m_score;
	int32				m_ifir, m_ilas, m_jfir, m_jlas;
	uint32				m_identical, m_similar, m_length;
	uint32				m_gaps, m_gapn;
	vector<insertion>	m_insertions;
};

MHit* MHit::Create(const string& id, const string& def, const string& seq, const sequence& chain)
{
	MHit* result = new MHit(id, def, encode(seq));

	static const boost::regex
		kM6FastARE("^(\\w+)((?:\\|([^| ]*))?(?:\\|([^| ]+))?(?:\\|([^| ]+))?(?:\\|([^| ]+))?)");
	
	boost::smatch m;
	if (boost::regex_match(result->m_id, m, kM6FastARE, boost::match_not_dot_newline))
	{
		if (m[1] == "sp" or m[1] == "tr")
		{
			result->m_acc = m[3];
			result->m_id = m[4];
		}
		else
			result->m_id = m[2];
	}
	
	result->m_aligned = string(chain.length(), ' ');
	result->m_distance = calculateDistance(result->m_seq, chain);
	return result;
}

void MHit::Update(const matrix<int8>& inTraceBack, const sequence& inChain,
	int32 inX, int32 inY, const matrix<float>& inB, const int8 inMatrix[])
{
	m_ilas = inX + 1;
	m_jlas = inY + 1;

	m_identical = m_similar = m_length = m_gaps = m_gapn = 0;

	int32 x = inX;
	int32 y = inY;
	
	bool gap = false;
	
	// trace back the matrix
	while (x >= 0 and y >= 0 and inB(x, y) > 0)
	{
		++m_length;
		switch (inTraceBack(x, y))
		{
			case -1:
				if (not gap)
					++m_gaps;
				++m_gapn;
				gap = true;
				--y;
				break;

			case 1:
				m_aligned[x] = '-';
				--x;
				break;

			case 0:
				gap = false;
				m_aligned[x] = kResidues[m_seq[y]];
				if (inChain[x] == m_seq[y])
					++m_identical, ++m_similar;
				else if (score(inMatrix, inChain[x], m_seq[y]) > 0)
					++m_similar;
				--x;
				--y;
				break;
		}
	}
	
	assert(gap == false);
	m_ifir = x + 2;
	m_jfir = y + 2;
	
	m_score = float(m_identical) / m_length;
}

// --------------------------------------------------------------------

struct MResInfo
{
	uint8			m_letter;
	uint8			m_chain_id;
	uint32			m_seq_nr;
	uint32			m_pdb_nr;
	string			m_dssp;
	float			m_consweight;
	uint32			m_nocc;
	uint32			m_dist[23];
	uint32			m_ins, m_del;
	float			m_score[23];

	void			Add(uint8 r, const int8 inMatrix[]);
	void			CalculateConservation();
};

typedef vector<MResInfo> MResInfoList;

void MResInfo::Add(uint8 r, const int8 inMatrix[])
{
	assert(r < 23);
	m_nocc += 1;
	m_dist[r] += 1;
	
	for (int i = 0; i < 23; ++i)
	{
		m_score[i] = 0;
		
		for (int j = 0; j < 23; ++j)
			m_score[i] += float(score(inMatrix, i, j)) * m_dist[j];
		
		m_score[i] /= m_nocc;
	}
}

void MResInfo::CalculateConservation()
{
	
}

// --------------------------------------------------------------------

struct MProfile
{
					MProfile(const MChain& inChain, const sequence& inSequence);

	void			Process(istream& inHits, progress& inProgress);
	void			Align(MHit* e);

	void			dump(ostream& os, const matrix<int8>& tb, const sequence& s);

	const MChain&	m_chain;
	sequence		m_seq;
	MResInfoList	m_residues;
	vector<MHit*>	m_entries;
};

MProfile::MProfile(const MChain& inChain, const sequence& inSequence)
	: m_chain(inChain), m_seq(inSequence)
{
	const vector<MResidue*>& residues = m_chain.GetResidues();
	vector<MResidue*>::const_iterator ri = residues.begin();

	for (uint32 i = 0; i < inSequence.length(); ++i)
	{
		assert(ri != residues.end());
		
		if (ri != residues.begin() and (*ri)->GetNumber() > (*(ri - 1))->GetNumber() + 1)
		{
			MResInfo res = { 0, 0, m_residues.size() + 1 };
			m_residues.push_back(res);
		}
		else
		{
			string dssp = ResidueToDSSPLine(**ri).substr(5, 34);
			MResInfo res = { inSequence[i], m_chain.GetChainID(), m_residues.size() + 1, (*ri)->GetNumber(), dssp };
			res.Add(res.m_letter, kM6Pam250);
			m_residues.push_back(res);
		}

		++ri;
	}
}

void MProfile::dump(ostream& os, const matrix<int8>& tb, const sequence& s)
{
	os << ' ';
	foreach (auto& e, m_residues)
		os << kResidues[e.m_letter];
	os << endl;

	for (uint32 y = 0; y < s.length(); ++y)
	{
		os << kResidues[s[y]];
		for (uint32 x = 0; x < m_residues.size(); ++x)
		{
			switch (tb(x, y))
			{
				case -1:	os << '|'; break;
				case 0:		os << '\\'; break;
				case 1:		os << '-'; break;
				case 2:		os << '.'; break;
			}
		}
		os << endl;
	}
}

void MProfile::Align(MHit* e)
{
	int32 x = 0, dimX = static_cast<int32>(m_seq.length());
	int32 y = 0, dimY = static_cast<int32>(e->m_seq.length());
	
	matrix<float> B(dimX, dimY);
	matrix<float> Ix(dimX, dimY);
	matrix<float> Iy(dimX, dimY);
	matrix<int8> tb(dimX, dimY);
	
	float minLength = static_cast<float>(dimX), maxLength = static_cast<float>(dimY);
	if (minLength > maxLength)
		swap(minLength, maxLength);
	
	float gop = 10, gep = 0.2f;
	
	float logmin = 1.0f / log10(minLength);
	float logdiff = 1.0f + 0.5f * log10(minLength / maxLength);
	
	// initial gap open cost, 0.05f is the remaining magical number here...
//	gop = (gop / (logdiff * logmin)) * abs(smat.mismatch_average()) * smat.scale_factor() * magic;

	// position specific gap penalties
	// initial gap extend cost is adjusted for difference in sequence lengths
	vector<float> gop_a(dimX, gop), gep_a(dimX, gep /* * (1 + log10(float(dimX) / dimY))*/);
//	adjust_gp(gop_a, gep_a, a);
	
	vector<float> gop_b(dimY, gop), gep_b(dimY, gep /* * (1 + log10(float(dimY) / dimX))*/);
//	adjust_gp(gop_b, gep_b, b);

	int32 highX = 0, highY = 0;
	float highS = 0;
	
	for (x = 0; x < dimX; ++x)
	{
		for (y = 0; y < dimY; ++y)
		{
			float Ix1 = 0; if (x > 0) Ix1 = Ix(x - 1, y);
			float Iy1 = 0; if (y > 0) Iy1 = Iy(x, y - 1);
			
			float M = m_residues[x].m_score[e->m_seq[y]];
			if (x > 0 and y > 0)
				M += B(x - 1, y - 1);

			float s;
			if (M >= Ix1 and M >= Iy1)
			{
				tb(x, y) = 0;
				B(x, y) = s = M;
			}
			else if (Ix1 >= Iy1)
			{
				tb(x, y) = 1;
				B(x, y) = s = Ix1;
			}
			else
			{
				tb(x, y) = -1;
				B(x, y) = s = Iy1;
			}
			
			if (highS < s)
			{
				highS = s;
				highX = x;
				highY = y;
			}
			
			Ix(x, y) = max(M - (x < dimX - 1 ? gop_a[x] : 0), Ix1 - gep_a[x]);
			Iy(x, y) = max(M - (y < dimY - 1 ? gop_b[y] : 0), Iy1 - gep_b[y]);
		}
	}

	// build the alignment
	x = highX;
	y = highY;

//	if (VERBOSE >= 6)
//		dump(cerr, tb, e->m_seq);

	uint32 ident = 0, length = 0;

	// trace back the matrix
	while (x >= 0 and y >= 0 and B(x, y) > 0)
	{
		++length;
		switch (tb(x, y))
		{
			case -1:
				--y;
				break;

			case 1:
				--x;
				break;

			case 0:
				if (e->m_seq[y] == m_seq[x])
					++ident;
				--x;
				--y;
				break;
			
			default:
				assert(false);
				break;
		}
	}
	
	if (drop(float(ident) / length, length, kThreshold))
		delete e;
	else
	{
		e->Update(tb, m_seq, highX, highY, B, kM6Pam250);
		m_entries.push_back(e);
		for (uint32 i = 0; i < m_seq.length(); ++i)
		{
			int8 r = ResidueNr(e->m_aligned[i]);
			if (r < 0 or r >= 23)
				continue;
			m_residues[i].Add(r, kM6Pam250);
		}
	}
}

// --------------------------------------------------------------------

void MProfile::Process(istream& inHits, progress& inProgress)
{
	string id, def, seq;
	for (;;)
	{
		string line;
		getline(inHits, line);
		if (line.empty() and inHits.eof())
			break;

		inProgress.step(line.length() + 1);
		
		if (ba::starts_with(line, ">"))
		{
			if (not (id.empty() or seq.empty()))
				Align(MHit::Create(id, def, seq, m_seq));
			
			id.clear();
			def.clear();
			seq.clear();
			
			string::size_type s = line.find(' ');
			if (s != string::npos)
			{
				id = line.substr(1, s - 1);
				def = line.substr(s + 1);
			}
			else
				id = line.substr(1);
		}
		else
			seq += line;
	}

	if (not (id.empty() or seq.empty()))
		Align(MHit::Create(id, def, seq, m_seq));
}

// --------------------------------------------------------------------

// Find the minimal set of overlapping sequences
// Only search fully contained subsequences, in case of strong similarity
// (distance < 0.01) we take the longest chain.
void ClusterSequences(vector<sequence>& s, vector<uint32>& ix)
{
	for (;;)
	{
		bool found = false;
		for (uint32 i = 0; not found and i < s.size() - 1; ++i)
		{
			for (uint32 j = i + 1; not found and j < s.size(); ++j)
			{
				sequence& a = s[i];
				sequence& b = s[j];

				if (a.empty() or b.empty())
					continue;
				
				if (a == b)
				{
					s[j].clear();
					ix[j] = i;
					found = true;
				}
				else
				{
					float d = calculateDistance(a, b);
					if (d <= 0.01)
					{
						if (b.length() > a.length())
							swap(i, j);

						s[j].clear();
						ix[j] = i;
						found = true;
					}
				}
			}
		}
		
		if (not found)
			break;
	}
}

// --------------------------------------------------------------------
// Write collected information as a HSSP file to the output stream

void CreateHSSPOutput(const string& inProteinID, const string& inProteinDescription,
	float inThreshold, uint32 inSeqLength, uint32 inNChain, uint32 inKChain,
	const string& inUsedChains, const vector<MHit*>& inHits, const vector<MResInfo>& inResInfo, ostream& os)
{
	using namespace boost::gregorian;
	date today = day_clock::local_day();

	// print the header
	os << "HSSP       HOMOLOGY DERIVED SECONDARY STRUCTURE OF PROTEINS , VERSION 2.0 2011" << endl
	   << "PDBID      " << inProteinID << endl
	   << "DATE       file generated on " << to_iso_extended_string(today) << endl
//	   << "SEQBASE    " << inDatabank->GetName() << " version " << inDatabank->GetVersion() << endl
	   << "THRESHOLD  according to: t(L)=(290.15 * L ** -0.562) + " << (inThreshold * 100) << endl
	   << "REFERENCE  Sander C., Schneider R. : Database of homology-derived protein structures. Proteins, 9:56-68 (1991)." << endl
	   << "CONTACT    Maintained at http://www.cmbi.ru.nl/ by Maarten L. Hekkelman <m.hekkelman@cmbi.ru.nl>" << endl
	   << inProteinDescription
	   << boost::format("SEQLENGTH %5.5d") % inSeqLength << endl
	   << boost::format("NCHAIN     %4.4d chain(s) in %s data set") % inNChain % inProteinID << endl;
	
	if (inKChain != inNChain)
		os << boost::format("KCHAIN     %4.4d chain(s) used here ; chains(s) : ") % inKChain << inUsedChains << endl;
	
	os << boost::format("NALIGN     %4.4d") % inHits.size() << endl
	   << "NOTATION : ID: EMBL/SWISSPROT identifier of the aligned (homologous) protein" << endl
	   << "NOTATION : STRID: if the 3-D structure of the aligned protein is known, then STRID is the Protein Data Bank identifier as taken" << endl
	   << "NOTATION : from the database reference or DR-line of the EMBL/SWISSPROT entry" << endl
	   << "NOTATION : %IDE: percentage of residue identity of the alignment" << endl
	   << "NOTATION : %SIM (%WSIM):  (weighted) similarity of the alignment" << endl
	   << "NOTATION : IFIR/ILAS: first and last residue of the alignment in the test sequence" << endl
	   << "NOTATION : JFIR/JLAS: first and last residue of the alignment in the alignend protein" << endl
	   << "NOTATION : LALI: length of the alignment excluding insertions and deletions" << endl
	   << "NOTATION : NGAP: number of insertions and deletions in the alignment" << endl
	   << "NOTATION : LGAP: total length of all insertions and deletions" << endl
	   << "NOTATION : LSEQ2: length of the entire sequence of the aligned protein" << endl
	   << "NOTATION : ACCNUM: SwissProt accession number" << endl
	   << "NOTATION : PROTEIN: one-line description of aligned protein" << endl
	   << "NOTATION : SeqNo,PDBNo,AA,STRUCTURE,BP1,BP2,ACC: sequential and PDB residue numbers, amino acid (lower case = Cys), secondary" << endl
	   << "NOTATION : structure, bridge partners, solvent exposure as in DSSP (Kabsch and Sander, Biopolymers 22, 2577-2637(1983)" << endl
	   << "NOTATION : VAR: sequence variability on a scale of 0-100 as derived from the NALIGN alignments" << endl
	   << "NOTATION : pair of lower case characters (AvaK) in the alignend sequence bracket a point of insertion in this sequence" << endl
	   << "NOTATION : dots (....) in the alignend sequence indicate points of deletion in this sequence" << endl
	   << "NOTATION : SEQUENCE PROFILE: relative frequency of an amino acid type at each position. Asx and Glx are in their" << endl
	   << "NOTATION : acid/amide form in proportion to their database frequencies" << endl
	   << "NOTATION : NOCC: number of aligned sequences spanning this position (including the test sequence)" << endl
	   << "NOTATION : NDEL: number of sequences with a deletion in the test protein at this position" << endl
	   << "NOTATION : NINS: number of sequences with an insertion in the test protein at this position" << endl
	   << "NOTATION : ENTROPY: entropy measure of sequence variability at this position" << endl
	   << "NOTATION : RELENT: relative entropy, i.e.  entropy normalized to the range 0-100" << endl
	   << "NOTATION : WEIGHT: conservation weight" << endl
	   << endl
	   << "## PROTEINS : identifier and alignment statistics" << endl
	   << "  NR.    ID         STRID   %IDE %WSIM IFIR ILAS JFIR JLAS LALI NGAP LGAP LSEQ2 ACCNUM     PROTEIN" << endl;
	   
	// print the first list
	uint32 nr = 1;
	boost::format fmt1("%5.5d : %12.12s%4.4s    %4.2f  %4.2f%5.5d%5.5d%5.5d%5.5d%5.5d%5.5d%5.5d%5.5d  %10.10s %s");
	foreach (MHit* h, inHits)
	{
		string id = h->m_id, acc = h->m_acc, pdb;

		if (id.length() > 12)
			id.erase(12, string::npos);
		else if (id.length() < 12)
			id.append(12 - id.length(), ' ');
		
		if (acc.length() > 10)
			acc.erase(10, string::npos);
		else if (acc.length() < 10)
			acc.append(10 - acc.length(), ' ');
		
		float sim = float(h->m_similar) / h->m_length;
		
		os << fmt1 % nr
				   % id % pdb
				   % h->m_score % sim % h->m_ifir % h->m_ilas % h->m_jfir % h->m_jlas % h->m_length
				   % h->m_gaps % h->m_gapn % h->m_seq.length()
				   % acc % h->m_def
		   << endl;
		
		++nr;
	}

	// print the alignments
	for (uint32 i = 0; i < inHits.size(); i += 70)
	{
		uint32 n = i + 70;
		if (n > inHits.size())
			n = inHits.size();
		
		uint32 k[7] = {
			((i +  0) / 10 + 1) % 10,
			((i + 10) / 10 + 1) % 10,
			((i + 20) / 10 + 1) % 10,
			((i + 30) / 10 + 1) % 10,
			((i + 40) / 10 + 1) % 10,
			((i + 50) / 10 + 1) % 10,
			((i + 60) / 10 + 1) % 10
		};
		
		os << boost::format("## ALIGNMENTS %4.4d - %4.4d") % (i + 1) % n << endl
		   << boost::format(" SeqNo  PDBNo AA STRUCTURE BP1 BP2  ACC NOCC  VAR  ....:....%1.1d....:....%1.1d....:....%1.1d....:....%1.1d....:....%1.1d....:....%1.1d....:....%1.1d")
		   					% k[0] % k[1] % k[2] % k[3] % k[4] % k[5] % k[6] << endl;

//		res_ptr last;
		uint32 x = 0;
		foreach (auto& ri, inResInfo)
		{
			if (ri.m_chain_id == 0)
				os << boost::format(" %5.5d        !  !           0   0    0    0    0") % ri.m_seq_nr << endl;
			else
			{
				string aln;
				
				foreach (MHit* hit, boost::make_iterator_range(inHits.begin() + i, inHits.begin() + n))
					aln += hit->m_aligned[x];
				
				uint32 ivar = uint32(100 * (1 - ri.m_consweight));

				os << ' ' << boost::format("%5.5d%s%4.4d %4.4d  ")
					% ri.m_seq_nr % ri.m_dssp % ri.m_nocc % ivar << aln << endl;
			}
			++x;
		}
	}
	
//	// ## SEQUENCE PROFILE AND ENTROPY
//	os << "## SEQUENCE PROFILE AND ENTROPY" << endl
//	   << " SeqNo PDBNo   V   L   I   M   F   W   Y   G   A   P   S   T   C   H   R   K   Q   E   N   D  NOCC NDEL NINS ENTROPY RELENT WEIGHT" << endl;
//	
//	res_ptr last;
//	foreach (res_ptr r, res)
//	{
//		if (r->letter == 0)
//		{
//			os << boost::format("%5.5d          0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0   0     0    0    0   0.000      0  1.00")
//				% r->seqNr << endl;
//		}
//		else
//		{
//			os << boost::format("%5.5d%5.5d %c") % r->seqNr % r->pdbNr % r->chain;
//
//			for (uint32 i = 0; i < 20; ++i)
//				os << boost::format("%4.4d") % r->dist[i];
//
//			uint32 relent = uint32(100 * r->entropy / log(20.0));
//			os << "  " << boost::format("%4.4d %4.4d %4.4d   %5.3f   %4.4d  %4.2f") % r->nocc % r->ndel % r->nins % r->entropy % relent % r->consweight << endl;
//		}
//	}
//	
//	// insertion list
//	
//	os << "## INSERTION LIST" << endl
//	   << " AliNo  IPOS  JPOS   Len Sequence" << endl;
//
//	foreach (hit_ptr h, hits)
//	{
//		//foreach (insertion& ins, h->insertions)
//		foreach (const insertion& ins, h->m_seq.insertions())
//		{
//			string s = ins.m_seq;
//			
//			if (s.length() <= 100)
//				os << boost::format(" %5.5d %5.5d %5.5d %5.5d ") % h->m_nr % (ins.m_ipos + h->m_offset) % ins.m_jpos % (ins.m_seq.length() - 2) << s << endl;
//			else
//			{
//				os << boost::format(" %5.5d %5.5d %5.5d %5.5d ") % h->m_nr % (ins.m_ipos + h->m_offset) % ins.m_jpos % (ins.m_seq.length() - 2) << s.substr(0, 100) << endl;
//				s.erase(0, 100);
//				
//				while (not s.empty())
//				{
//					uint32 n = s.length();
//					if (n > 100)
//						n = 100;
//					
//					os << "     +                   " << s.substr(0, n) << endl;
//					s.erase(0, n);
//				}
//			}
//		}			
//	}
	
	os << "//" << endl;
}

// --------------------------------------------------------------------
// Calculate the variability of a residue, based on dayhoff similarity
// and weights

uint32 kSentinel = numeric_limits<uint32>::max();
boost::mutex sSumLock;

bool is_gap(char aa) { return aa == ' ' or aa == '-'; }

void CalculateConservation(const vector<MHit*>& inHits, buffer<uint32>& b, vector<float>& csumvar, vector<float>& csumdist)
{
	vector<float> sumvar(csumvar.size()), sumdist(csumdist.size()), simval(csumdist.size());

	for (;;)
	{
		uint32 i = b.get();
		if (i == kSentinel)
			break;

		const string& si = inHits[i]->m_aligned;
		
		for (uint32 j = i + 1; j < inHits.size(); ++j)
		{
			const string& sj = inHits[j]->m_aligned;
	
			uint32 len = 0, agr = 0;
			for (uint32 k = 0; k < si.length(); ++k)
			{
				if (is_gap(si[k]) or is_gap(sj[k]))
					continue;

				++len;
				if (si[k] == sj[k])
					++agr;

				int8 ri = ResidueNr(si[k]);
				int8 rj = ResidueNr(sj[k]);

				if (ri < 20 and rj < 20)
					simval[k] = score(kDayhoffData, ri, rj);
				else
					simval[k] = numeric_limits<float>::min();
			}
			
			if (len == 0)
				continue;

			float distance = 1 - (float(agr) / float(len));
			for (uint32 k = 0; k < si.length(); ++k)
			{
				if (simval[k] != numeric_limits<float>::min())
				{
					sumvar[k] += distance * simval[k];
					sumdist[k] += distance * 1.5f;
				}
			}
		}
	}

	b.put(kSentinel);
	
	// accumulate our data
	boost::mutex::scoped_lock l(sSumLock);
	
	transform(sumvar.begin(), sumvar.end(), csumvar.begin(), csumvar.begin(), plus<float>());
	transform(sumdist.begin(), sumdist.end(), csumdist.begin(), csumdist.begin(), plus<float>());
}

void CalculateConservation(const sequence& inChain, vector<MHit*>& inHits, MResInfoList& inResidues)
{
	if (VERBOSE)
		cerr << "Calculating conservation weights...";

	vector<float> sumvar(inChain.length()), sumdist(inChain.length());
	
	// Calculate conservation weights in multiple threads to gain speed.
	buffer<uint32> b;
	boost::thread_group threads;
	//for (uint32 t = 0; t < boost::thread::hardware_concurrency(); ++t)
	//{
		threads.create_thread(boost::bind(&CalculateConservation, boost::ref(inHits),
			boost::ref(b), boost::ref(sumvar), boost::ref(sumdist)));
	//}
		
#pragma message("missing the query here!")
	for (uint32 i = 0; i + 1 < inHits.size(); ++i)
		b.put(i);
	
	b.put(kSentinel);
	threads.join_all();

	MResInfoList::iterator ri = inResidues.begin();
	for (uint32 i = 0; i < inChain.length(); ++i)
	{
		assert(ri != inResidues.end());

		float weight = 1.0f;
		if (sumdist[i] > 0)
			weight = sumvar[i] / sumdist[i];
		
		ri->m_consweight = weight;
		
		do {
			++ri;
		} while (ri != inResidues.end() and ri->m_chain_id == 0);
	}
	assert(ri == inResidues.end());

	if (VERBOSE)
		cerr << " done" << endl;
}

// --------------------------------------------------------------------

void CreateHSSP(const MProtein& inProtein, const vector<string>& inDatabanks,
	uint32 inMaxhits, uint32 inMinSeqLength, float inThreshold, uint32 inThreads, ostream& inOs)
{
	// construct a set of unique sequences, containing only the largest ones in case of overlap
	vector<sequence> seqset;
	vector<uint32> ix;
	vector<const MChain*> chains;
	string used;
	
	foreach (const MChain* chain, inProtein.GetChains())
	{
		string seq;
		chain->GetSequence(seq);
		
		if (seq.length() < inMinSeqLength)
			continue;
		
		chains.push_back(chain);
		seqset.push_back(encode(seq));
		ix.push_back(ix.size());
	}
	
	if (seqset.empty())
		throw runtime_error("Not enough sequences in PDB file of minimal length");

	if (seqset.size() > 1)
		ClusterSequences(seqset, ix);
	
	// only take the unique sequences
	ix.erase(unique(ix.begin(), ix.end()), ix.end());

	vector<MProfile*> profiles;

	uint32 seqlength = 0;
	foreach (uint32 i, ix)
	{
		const MChain& chain(*chains[ix[i]]);
		
		if (not used.empty())
			used += ", ";
		used += chain.GetChainID();
		
		unique_ptr<MProfile> profile(new MProfile(chain, seqset[ix[i]]));
		
		fs::path blastHits(inProtein.GetID() + '-' + chain.GetChainID() + "-hits.fa");
		fs::ifstream file(blastHits);
		if (not file.is_open())
			throw runtime_error("Could not open blast hit file");
		
		{
			progress pr1("processing", fs::file_size(blastHits));
			profile->Process(file, pr1);
		}
		
		CalculateConservation(seqset[ix[i]], profile->m_entries, profile->m_residues);

		seqlength += seqset[ix[i]].length();
		profiles.push_back(profile.release());
	}
	
	
	CreateHSSPOutput("1crn", "", inThreshold, seqlength, chains.size(), ix.size(), used,
		profiles.back()->m_entries, profiles.back()->m_residues, inOs);
		
//	uint32 seqlength = 0;
//
//	vector<mseq> alignments(inChainAlignments.size());
//	vector<const MChain*> chains;
//	vector<pair<uint32,uint32> > res_ranges;
//
//	res_list res;
//	hit_list hits;
//
//	uint32 kchain = 0;
//	foreach (string ch, inChainAlignments)
//	{
//		if (ch.length() < 3 or ch[1] != '=')
//			throw mas_exception(boost::format("Invalid chain/stockholm pair specified: '%s'") % ch);
//
//		const MChain& chain = inProtein.GetChain(ch[0]);
//		chains.push_back(&chain);
//
//		string seq;
//		chain.GetSequence(seq);
//
//		// strip off trailing X's. They are not very useful
//		while (ba::ends_with(seq, "X"))
//			seq.erase(seq.end() - 1);
//
//		if (VERBOSE > 1)
//			cerr << "Chain " << ch[0] << " => '" << seq << '\'' << endl;
//
//		seqlength += seq.length();
//		
//		// alignments are stored in datadir
//		fs::path afp = ch.substr(2);
//		if (not fs::exists(afp))
//		{
//			fs::path dataDir = "/data/hssp2/sto/";
//			afp = dataDir / (ch.substr(2) + ".aln.bz2");
//		}
//		if (not fs::exists(afp))
//			throw mas_exception("alignment is missing, exiting");
//
//		fs::ifstream af(afp, ios::binary);
//		if (not af.is_open())
//			throw mas_exception(boost::format("Could not open alignment file '%s'") % afp);
//
//		if (VERBOSE)
//			cerr << "Using fasta file '" << afp << '\'' << endl;
//
//		io::filtering_stream<io::input> in;
//		in.push(io::bzip2_decompressor());
//		in.push(af);
//
//		try {
//			ReadFastA(in, alignments[kchain], seq, inMaxHits, inMinLength, inCutOff);
//		}
//		catch (...)
//		{
//			cerr << "exception while reading file " << afp << endl;
//			throw;
//		}
//
//		// Remove all hits that are not above the threshold here
//		mseq& msa = alignments[kchain];
//		msa.erase(remove_if(msa.begin() + 1, msa.end(), boost::bind(&seq::drop, _1, inCutOff)), msa.end());
//
//		++kchain;
//	}
//
//	string usedChains;
//	kchain = 0;
//	foreach (const MChain* chain, chains)
//	{
//		if (not res.empty())
//			res.push_back(res_ptr(new ResidueHInfo(res.size() + 1)));
//		
//		uint32 first = res.size();
//		
//		mseq& msa = alignments[kchain];
//		ChainToHits(inDatabank, msa, *chain, hits, res);
//		
//		res_ranges.push_back(make_pair(first, res.size()));
//
//		if (not usedChains.empty())
//			usedChains += ',';
//		usedChains += chain->GetChainID();
//
//		++kchain;
//	}
//
//	sort(hits.begin(), hits.end(), compare_hit());
//
//	if (inMaxHits > 0 and hits.size() > inMaxHits)
//		hits.erase(hits.begin() + inMaxHits, hits.end());
//
//	if (hits.empty())
//		throw mas_exception("No hits found or remaining");
//	
//	uint32 nr = 1;
//	foreach (hit_ptr h, hits)
//		h->m_nr = nr++;
//
//	for (uint32 c = 0; c < kchain; ++c)
//	{
//		pair<uint32,uint32> range = res_ranges[c];
//		
//		res_range r(res.begin() + range.first, res.begin() + range.second);
//		CalculateConservation(alignments[c], r);
//
//		foreach (res_ptr ri, r)
//			ri->CalculateVariability(hits);
//	}
//	
//	stringstream desc;
//	if (inProtein.GetHeader().length() >= 50)
//		desc << "HEADER     " + inProtein.GetHeader().substr(10, 40) << endl;
//	if (inProtein.GetCompound().length() > 10)
//		desc << "COMPND     " + inProtein.GetCompound().substr(10) << endl;
//	if (inProtein.GetSource().length() > 10)
//		desc << "SOURCE     " + inProtein.GetSource().substr(10) << endl;
//	if (inProtein.GetAuthor().length() > 10)
//		desc << "AUTHOR     " + inProtein.GetAuthor().substr(10) << endl;
//
//	CreateHSSPOutput(inDatabank, inProtein.GetID(), desc.str(), inCutOff, seqlength,
//		inProtein.GetChains().size(), kchain, usedChains, hits, res, outHSSP);
//	
}

}

// --------------------------------------------------------------------

#if defined(_MSC_VER)
#include <conio.h>
#endif

int main(int argc, char* argv[])
{
#if P_UNIX
	// enable the dumping of cores to enable postmortem debugging
	rlimit l;
	if (getrlimit(RLIMIT_CORE, &l) == 0)
	{
		l.rlim_cur = l.rlim_max;
		if (l.rlim_cur == 0 or setrlimit(RLIMIT_CORE, &l) < 0)
			cerr << "Failed to set rlimit" << endl;
	}
#endif

	try
	{
		po::options_description desc("MKHSSP options");
		desc.add_options()
			("help,h",							 "Display help message")
			("input,i",		po::value<string>(), "Input PDB file (or PDB ID)")
			("output,o",	po::value<string>(), "Output file, use 'stdout' to output to screen")
			("databank,d",	po::value<vector<string>>(),
												 "Databank(s) to use")
			("threads,a",	po::value<uint32>(), "Number of threads (default is maximum)")
			("min-length",	po::value<uint32>(), "Minimal chain length")
			("max-hits,m",	po::value<uint32>(), "Maximum number of hits to include (default = 1500)")
			("threshold",	po::value<float>(),  "Homology threshold adjustment (default = 0.05)")
			("verbose,v",						 "Verbose output")
			;
	
		po::positional_options_description p;
		p.add("input", 1);
		p.add("output", 2);
	
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

		fs::path home = get_home();
		if (fs::exists(home / ".hssprc"))
		{
			fs::ifstream rc(home / ".hssprc");
			po::store(po::parse_config_file(rc, desc), vm);
		}

		po::notify(vm);

		if (vm.count("help") or not vm.count("input") or vm.count("databank") == 0)
		{
			cerr << desc << endl;
			exit(1);
		}

		VERBOSE = vm.count("verbose") > 0;
		
		vector<string> databanks(vm["databank"].as<vector<string>>());
			
		uint32 minlength = 25;
		if (vm.count("min-length"))
			minlength= vm["min-length"].as<uint32>();

		uint32 maxhits = 1500;
		if (vm.count("max-hits"))
			maxhits= vm["max-hits"].as<uint32>();

		float threshold = 0.05f;
		if (vm.count("threshold"))
			threshold = vm["threshold"].as<float>();

		uint32 threads = boost::thread::hardware_concurrency();
		if (vm.count("threads"))
			threads = vm["threads"].as<uint32>();
		if (threads < 1)
			threads = 1;
			
		// what input to use
		string input = vm["input"].as<string>();
		io::filtering_stream<io::input> in;
		ifstream infile(input.c_str(), ios_base::in | ios_base::binary);
		if (not infile.is_open())
			throw runtime_error("Error opening input file");

		if (ba::ends_with(input, ".bz2"))
		{
			in.push(io::bzip2_decompressor());
			input.erase(input.length() - 4, string::npos);
		}
		else if (ba::ends_with(input, ".gz"))
		{
			in.push(io::gzip_decompressor());
			input.erase(input.length() - 3, string::npos);
		}
		in.push(infile);

		// Where to write our HSSP file to:
		// either to cout or an (optionally compressed) file.
		ofstream outfile;
		io::filtering_stream<io::output> out;

		if (vm.count("output") and vm["output"].as<string>() != "stdout")
		{
			string output = vm["output"].as<string>();
			outfile.open(output.c_str(), ios_base::out|ios_base::trunc|ios_base::binary);
			
			if (not outfile.is_open())
				throw runtime_error("could not create output file");
			
			if (ba::ends_with(output, ".bz2"))
				out.push(io::bzip2_compressor());
			else if (ba::ends_with(output, ".gz"))
				out.push(io::gzip_compressor());
			out.push(outfile);
		}
		else
			out.push(cout);

		// read protein and calculate the secondary structure
		MProtein a(in);
		a.CalculateSecondaryStructure();
		
		// create the HSSP file
		HSSP::CreateHSSP(a, databanks, maxhits, minlength, threshold, threads, out);
	}
	catch (exception& e)
	{
		cerr << e.what() << endl;
		exit(1);
	}

#if defined(_MSC_VER) && ! NDEBUG
	cerr << "Press any key to quit application ";
	char ch = _getch();
	cerr << endl;
#endif
	
	return 0;
}
