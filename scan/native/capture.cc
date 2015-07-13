/* copyright 2015 by mike lodato (zvxryb@gmail.com)
 * this work is subject to the terms of the MIT license */

#include "capture.hh"

#include <sys/ioctl.h>

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

// from the V4L2 video capture example
static int xioctl(int fd, int req, void *arg) {
	int ret = 0;
	do ret = ioctl(fd, req, arg);
	while (ret < 0 && errno == EINTR);
	return ret;
}

namespace capture {
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

	InfoPtr query(int fd) {
		v4l2_capability caps = {};
		if (xioctl(fd, static_cast<int>(VIDIOC_QUERYCAP), &caps) < 0) {
			return nullptr;
		}
		return InfoPtr(new Info(caps));
	}

	FormatListPtr formats(int fd) {
		FormatListPtr formats(new FormatList());
		
		int ret = 0;
		uint32_t i = 0;
		for (;;) {
			v4l2_fmtdesc fmt = {};
			fmt.index = i++;
			fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			ret = xioctl(fd, static_cast<int>(VIDIOC_ENUM_FMT), &fmt);
			if (ret < 0) break;
			formats->push_back(Format(fmt));
		}
		if (errno != EINVAL)
			return nullptr;
		return formats;
	}

	FrameSizeListPtr frameSizes(int fd, uint32_t fourcc) {
		FrameSizeListPtr frameSizes(new FrameSizeList());
		
		int ret = 0;
		uint32_t i = 0;
		for (;;) {
			v4l2_frmsizeenum sizeInfo = {};
			sizeInfo.index        = i++;
			sizeInfo.pixel_format = fourcc;
			ret = xioctl(fd, static_cast<int>(VIDIOC_ENUM_FRAMESIZES), &sizeInfo);
			if (ret < 0) break;
			if (sizeInfo.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
				v4l2_frmsize_discrete const &size = sizeInfo.discrete;
				frameSizes->push_back(FrameSize(size.width, size.height));
			}
		}
		if (errno != EINVAL)
			return nullptr;
		return frameSizes;
	}
}

BOOST_PYTHON_MODULE(capture) {
	boost::python::class_<capture::FrameSize>("FrameSize")
		.def_readonly("width", &capture::FrameSize::width)
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
}

