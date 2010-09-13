// utility routines for mas

#include "mas.h"

#include <iomanip>
#include <iostream>
#include <boost/thread.hpp>
#include <time.h>
#if defined(_MSC_VER)
#define TERM_WIDTH 80
#else
#include <termios.h>
#include <sys/ioctl.h>
#endif

#include "utils.h"

using namespace std;

// --------------------------------------------------------------------

mas_exception::mas_exception(const string& msg)
{
	snprintf(m_msg, sizeof(m_msg), "%s", msg.c_str());
}

mas_exception::mas_exception(const boost::format& msg)
{
	snprintf(m_msg, sizeof(m_msg), "%s", msg.str().c_str());
}

// --------------------------------------------------------------------

uint32 get_terminal_width()
{
	uint32 width;

#if defined(_MSC_VER)
	width = TERM_WIDTH;
#else
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	width = w.ws_col;
#endif

	return width;
}

// --------------------------------------------------------------------

progress::progress(const string& msg, uint32 max)
	: m_msg(msg)
	, m_step(0)
	, m_max(max)
	, m_thread(boost::bind(&progress::run, this))
{
	time(&m_start);
}

progress::~progress()
{
	boost::mutex::scoped_lock lock(m_mutex);
	m_step = m_max;
	m_thread.interrupt();
	m_thread.join();
}

void progress::step()
{
	boost::mutex::scoped_lock lock(m_mutex);
	++m_step;
}

void progress::run()
{
	if (VERBOSE)
		return;

	bool first = true;

	// first, get the current terminal width
	uint32 width = get_terminal_width();

	try
	{
		for (;;)
		{
			boost::this_thread::sleep(boost::posix_time::seconds(1));

			string msg;
			if (m_msg.length() > width)
				msg = m_msg.substr(0, width);
			else
			{
				msg = m_msg;
				
				if (msg.length() + 10 < width)
				{
					float fraction = float(m_step) / m_max;
					if (fraction < 0)
						fraction = 0;
					else if (fraction > 1)
						fraction = 1;
					
					uint32 thermometer_width = width - msg.length() - 10;
					uint32 thermometer_done_width = uint32(fraction * thermometer_width);
					msg += " [";
					msg += string(thermometer_done_width, '#');
					msg += string(thermometer_width - thermometer_done_width, '-');
					msg += ']';
					msg += string(width - msg.length(), ' ');
				}
			}

			cout << '\r' << msg << flush;
			first = false;
		}
	}
	catch (boost::thread_interrupted&)
	{
		if (not first)
			cout << '\r' << m_msg << " done" << string(width - m_msg.length() - 5, ' ') << endl;
	}
}