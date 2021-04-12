#pragma once

#ifndef JSON_HEADER_FILE_H
#define JSON_HEADER_FILE_H

/**
* Need to undef snprintf other MSVC throws error that it is not part of std::
* Somewhere when including pjsua2.hpp this gets set.
*/
#undef snprintf
#include <nlohmann/json.hpp>

#endif