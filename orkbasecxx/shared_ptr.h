#ifdef SUPPORTS_CPP11

#include <memory>

namespace oreka {
	using std::shared_ptr;
	using std::make_shared;
};

#else

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace oreka {
	using boost::shared_ptr;
	using boost::make_shared;
};

#endif
