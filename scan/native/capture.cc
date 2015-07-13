/* copyright 2015 by mike lodato (zvxryb@gmail.com)
 * this work is subject to the terms of the MIT license */

#include "capture.hh"

#include <functional>
#include <sys/ioctl.h>

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

// from the V4L2 video capture example
static int xioctl(int fd, unsigned long req, void *arg) {
	int ret = 0;
	do ret = ioctl(fd, req, arg);
	while (ret < 0 && errno == EINTR);
	return ret;
}

namespace capture {
	Interval::Interval(uint32_t numerator, uint32_t denominator)
		: numerator(numerator), denominator(denominator) {
	}
	
	bool Interval::operator==(Interval const &other) const {
		return this->numerator   == other.numerator &&
		       this->denominator == other.denominator;
	}

	FrameSize::FrameSize(uint32_t w, uint32_t h)
		: width(w), height(h) {}
	
	bool FrameSize::operator==(FrameSize const &other) const {
		return this->width == other.width && this->height == other.height;
	}
	
	Format::Format(v4l2_fmtdesc const &fmt)
		: description(reinterpret_cast<char const*>(fmt.description))
		, fourcc    (fmt.pixelformat)
		, compressed(fmt.flags & V4L2_FMT_FLAG_COMPRESSED)
		, emulated  (fmt.flags & V4L2_FMT_FLAG_EMULATED) {
	}

	bool Format::operator==(Format const &other) const {
		return this->fourcc == other.fourcc;
	}

	Info::Info(v4l2_capability const &cap)
		: driver  (reinterpret_cast<char const*>(cap.driver  ))
		, card    (reinterpret_cast<char const*>(cap.card    ))
		, bus_info(reinterpret_cast<char const*>(cap.bus_info))
		, version (cap.version )
		, capture (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
	}

	string Info::format() const {
		return card + " (" + driver + ", " + bus_info + ")";
	}

	template <unsigned long req> struct RequestInfo;
	template <> struct RequestInfo<VIDIOC_QUERYCAP> {
		typedef v4l2_capability type;
	};
	template <> struct RequestInfo<VIDIOC_ENUM_FMT> {
		typedef v4l2_fmtdesc type;
	};
	template <> struct RequestInfo<VIDIOC_ENUM_FRAMESIZES> {
		typedef v4l2_frmsizeenum type;
	};
	template <> struct RequestInfo<VIDIOC_ENUM_FRAMEINTERVALS> {
		typedef v4l2_frmivalenum type;
	};

	template <unsigned long req>
	bool request(int fd, typename RequestInfo<req>::type &arg) {
		return xioctl(fd, req, &arg) >= 0;
	}

	InfoPtr query(int fd) {
		v4l2_capability caps = {};
		if (!request<VIDIOC_QUERYCAP>(fd, caps))
			return nullptr;
		return InfoPtr(new Info(caps));
	}

	template <class T, unsigned long req>
	shared_ptr<vector<T> > enumerate(int fd,
		std::function<void          (typename RequestInfo<req>::type &, int)> &&initArgs,
		std::function<unique_ptr<T> (typename RequestInfo<req>::type &&)    > &&convert)
	{
		shared_ptr<vector<T> > results(new vector<T>());
		
		uint32_t i = 0;
		for (;;) {
			typename RequestInfo<req>::type arg = {};
			initArgs(arg, i++);
			if (!request<req>(fd, arg))
				break;
			unique_ptr<T> result = convert(std::move(arg));
			if (result == nullptr)
				continue;
			results->push_back(*result);
		}
		if (errno != EINVAL)
			return nullptr;
		return results;
	}

	FormatListPtr formats(int fd) {
		return enumerate<Format, VIDIOC_ENUM_FMT>(fd,
			[](v4l2_fmtdesc &fmt, int i) {
				fmt.index = i++;
				fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			},
			[](v4l2_fmtdesc &&fmt) {
				return unique_ptr<Format>(new Format(fmt));
			});
	}

	FrameSizeListPtr frameSizes(int fd, uint32_t fourcc) {
		return enumerate<FrameSize, VIDIOC_ENUM_FRAMESIZES>(fd,
			[fourcc](v4l2_frmsizeenum &sizeInfo, int i) {
				sizeInfo.index        = i++;
				sizeInfo.pixel_format = fourcc;
			},
			[](v4l2_frmsizeenum &&sizeInfo) -> unique_ptr<FrameSize> {
				if (sizeInfo.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
					v4l2_frmsize_discrete const &size = sizeInfo.discrete;
					return unique_ptr<FrameSize>(
						new FrameSize(size.width, size.height));
				}
				return nullptr;
			});
	}

	IntervalListPtr intervals (int fd, uint32_t fourcc,
		uint32_t width, uint32_t height)
	{
		return enumerate<Interval, VIDIOC_ENUM_FRAMEINTERVALS>(fd,
			[fourcc, width, height](v4l2_frmivalenum &arg, int i) {
				arg.index        = i;
				arg.pixel_format = fourcc;
				arg.width        = width;
				arg.height       = height;
			},
			[](v4l2_frmivalenum &&arg) -> unique_ptr<Interval> {
				if (arg.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
					return unique_ptr<Interval>(new Interval(
						arg.discrete.numerator, arg.discrete.denominator));
				}
				return nullptr;
			});
	}
}

BOOST_PYTHON_MODULE(capture) {
	boost::python::class_<capture::Interval>("Interval")
		.def_readonly("numerator",   &capture::Interval::numerator)
		.def_readonly("denominator", &capture::Interval::denominator);
	boost::python::class_<
			capture::IntervalList,
			capture::IntervalListPtr
		>("IntervalList")
		.def(boost::python::vector_indexing_suite<capture::IntervalList>());
	boost::python::class_<capture::FrameSize>("FrameSize")
		.def_readonly("width",  &capture::FrameSize::width)
		.def_readonly("height", &capture::FrameSize::height);
	boost::python::class_<
			capture::FrameSizeList,
			capture::FrameSizeListPtr
		>("FrameSizeList")
		.def(boost::python::vector_indexing_suite<capture::FrameSizeList>());
	boost::python::class_<capture::Format>("Format")
		.def_readonly("description", &capture::Format::description)
		.def_readonly("fourcc",      &capture::Format::fourcc)
		.def_readonly("compressed",  &capture::Format::compressed)
		.def_readonly("emulated",    &capture::Format::emulated);
	boost::python::class_<
			capture::FormatList,
			capture::FormatListPtr
		>("FormatList")
		.def(boost::python::vector_indexing_suite<capture::FormatList>());
	boost::python::class_<capture::Info, capture::InfoPtr>("Info")
		.def("__str__", &capture::Info::format)
		.def_readonly("driver",   &capture::Info::driver)
		.def_readonly("card",     &capture::Info::card)
		.def_readonly("bus_info", &capture::Info::bus_info)
		.def_readonly("version",  &capture::Info::version)
		.def_readonly("capture",  &capture::Info::capture);
	boost::python::def("query",       capture::query);
	boost::python::def("formats",     capture::formats);
	boost::python::def("frame_sizes", capture::frameSizes);
	boost::python::def("intervals",   capture::intervals);
}

