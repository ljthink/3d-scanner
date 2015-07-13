/* copyright 2015 by mike lodato (zvxryb@gmail.com)
 * this work is subject to the terms of the MIT license */

#ifndef CAPTURE_H
#define CAPTURE_H

#include <linux/videodev2.h>

#include <boost/smart_ptr.hpp>

#include <string>

namespace capture {
	using boost::shared_ptr;
	using std::vector;
	using std::string;
	using std::unique_ptr;
	
	struct Interval {
		Interval() : numerator(), denominator() {}
		Interval(uint32_t, uint32_t);
		bool operator==(Interval const &) const;
		uint32_t numerator;
		uint32_t denominator;
	};
	typedef vector    <Interval    > IntervalList;
	typedef shared_ptr<IntervalList> IntervalListPtr;
	
	struct FrameSize {
		FrameSize() : width(), height() {}
		FrameSize(uint32_t, uint32_t);
		bool operator==(FrameSize const &) const;
		uint32_t width;
		uint32_t height;
	};
	typedef vector    <FrameSize    > FrameSizeList;
	typedef shared_ptr<FrameSizeList> FrameSizeListPtr;
	
	struct Format {
		Format() : compressed(), emulated() {}
		Format(v4l2_fmtdesc const &);
		bool operator==(Format const &) const;
		string   description;
		uint32_t fourcc;
		bool     compressed;
		bool     emulated;
	};
	typedef vector    <Format    > FormatList;
	typedef shared_ptr<FormatList> FormatListPtr;
	
	struct Info {
		Info() : version(), capture() {}
		Info(v4l2_capability const &);
		string   format() const;
		string   driver;
		string   card;
		string   bus_info;
		uint32_t version;
		bool     capture;
	};
	typedef shared_ptr<Info> InfoPtr;
	
	InfoPtr          query     (int);
	FormatListPtr    formats   (int);
	FrameSizeListPtr frameSizes(int, uint32_t);
	IntervalListPtr  intervals (int, uint32_t, uint32_t, uint32_t);
}

#endif

