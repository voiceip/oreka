#ifdef SUPPORTS_CPP11

#include <memory>

namespace oreka {
	using std::shared_ptr;
};

#else

#include <boost/shared_ptr.hpp>

namespace oreka {
	using boost::shared_ptr;
};

#endif
