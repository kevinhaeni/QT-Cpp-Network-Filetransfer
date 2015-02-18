#pragma once

#define NOMINMAX

#include <windows.h>

#include <stdexcept>
#include <string>
#include <iostream>
#include <sstream>
#include <limits>
#include <cassert>
#include <ctime>

#include <util/SharedPtr.hpp>
#include <util/Error.hpp>
#include <util/GetOpt.hpp>
#include <util/MemoryStream.hpp>
#include <util/ScopedArray.hpp>

#include <net/BindingFactory.hpp>
#include <net/StreamListener.hpp>

#include <msg/Messenger.hpp>
